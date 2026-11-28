
#include "ComunicacionWiFi.h"
#include "InterfazWeb.h"
#include "Comandos.h"
#include "Motor.h"
#include "ComunicacionBLE.h"

#include <WiFi.h>
#include <WebServer.h>

// ======================================
// CONFIGURACION WIFI
// ======================================

#include "CredencialesWiFi.h"

static WebServer servidor(80);

static bool wifiActivo = false;

static IPAddress ipLocal(192, 168, 4, 1);
static IPAddress gateway(192, 168, 4, 1);
static IPAddress subnet(255, 255, 255, 0);

// ======================================
// INTERFAZ WEB
// ======================================

static void atenderInicio() {

    servidor.sendHeader("Cache-Control", "no-store");

    servidor.send_P(
        200,
        "text/html; charset=utf-8",
        PAGINA_WEB
    );
}

// ======================================
// API: ESTADO DEL MOTOR
// ======================================

static void atenderEstado() {

    String json;

    json.reserve(256);

    json = "{";

    json += "\"activo\":";
    json += sistemaMotorActivo() ? "true" : "false";

    json += ",\"estado\":\"";
    json += obtenerEstadoTexto();
    json += "\"";

    json += ",\"pwm\":";
    json += String(obtenerVelocidad());

    json += ",\"direccion\":\"";
    json += obtenerSentidoTexto();
    json += "\"";

    json += ",\"modo\":\"";
    json += obtenerModoTexto();
    json += "\"";

    json += ",\"giro\":";
    json += String(obtenerTiempoGiro());

    json += ",\"pausa\":";
    json += String(obtenerTiempoPausa());

    json += "}";

    servidor.sendHeader("Cache-Control", "no-store");

    servidor.send(
        200,
        "application/json",
        json
    );
}

// ======================================
// API: DIAGNOSTICO
// ======================================

static void atenderDiagnostico() {

    String json;

    json.reserve(384);

    json = "{";

    json += "\"firmware\":\"1.3\"";

    json += ",\"microcontrolador\":\"";
    json += ESP.getChipModel();
    json += "\"";

    json += ",\"flash_total\":";
    json += String(ESP.getFlashChipSize());

    json += ",\"flash_programa\":";
    json += String(ESP.getSketchSize());

    json += ",\"ram_libre\":";
    json += String(ESP.getFreeHeap());

    json += ",\"ram_minima\":";
    json += String(ESP.getMinFreeHeap());

    json += ",\"bluetooth_conectado\":";
    json += bluetoothConectado() ? "true" : "false";

    json += ",\"wifi_activo\":";
    json += wifiActivo ? "true" : "false";

    json += ",\"clientes_wifi\":";
    json += String(WiFi.softAPgetStationNum());

    json += ",\"ip\":\"";
    json += WiFi.softAPIP().toString();
    json += "\"";

    json += ",\"tiempo_activo\":";
    json += String(millis() / 1000UL);

    json += "}";

    servidor.sendHeader("Cache-Control", "no-store");

    servidor.send(
        200,
        "application/json",
        json
    );
}

// ======================================
// PAGINA NO ENCONTRADA
// ======================================

static void atenderNoEncontrado() {

    servidor.send(
        404,
        "text/plain",
        "Pagina no encontrada"
    );
}

// ======================================
// INICIALIZACION WIFI
// ======================================

bool inicializarWiFi() {

    Serial.println("--------------------------------");
    Serial.println("Inicializando WiFi...");

    WiFi.mode(WIFI_AP);

    if (!WiFi.softAPConfig(ipLocal, gateway, subnet)) {

        Serial.println("ERROR: Configuracion IP");

        WiFi.mode(WIFI_OFF);

        return false;
    }

    if (!WiFi.softAP(SSID, PASSWORD, 1, false, 1)) {

        Serial.println("ERROR: Creacion de red WiFi");

        WiFi.mode(WIFI_OFF);

        return false;
    }

    // Rutas HTTP

    servidor.on(
        "/",
        HTTP_GET,
        atenderInicio
    );

    servidor.on(
        "/api/status",
        HTTP_GET,
        atenderEstado
    );

    servidor.on(
        "/api/diagnostico",
        HTTP_GET,
        atenderDiagnostico
    );

    servidor.onNotFound(atenderNoEncontrado);

    servidor.begin();

    wifiActivo = true;

    Serial.println("WiFi inicializado correctamente");

    Serial.print("Red: ");
    Serial.println(SSID);

    Serial.print("Direccion IP: ");
    Serial.println(WiFi.softAPIP());

    Serial.println("Servidor HTTP: OK");
    Serial.println("Interfaz web: OK");
    Serial.println("API Estado: OK");
    Serial.println("API Diagnostico: OK");

    Serial.println("--------------------------------");

    return true;
}

// ======================================
// ACTUALIZACION WIFI
// ======================================

void actualizarWiFi() {

    if (wifiActivo) {

        servidor.handleClient();
    }
}

// ======================================
// ESTADO WIFI
// ======================================

bool wifiDisponible() {

    return wifiActivo;
}

// ======================================
// CLIENTES CONECTADOS
// ======================================

uint8_t obtenerClientesWiFi() {

    if (!wifiActivo) {

        return 0;
    }

    return WiFi.softAPgetStationNum();
}

// ======================================
// DIRECCION IP
// ======================================

String obtenerIPWiFi() {

    if (!wifiActivo) {

        return "0.0.0.0";
    }

    return WiFi.softAPIP().toString();
}
