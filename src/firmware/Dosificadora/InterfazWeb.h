
#ifndef INTERFAZ_WEB_H
#define INTERFAZ_WEB_H

#include <Arduino.h>

static const char PAGINA_WEB[] PROGMEM = R"HTML(

<!DOCTYPE html>
<html lang="es">

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1.0">

<title>Dosificadora ESP32</title>

<style>

/* ======================================
   CONFIGURACION GENERAL
   ====================================== */

:root {
    --fondo: #101722;
    --panel: #1c2737;
    --azul: #60a5fa;
    --verde: #22c55e;
    --rojo: #ef4444;
    --amarillo: #fbbf24;
    --texto: #f1f5f9;
    --secundario: #94a3b8;
    --borde: #334155;
}

* {
    box-sizing: border-box;
}

body {
    margin: 0;
    padding: 20px;
    background: var(--fondo);
    color: var(--texto);
    font-family: Arial, sans-serif;
}

.contenedor {
    max-width: 480px;
    margin: auto;
}

/* ======================================
   ENCABEZADO
   ====================================== */

.encabezado {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    gap: 10px;
}

h1 {
    font-size: 23px;
    margin: 0;
}

.subtitulo {
    font-size: 11px;
    color: var(--secundario);
    letter-spacing: 1px;
}

.conexion {
    color: var(--secundario);
    font-size: 11px;
    text-align: right;
}

.conexion.conectado {
    color: var(--verde);
}

.conexion.desconectado {
    color: var(--rojo);
}

/* ======================================
   NAVEGACION
   ====================================== */

.navegacion {
    display: flex;
    gap: 10px;
    margin-bottom: 20px;
}

.navegacion button {
    flex: 1;
}

button {
    border: 1px solid var(--borde);
    border-radius: 10px;
    background: var(--panel);
    color: var(--texto);
    padding: 13px;
    font-size: 15px;
    cursor: pointer;
    touch-action: manipulation;
}

button.seleccionado {
    background: #2563eb;
    border-color: #60a5fa;
}

button:active {
    opacity: 0.7;
}

button:disabled {
    opacity: 0.45;
    cursor: not-allowed;
}

/* ======================================
   PANELES
   ====================================== */

.panel {
    background: var(--panel);
    padding: 18px;
    border-radius: 14px;
    margin-bottom: 16px;
    border: 1px solid var(--borde);
}

.etiqueta {
    font-size: 12px;
    color: var(--secundario);
    letter-spacing: 0.7px;
}

.estado {
    font-size: 27px;
    font-weight: bold;
    margin: 12px 0;
    color: var(--secundario);
}

.estado.girando {
    color: var(--verde);
}

.estado.pausa {
    color: var(--amarillo);
}

.estado.detenido {
    color: var(--rojo);
}

.estado.desconectado {
    color: var(--secundario);
}

/* ======================================
   BOTONES PRINCIPALES
   ====================================== */

.acciones {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 16px;
}

.acciones button {
    padding: 22px 8px;
    font-weight: bold;
    font-size: 16px;
}

.arrancar {
    background: #15803d;
    border-color: #22c55e;
}

.detener {
    background: #b91c1c;
    border-color: #ef4444;
}

.simbolo {
    display: block;
    font-size: 27px;
    margin-bottom: 8px;
}

/* ======================================
   CONTROL PWM
   ====================================== */

.valor {
    color: var(--azul);
    font-size: 30px;
    font-weight: bold;
}

.fila {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
}

input[type="range"] {
    width: 100%;
    accent-color: var(--azul);
    margin: 20px 0;
}

input:disabled {
    opacity: 0.6;
}

.limites {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    color: var(--secundario);
}

/* ======================================
   SELECTORES
   ====================================== */

.selector {
    display: flex;
    gap: 10px;
    margin-top: 12px;
}

.selector button {
    flex: 1;
}

/* ======================================
   TEMPORIZADORES
   ====================================== */

.tiempos {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
}

.tiempo {
    text-align: center;
}

.tiempo .valor {
    display: block;
    margin: 15px 0;
}

.controles-tiempo {
    display: flex;
    gap: 10px;
}

.controles-tiempo button {
    flex: 1;
    font-size: 20px;
}

/* ======================================
   DIAGNOSTICO
   ====================================== */

.dato {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
    padding: 14px 0;
    border-bottom: 1px solid var(--borde);
    font-size: 13px;
}

.dato span:first-child {
    color: var(--secundario);
}

.dato span:last-child {
    text-align: right;
}

.pendiente {
    color: var(--amarillo);
}

.oculto {
    display: none;
}

.aviso {
    color: var(--amarillo);
    font-size: 12px;
    line-height: 1.6;
    text-align: center;
    margin-bottom: 18px;
}

footer {
    text-align: center;
    color: var(--secundario);
    font-size: 11px;
    padding: 20px 0;
    line-height: 1.8;
}

</style>

</head>

<body>

<div class="contenedor">

    <!-- ENCABEZADO -->

    <header class="encabezado">

        <div>
            <h1>DOSIFICADORA</h1>

            <div class="subtitulo">
                CONTROL INDUSTRIAL | V1.3
            </div>
        </div>

        <span id="conexion" class="conexion">
            CONECTANDO...
        </span>

    </header>

    <div class="aviso">
        PROTOTIPO DE CONTROL — PRUEBAS SIN 12 V<br><span id="permiso">Sin control</span><br><button id="tomar" disabled>Solicitar control</button> <button id="liberar" disabled>Liberar control</button><br><span id="mensaje"></span>
    </div>

    <!-- NAVEGACION -->

    <nav class="navegacion">

        <button id="tabControl"
                class="seleccionado"
                onclick="mostrarMenu('control')">
            Control
        </button>

        <button id="tabDiagnostico"
                onclick="mostrarMenu('diagnostico')">
            Diagnóstico
        </button>

    </nav>

    <!-- MENU PRINCIPAL -->

    <section id="control">

        <!-- ESTADO DEL MOTOR -->

        <div class="panel">

            <div class="etiqueta">
                ESTADO DEL FIRMWARE
            </div>

            <div id="estadoMotor" class="estado">
                CONSULTANDO...
            </div>

            <div id="estadoSistema" class="subtitulo">
                Esperando información
            </div>

        </div>

        <!-- BOTONES DE CONTROL -->

        <div class="acciones">

            <button class="arrancar"
                    id="btnArrancar"
                    disabled>

                <span class="simbolo">&#9654;</span>
                ARRANCAR

            </button>

            <button class="detener"
                    id="btnDetener"
                    disabled>

                <span class="simbolo">&#9632;</span>
                DETENER

            </button>

        </div>

        <!-- PWM -->

        <div class="panel">

            <div class="fila">

                <span>CICLO DE TRABAJO PWM</span>

                <span id="valorPWM" class="valor">
                    --
                </span>

            </div>

            <input type="range"
                   id="pwm"
                   min="0"
                   max="100"
                   step="1"
                   value="0"
                   disabled>

            <div class="limites">
                <span>0%</span>
                <span>100%</span>
            </div>

        </div>

        <!-- SENTIDO DE GIRO -->

        <div class="panel">

            <div class="etiqueta">
                SENTIDO DE GIRO
            </div>

            <div class="selector">

                <button id="horario" disabled>
                    &#8635; Horario
                </button>

                <button id="antihorario" disabled>
                    &#8634; Antihorario
                </button>

            </div>

        </div>

        <!-- MODO DE OPERACION -->

        <div class="panel">

            <div class="etiqueta">
                MODO DE OPERACIÓN
            </div>

            <div class="selector">

                <button id="ciclico" disabled>
                    Cíclico
                </button>

                <button id="continuo" disabled>
                    Continuo
                </button>

            </div>

        </div>

        <!-- TEMPORIZADORES -->

        <div id="temporizadores" class="tiempos">

            <div class="panel tiempo">

                <div class="etiqueta">
                    TIEMPO DE GIRO
                </div>

                <span id="valorGiro" class="valor">
                    --
                </span>

                <div class="controles-tiempo">

                    <button disabled>-</button>
                    <button disabled>+</button>

                </div>

            </div>

            <div class="panel tiempo">

                <div class="etiqueta">
                    TIEMPO DE PAUSA
                </div>

                <span id="valorPausa" class="valor">
                    --
                </span>

                <div class="controles-tiempo">

                    <button disabled>-</button>
                    <button disabled>+</button>

                </div>

            </div>

        </div>

    </section>

    <!-- MENU DE DIAGNOSTICO -->

    <section id="diagnostico" class="oculto">

        <div class="panel">

            <h3>Diagnóstico del sistema</h3>

            <div class="dato">
                <span>Microcontrolador</span>
                <span id="diagMicro">--</span>
            </div>

            <div class="dato">
                <span>Firmware</span>
                <span id="diagFirmware">--</span>
            </div>

            <div class="dato">
                <span>Dirección IP</span>
                <span id="diagIP">--</span>
            </div>

            <div class="dato">
                <span>Wi-Fi</span>
                <span id="diagWiFi">--</span>
            </div>

            <div class="dato">
                <span>Clientes Wi-Fi</span>
                <span id="diagClientes">--</span>
            </div>

            <div class="dato">
                <span>Bluetooth BLE</span>
                <span id="diagBLE">--</span>
            </div>

            <div class="dato">
                <span>Memoria Flash física</span>
                <span id="diagFlashTotal">--</span>
            </div>

            <div class="dato">
                <span>Flash del programa</span>
                <span id="diagFlashPrograma">--</span>
            </div>

            <div class="dato">
                <span>RAM libre</span>
                <span id="diagRAM">--</span>
            </div>

            <div class="dato">
                <span>RAM libre mínima</span>
                <span id="diagRAMMinima">--</span>
            </div>

            <div class="dato">
                <span>Tiempo de funcionamiento</span>
                <span id="diagTiempo">--</span>
            </div>

            <div class="dato">
                <span>Pantalla LCD</span>
                <span class="pendiente">Pendiente</span>
            </div>

            <div class="dato">
                <span>Encoder</span>
                <span class="pendiente">Pendiente</span>
            </div>

            <div class="dato">
                <span>Sensores</span>
                <span class="pendiente">Pendientes</span>
            </div>

            <div class="dato">
                <span>Heartbeat de seguridad</span>
                <span id="diagHB">--</span>
            </div>

        </div>

    </section>

    <footer>

        DOSIFICADORA DE BOMBONES<br>

        INGENIERÍA MECATRÓNICA | V1.3<br><br>

        <span id="ultimaActualizacion">
            Esperando comunicación...
        </span>

    </footer>

</div>

<script>

// ======================================
// CONFIGURACION DE COMUNICACION
// ======================================

const INTERVALO_ESTADO = 1000;
const INTERVALO_DIAGNOSTICO = 5000;
const TIMEOUT_HTTP = 2000;

let ultimaConsultaDiagnostico = 0;

// ======================================
// FUNCIONES AUXILIARES
// ======================================

function elemento(id) {

    return document.getElementById(id);
}

function escribir(id, valor) {

    elemento(id).textContent = valor;
}

function convertirKiB(bytes) {

    return (bytes / 1024).toFixed(2) + " KiB";
}

function convertirMiB(bytes) {

    return (bytes / 1048576).toFixed(2) + " MiB";
}

function formatearTiempo(segundos) {

    const horas = Math.floor(segundos / 3600);

    const minutos = Math.floor(
        (segundos % 3600) / 60
    );

    const restantes = segundos % 60;

    return horas + " h " +
           minutos + " min " +
           restantes + " s";
}

// ======================================
// NAVEGACION
// ======================================

function mostrarMenu(menu) {

    elemento("control")
        .classList.toggle(
            "oculto",
            menu !== "control"
        );

    elemento("diagnostico")
        .classList.toggle(
            "oculto",
            menu !== "diagnostico"
        );

    elemento("tabControl")
        .classList.toggle(
            "seleccionado",
            menu === "control"
        );

    elemento("tabDiagnostico")
        .classList.toggle(
            "seleccionado",
            menu === "diagnostico"
        );
}

// ======================================
// CONSULTAS HTTP
// ======================================

async function consultarAPI(ruta) {

    const controlador = new AbortController();

    const temporizador = setTimeout(
        function() {
            controlador.abort();
        },
        TIMEOUT_HTTP
    );

    try {

        const respuesta = await fetch(ruta, {

            method: "GET",

            cache: "no-store",

            signal: controlador.signal

        });

        if (!respuesta.ok) {

            throw new Error(
                "Error HTTP: " + respuesta.status
            );
        }

        return await respuesta.json();

    } finally {

        clearTimeout(temporizador);
    }
}

// ======================================
// INDICADOR DE CONEXION
// ======================================

function actualizarConexion(conectado) {

    const indicador = elemento("conexion");

    indicador.textContent = conectado
        ? "CONECTADO"
        : "SIN CONEXIÓN";

    indicador.className = conectado
        ? "conexion conectado"
        : "conexion desconectado";
}

// ======================================
// ACTUALIZACION DEL ESTADO REAL
// ======================================

function mostrarEstado(datos) {

    const estadosValidos = [
        "DETENIDO",
        "GIRANDO",
        "PAUSA"
    ];

    if (
        !estadosValidos.includes(datos.estado) ||
        typeof datos.activo !== "boolean" ||
        !Number.isFinite(datos.pwm) ||
        datos.pwm < 0 ||
        datos.pwm > 100 ||
        !Number.isFinite(datos.giro) ||
        !Number.isFinite(datos.pausa) ||
        !["HORARIO", "ANTIHORARIO"].includes(datos.direccion) ||
        !["CICLICO", "CONTINUO"].includes(datos.modo)
    ) {

        throw new Error("Estado no válido");
    }

    const estado = elemento("estadoMotor");

    estado.textContent = datos.estado;

    estado.className =
        "estado " + datos.estado.toLowerCase();

    escribir(
        "estadoSistema",
        datos.activo
            ? "SISTEMA ACTIVO"
            : "SISTEMA DESACTIVADO"
    );

    // PWM

    escribir("valorPWM", datos.pwm + "%");

    if (!editandoPWM) elemento("pwm").value = datos.pwm;

    // Direccion

    elemento("horario")
        .classList.toggle(
            "seleccionado",
            datos.direccion === "HORARIO"
        );

    elemento("antihorario")
        .classList.toggle(
            "seleccionado",
            datos.direccion === "ANTIHORARIO"
        );

    // Modo de operacion

    elemento("ciclico")
        .classList.toggle(
            "seleccionado",
            datos.modo === "CICLICO"
        );

    elemento("continuo")
        .classList.toggle(
            "seleccionado",
            datos.modo === "CONTINUO"
        );

    elemento("temporizadores")
        .classList.toggle(
            "oculto",
            datos.modo === "CONTINUO"
        );

    // Conversion de milisegundos a segundos

    escribir(
        "valorGiro",
        (datos.giro / 1000) + " s"
    );

    escribir(
        "valorPausa",
        (datos.pausa / 1000) + " s"
    );

    actualizarConexion(true);

    escribir(
        "ultimaActualizacion",
        "Última actualización: " +
        new Date().toLocaleTimeString()
    );
}

// ======================================
// ACTUALIZACION DEL DIAGNOSTICO
// ======================================

function mostrarDiagnostico(datos) {

    escribir("diagMicro", datos.microcontrolador);

    escribir("diagFirmware", "V" + datos.firmware);

    escribir("diagIP", datos.ip);

    escribir(
        "diagWiFi",
        datos.wifi_activo ? "Activo" : "Inactivo"
    );

    escribir(
        "diagClientes",
        datos.clientes_wifi
    );

    escribir(
        "diagBLE",
        datos.bluetooth_conectado
            ? "Conectado"
            : "Sin cliente"
    );

    escribir(
        "diagFlashTotal",
        convertirMiB(datos.flash_total)
    );

    escribir(
        "diagFlashPrograma",
        convertirMiB(datos.flash_programa)
    );

    escribir(
        "diagRAM",
        convertirKiB(datos.ram_libre)
    );

    escribir(
        "diagRAMMinima",
        convertirKiB(datos.ram_minima)
    );

    escribir(
        "diagTiempo",
        formatearTiempo(datos.tiempo_activo)
    );
}

// ======================================
// INVALIDAR DIAGNOSTICO
// ======================================

function limpiarDiagnostico() {

    const campos = [
        "diagMicro",
        "diagFirmware",
        "diagIP",
        "diagWiFi",
        "diagClientes",
        "diagBLE",
        "diagFlashTotal",
        "diagFlashPrograma",
        "diagRAM",
        "diagRAMMinima",
        "diagTiempo"
    ];

    campos.forEach(function(id) {

        escribir(id, "--");
    });
}

// ======================================
// PERDIDA DE COMUNICACION
// ======================================

function mostrarDesconexion() {

    actualizarConexion(false);

    const estado = elemento("estadoMotor");

    estado.textContent = "SIN CONEXIÓN";

    estado.className = "estado desconectado";

    escribir(
        "estadoSistema",
        "Estado del motor desconocido"
    );

    escribir("valorPWM", "--");

    elemento("pwm").value = 0;

    escribir("valorGiro", "--");

    escribir("valorPausa", "--");

    const selectores = [
        "horario",
        "antihorario",
        "ciclico",
        "continuo"
    ];

    selectores.forEach(function(id) {

        elemento(id)
            .classList.remove("seleccionado");
    });

    elemento("temporizadores")
        .classList.remove("oculto");

    limpiarDiagnostico();

    ultimaConsultaDiagnostico = 0;

    escribir(
        "ultimaActualizacion",
        "Comunicación interrumpida"
    );
}

// ======================================
// CICLO DE MONITOREO
// ======================================

let token = "", ocupado = false, conectado = false, ultimoEstado = null;
let generacion = 0, editandoPWM = false;
const controles = ["pwm","horario","antihorario","ciclico","continuo"];
const tiempos = Array.from(document.querySelectorAll(".controles-tiempo button"));
function botones() {
    const autorizado = conectado && !!token && !document.hidden;
    const config = autorizado && ultimoEstado && !ultimoEstado.activo && !ocupado;
    elemento("tomar").disabled = !conectado || ocupado || !!token || !ultimoEstado || ultimoEstado.propietario !== "NINGUNO" || document.hidden;
    elemento("liberar").disabled = !autorizado || ocupado;
    elemento("btnArrancar").disabled = !config || ultimoEstado.pwm < 80;
    elemento("btnDetener").disabled = !conectado || ocupado;
    controles.forEach(id => elemento(id).disabled = !config);
    tiempos.forEach(b => b.disabled = !config);
    escribir("permiso", autorizado ? "CONTROL WIFI ACTIVO" : "Sin control local" + (ultimoEstado ? " | Propietario: " + ultimoEstado.propietario : ""));
    escribir("diagHB", autorizado ? "Renovación activa" : "Sin permiso local");
}
function perder() { generacion++; token=""; conectado=false; ultimoEstado=null; editandoPWM=false; mostrarDesconexion(); botones(); }
async function post(c, permiso) {
    const ac = new AbortController(); const timer=setTimeout(()=>ac.abort(),650);
    try {
        const r=await fetch(c==="HEARTBEAT"?"/api/heartbeat":"/api/comando",{method:"POST",cache:"no-store",headers:{"Content-Type":"text/plain","X-Dosificadora":"1","X-Control":permiso},body:c,signal:ac.signal});
        if(!r.ok) throw Error("HTTP "+r.status);
        return await r.text();
    } finally {clearTimeout(timer);}
}
async function accion(c) {
    if(ocupado || document.hidden || !conectado) return;
    ocupado=true; botones(); const g=generacion;
    try {
        const r=await post(c,token);
        if(g!==generacion)return;
        escribir("mensaje",r.split(";")[0]);
        if(c==="CONTROL:TAKE" && r.startsWith("OK:")) {const m=r.match(/;TOKEN=([a-f0-9]{32})$/);if(!m)throw Error("Token ausente");token=m[1];}
        if(c==="STOP" || c==="CONTROL:RELEASE" || r.startsWith("ERROR:SIN_AUTORIZACION") || r.startsWith("ERROR:HEARTBEAT"))token="";
        if(r.startsWith("OK:")) { const e=await consultarAPI("/api/status");if(g===generacion){ultimoEstado=e;mostrarEstado(e);} }
    } catch(e){perder();escribir("mensaje","Comunicación interrumpida; no se confirma el comando");}
    finally {ocupado=false;botones();}
}
elemento("tomar").onclick=()=>accion("CONTROL:TAKE");
elemento("liberar").onclick=()=>accion("CONTROL:RELEASE");
elemento("btnArrancar").onclick=()=>accion("START");
elemento("btnDetener").onclick=()=>accion(token ? "HALT" : "STOP");
elemento("horario").onclick=()=>accion("DIR:CW");elemento("antihorario").onclick=()=>accion("DIR:CCW");
elemento("ciclico").onclick=()=>accion("MODO:CICLICO");elemento("continuo").onclick=()=>accion("MODO:CONTINUO");
["GIRO:DOWN","GIRO:UP","PAUSA:DOWN","PAUSA:UP"].forEach((c,i)=>tiempos[i].onclick=()=>accion(c));
elemento("pwm").oninput=()=>{editandoPWM=true;escribir("valorPWM",elemento("pwm").value+"%");};
elemento("pwm").onchange=()=>{editandoPWM=false;accion("PWM:"+elemento("pwm").value);};
let hbOcupado=false;
async function cicloHB(){
    if(token && !document.hidden && !hbOcupado){
        hbOcupado=true;const g=generacion;const t=token;
        try{const r=await post("HEARTBEAT",t);if(g===generacion && token===t && r!=="OK:HEARTBEAT")perder();}
        catch(e){if(g===generacion && token===t)perder();}
        finally{hbOcupado=false;}
    }
    setTimeout(cicloHB,400);
}
async function cicloMonitoreo(){
    if(!document.hidden && !ocupado){const g=generacion;
        try{const e=await consultarAPI("/api/status");if(g===generacion && !document.hidden && !ocupado){ultimoEstado=e;conectado=true;if(token && e.propietario!=="WIFI")token="";mostrarEstado(e);botones();}
            if(!token && Date.now()-ultimaConsultaDiagnostico>=5000){ultimaConsultaDiagnostico=Date.now();const d=await consultarAPI("/api/diagnostico");if(g===generacion)mostrarDiagnostico(d);}
        }catch(e){if(g===generacion)perder();}
    }
    setTimeout(cicloMonitoreo,500);
}
window.addEventListener("offline",perder);
window.addEventListener("pagehide",perder);
document.addEventListener("visibilitychange",()=>{if(document.hidden)perder();});
botones();cicloHB();cicloMonitoreo();
</script>

</body>

</html>

)HTML";

#endif
