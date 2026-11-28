#include "ComunicacionWiFi.h"
#include "InterfazWeb.h"
#include "Comandos.h"
#include "Motor.h"
#include "ComunicacionBLE.h"
#include "Seguridad.h"
#include "CredencialesWiFi.h"
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

static WebServer servidor(80);
static bool wifiActivo = false;
static IPAddress ipLocal(192,168,4,1), gateway(192,168,4,1), subnet(255,255,255,0);
struct Peticion { uint32_t id, tiempo; char comando[65], token[33]; };
struct Respuesta { uint32_t id; char texto[512]; };
static QueueHandle_t peticiones, respuestas;
static SemaphoreHandle_t mutexEstado;
static char estado[512] = "{}";
static char seguridad[256] = "";
static char token[33] = ""; // Solo se modifica en el ciclo de control.
static uint32_t contador = 0;
static void publicarEstado() {
    if (xSemaphoreTake(mutexEstado,0) != pdTRUE) return;
    snprintf(estado,sizeof(estado),"{\"activo\":%s,\"estado\":\"%s\",\"pwm\":%d,\"direccion\":\"%s\",\"modo\":\"%s\",\"giro\":%lu,\"pausa\":%lu,\"propietario\":\"%s\",\"motivo\":\"%s\"}", sistemaMotorActivo()?"true":"false",obtenerEstadoTexto(),obtenerVelocidad(),obtenerSentidoTexto(),obtenerModoTexto(),obtenerTiempoGiro(),obtenerTiempoPausa(),obtenerPropietarioTexto(),obtenerMotivoDetencionTexto());
    generarEstadoSeguridad().toCharArray(seguridad,sizeof(seguridad));
    xSemaphoreGive(mutexEstado);
}
static void enviar(int codigo,const char* tipo,const String& texto) {
    servidor.sendHeader("Cache-Control","no-store"); servidor.send(codigo,tipo,texto);
}
static void atenderEstado() {
    char copia[512];
    if(xSemaphoreTake(mutexEstado,pdMS_TO_TICKS(20))!=pdTRUE){enviar(503,"text/plain","OCUPADO");return;}
    memcpy(copia,estado,sizeof(copia)); xSemaphoreGive(mutexEstado);
    enviar(200,"application/json",copia);
}
static void atenderSeguridad() {
    char copia[256];
    if(xSemaphoreTake(mutexEstado,pdMS_TO_TICKS(20))!=pdTRUE){enviar(503,"text/plain","OCUPADO");return;}
    memcpy(copia,seguridad,sizeof(copia)); xSemaphoreGive(mutexEstado);
    enviar(200,"text/plain",copia);
}
static void atenderDiagnostico() {
    String j="{\"firmware\":\"1.3\",\"microcontrolador\":\""; j+=ESP.getChipModel();
    j+="\",\"flash_total\":";j+=String(ESP.getFlashChipSize());
    j+=",\"flash_programa\":";j+=String(ESP.getSketchSize());
    j+=",\"ram_libre\":";j+=String(ESP.getFreeHeap());
    j+=",\"ram_minima\":";j+=String(ESP.getMinFreeHeap());
    j+=",\"bluetooth_conectado\":";j+=bluetoothConectado()?"true":"false";
    j+=",\"wifi_activo\":true,\"clientes_wifi\":";j+=String(WiFi.softAPgetStationNum());
    j+=",\"ip\":\"";j+=WiFi.softAPIP().toString();j+="\",\"tiempo_activo\":";j+=String(millis()/1000UL);j+="}";
    enviar(200,"application/json",j);
}
static void atenderComando(bool hb) {
    if(servidor.header("X-Dosificadora")!="1"){enviar(403,"text/plain","ERROR:CABECERA");return;}
    String c=hb?String("HEARTBEAT"):servidor.arg("plain"); c.trim();c.toUpperCase();
    String t=servidor.header("X-Control");
    if(c.length()==0||c.length()>64||t.length()>32){enviar(400,"text/plain","ERROR:FORMATO");return;}
    Peticion p{};p.id=++contador;p.tiempo=millis();c.toCharArray(p.comando,sizeof(p.comando));t.toCharArray(p.token,sizeof(p.token));
    if(xQueueSend(peticiones,&p,0)!=pdTRUE){enviar(503,"text/plain","ERROR:COLA");return;}
    Respuesta r{};uint32_t inicio=millis();
    while(millis()-inicio<350){if(xQueueReceive(respuestas,&r,pdMS_TO_TICKS(10))==pdTRUE && r.id==p.id){enviar(200,"text/plain",r.texto);return;}}
    enviar(503,"text/plain","ERROR:TIMEOUT");
}
static void tareaHTTP(void*) { for(;;){servidor.handleClient();vTaskDelay(1);} }
bool inicializarWiFi() {
    peticiones=xQueueCreate(1,sizeof(Peticion));respuestas=xQueueCreate(1,sizeof(Respuesta));mutexEstado=xSemaphoreCreateMutex();
    if(!peticiones||!respuestas||!mutexEstado)return false;
    WiFi.mode(WIFI_AP);
    if(!WiFi.softAPConfig(ipLocal,gateway,subnet)||!WiFi.softAP(SSID,PASSWORD,1,false,1)){WiFi.mode(WIFI_OFF);return false;}
    const char* headers[]={"X-Dosificadora","X-Control"};servidor.collectHeaders(headers,2);
    servidor.on("/",HTTP_GET,[]{servidor.sendHeader("Cache-Control","no-store");servidor.send_P(200,"text/html; charset=utf-8",PAGINA_WEB);});
    servidor.on("/api/status",HTTP_GET,atenderEstado);servidor.on("/api/seguridad",HTTP_GET,atenderSeguridad);servidor.on("/api/diagnostico",HTTP_GET,atenderDiagnostico);
    servidor.on("/api/comando",HTTP_POST,[]{atenderComando(false);});servidor.on("/api/heartbeat",HTTP_POST,[]{atenderComando(true);});
    servidor.onNotFound([]{enviar(404,"text/plain","Pagina no encontrada");});
    publicarEstado();servidor.begin();
    if(xTaskCreatePinnedToCore(tareaHTTP,"HTTP",6144,nullptr,1,nullptr,0)!=pdPASS){servidor.stop();WiFi.mode(WIFI_OFF);return false;}
    wifiActivo=true;Serial.println("WiFi y tarea HTTP: OK");return true;
}
void actualizarWiFi() {
    if(!wifiActivo)return;
    actualizarSeguridad();
    if(!tieneControl(CanalControl::WIFI))token[0]=0;
    if(tieneControl(CanalControl::WIFI)&&WiFi.softAPgetStationNum()==0){notificarDesconexion(CanalControl::WIFI);token[0]=0;}
    Peticion p{};
    if(xQueueReceive(peticiones,&p,0)==pdTRUE){
        String c=p.comando,r;
        if(millis()-p.tiempo>=300 && c!="STOP")r="ERROR:PETICION_EXPIRADA";
        else if(c=="STOP"){r=procesarComando(c,CanalControl::WIFI);token[0]=0;}
        else if(c=="CONTROL:TAKE"){
            if(tieneControl(CanalControl::WIFI))r="ERROR:CONTROL_NO_DISPONIBLE";
            else {r=procesarComando(c,CanalControl::WIFI);if(r.startsWith("OK:")){uint32_t a=esp_random(),b=esp_random(),d=esp_random(),e=esp_random();snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",(unsigned long)a,(unsigned long)b,(unsigned long)d,(unsigned long)e);r+=";TOKEN=";r+=token;}}
        } else if(!tieneControl(CanalControl::WIFI)||!token[0]||strcmp(token,p.token)!=0)r="ERROR:SIN_AUTORIZACION";
        else r=procesarComando(c,CanalControl::WIFI);
        if(!tieneControl(CanalControl::WIFI))token[0]=0;
        publicarEstado();
        Respuesta salida{};salida.id=p.id;r.toCharArray(salida.texto,sizeof(salida.texto));xQueueOverwrite(respuestas,&salida);
    }
    static uint32_t anterior=0; if(millis()-anterior>=50){anterior=millis();publicarEstado();}
}
bool wifiDisponible(){return wifiActivo;}
uint8_t obtenerClientesWiFi(){return wifiActivo?WiFi.softAPgetStationNum():0;}
String obtenerIPWiFi(){return wifiActivo?WiFi.softAPIP().toString():String("0.0.0.0");}
