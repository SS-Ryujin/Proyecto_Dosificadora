
#include "Comandos.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// Procesamiento de comandos - V1.2
// ======================================

static String bufferComando = "";
static bool recibiendo = false;

// ======================================
// GENERACION DEL ESTADO
// ======================================

String generarEstado() {

    String respuesta = "STATUS";

    respuesta += ";ACTIVO=";
    respuesta += sistemaMotorActivo() ? "SI" : "NO";

    respuesta += ";ESTADO=";
    respuesta += obtenerEstadoTexto();

    respuesta += ";PWM=";
    respuesta += String(obtenerVelocidad());

    respuesta += ";DIR=";
    respuesta += obtenerSentidoTexto();

    respuesta += ";MODO=";
    respuesta += obtenerModoTexto();

    respuesta += ";GIRO=";
    respuesta += String(obtenerTiempoGiro());

    respuesta += ";PAUSA=";
    respuesta += String(obtenerTiempoPausa());

    return respuesta;
}

// ======================================
// MENU DE COMANDOS
// ======================================

void mostrarMenuComandos() {

    Serial.println();
    Serial.println("================================");
    Serial.println("   DOSIFICADORA DE BOMBONES");
    Serial.println("         VERSION 1.2");
    Serial.println("================================");

    Serial.println("?START*          Arrancar motor");
    Serial.println("?STOP*           Detener motor");

    Serial.println("?PWM:80*         Establecer PWM");
    Serial.println("?PWM:UP*         Aumentar PWM");
    Serial.println("?PWM:DOWN*       Disminuir PWM");

    Serial.println("?DIR:CW*         Giro horario");
    Serial.println("?DIR:CCW*        Giro antihorario");

    Serial.println("?MODO:CICLICO*   Modo ciclico");
    Serial.println("?MODO:CONTINUO*  Modo continuo");

    Serial.println("?GIRO:10000*     Tiempo de giro");
    Serial.println("?GIRO:UP*        Aumentar giro");
    Serial.println("?GIRO:DOWN*      Disminuir giro");

    Serial.println("?PAUSA:3000*     Tiempo de pausa");
    Serial.println("?PAUSA:UP*       Aumentar pausa");
    Serial.println("?PAUSA:DOWN*     Disminuir pausa");

    Serial.println("?STATUS*         Consultar estado");
    Serial.println("?MENU*           Consultar comandos");

    Serial.println("================================");
    Serial.println("Tiempos expresados en milisegundos");
    Serial.println("Sistema listo para recibir datos");
    Serial.println();
}

// ======================================
// INICIALIZACION
// ======================================

void inicializarComandos() {

    bufferComando.reserve(MAX_COMANDO);

    bufferComando = "";
    recibiendo = false;

    mostrarMenuComandos();
}

// ======================================
// VALIDACION NUMERICA
// ======================================

static bool esNumero(
    const String& valor,
    size_t maxDigitos
) {

    if (valor.length() == 0 ||
        valor.length() > maxDigitos) {
        return false;
    }

    for (size_t i = 0; i < valor.length(); i++) {

        if (valor[i] < '0' || valor[i] > '9') {
            return false;
        }
    }

    return true;
}

// ======================================
// PROCESAMIENTO DE COMANDOS
// ======================================

String procesarComando(const String& comando) {

    String cmd = comando;

    cmd.trim();
    cmd.toUpperCase();

    // ==================================
    // ARRANQUE
    // ==================================

    if (cmd == "START") {

        arrancarMotor();

        if (!sistemaMotorActivo()) {
            return "ERROR:ARRANQUE_RECHAZADO";
        }

        return "OK:START;" + generarEstado();
    }

    // ==================================
    // DETENCION
    // ==================================

    if (cmd == "STOP") {

        detenerMotor();

        return "OK:STOP;" + generarEstado();
    }

    // ==================================
    // CONTROL PWM
    // ==================================

    if (cmd == "PWM:UP") {

        aumentarVelocidad();

        return "OK:PWM;" + generarEstado();
    }

    if (cmd == "PWM:DOWN") {

        disminuirVelocidad();

        return "OK:PWM;" + generarEstado();
    }

    if (cmd.startsWith("PWM:")) {

        String valor = cmd.substring(4);

        if (!esNumero(valor, 3)) {
            return "ERROR:VALOR_INVALIDO";
        }

        int porcentaje = valor.toInt();

        if (porcentaje < PWM_MINIMO ||
            porcentaje > PWM_MAXIMO) {

            return "ERROR:PWM_FUERA_DE_RANGO";
        }

        establecerVelocidad(porcentaje);

        return "OK:PWM;" + generarEstado();
    }

    // ==================================
    // DIRECCION DEL MOTOR
    // ==================================

    if (cmd == "DIR:CW") {

        establecerSentido(SentidoMotor::HORARIO);

        return "OK:DIR;" + generarEstado();
    }

    if (cmd == "DIR:CCW") {

        establecerSentido(SentidoMotor::ANTIHORARIO);

        return "OK:DIR;" + generarEstado();
    }

    // ==================================
    // MODOS DE FUNCIONAMIENTO
    // ==================================

    if (cmd.startsWith("MODO:")) {

        if (sistemaMotorActivo()) {
            return "ERROR:SISTEMA_ACTIVO";
        }

        if (cmd == "MODO:CICLICO") {

            establecerModo(ModoMotor::CICLICO);

            return "OK:MODO;" + generarEstado();
        }

        if (cmd == "MODO:CONTINUO") {

            establecerModo(ModoMotor::CONTINUO);

            return "OK:MODO;" + generarEstado();
        }

        return "ERROR:MODO_INVALIDO";
    }

    // ==================================
    // TIEMPO DE GIRO
    // ==================================

    if (cmd.startsWith("GIRO:")) {

        if (sistemaMotorActivo()) {
            return "ERROR:SISTEMA_ACTIVO";
        }

        bool resultado = false;

        if (cmd == "GIRO:UP") {

            resultado = aumentarTiempoGiro();

        } else if (cmd == "GIRO:DOWN") {

            resultado = disminuirTiempoGiro();

        } else {

            String valor = cmd.substring(5);

            if (!esNumero(valor, 6)) {
                return "ERROR:VALOR_INVALIDO";
            }

            unsigned long tiempo = valor.toInt();

            resultado = establecerTiempoGiro(tiempo);
        }

        if (!resultado) {
            return "ERROR:GIRO_FUERA_DE_RANGO";
        }

        return "OK:GIRO;" + generarEstado();
    }

    // ==================================
    // TIEMPO DE PAUSA
    // ==================================

    if (cmd.startsWith("PAUSA:")) {

        if (sistemaMotorActivo()) {
            return "ERROR:SISTEMA_ACTIVO";
        }

        bool resultado = false;

        if (cmd == "PAUSA:UP") {

            resultado = aumentarTiempoPausa();

        } else if (cmd == "PAUSA:DOWN") {

            resultado = disminuirTiempoPausa();

        } else {

            String valor = cmd.substring(6);

            if (!esNumero(valor, 6)) {
                return "ERROR:VALOR_INVALIDO";
            }

            unsigned long tiempo = valor.toInt();

            resultado = establecerTiempoPausa(tiempo);
        }

        if (!resultado) {
            return "ERROR:PAUSA_FUERA_DE_RANGO";
        }

        return "OK:PAUSA;" + generarEstado();
    }

    // ==================================
    // CONSULTA DE ESTADO
    // ==================================

    if (cmd == "STATUS") {

        return generarEstado();
    }

    // ==================================
    // CONSULTA DE COMANDOS
    // ==================================

    if (cmd == "MENU") {

        return String("COMANDOS:") +
            "START,STOP," +
            "PWM:0-100,PWM:UP,PWM:DOWN," +
            "DIR:CW,DIR:CCW," +
            "MODO:CICLICO,MODO:CONTINUO," +
            "GIRO:1000-600000,GIRO:UP,GIRO:DOWN," +
            "PAUSA:1000-600000,PAUSA:UP,PAUSA:DOWN," +
            "STATUS,MENU";
    }

    return "ERROR:COMANDO_DESCONOCIDO";
}

// ======================================
// RECEPCION POR PUERTO SERIAL
// ======================================

void actualizarComandosSerial() {

    size_t procesados = 0;

    while (Serial.available() > 0 &&
           procesados < 64) {

        char caracter = Serial.read();

        procesados++;

        if (caracter == INICIO_COMANDO) {

            bufferComando = "";
            recibiendo = true;

        } else if (
            caracter == FIN_COMANDO && recibiendo
        ) {

            String respuesta =
                procesarComando(bufferComando);

            Serial.println(respuesta);

            bufferComando = "";
            recibiendo = false;

        } else if (recibiendo) {

            if (bufferComando.length() < MAX_COMANDO) {

                bufferComando += caracter;

            } else {

                bufferComando = "";
                recibiendo = false;

                Serial.println("ERROR:COMANDO_LARGO");
            }
        }
    }
}
