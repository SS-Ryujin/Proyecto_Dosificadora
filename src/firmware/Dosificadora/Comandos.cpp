
#include "Comandos.h"
#include "Motor.h"
#include "Seguridad.h"

// ======================================
// VARIABLES INTERNAS
// ======================================

static String bufferComando = "";

static bool recibiendo = false;

// ======================================
// VALIDACION DE NUMEROS
// ======================================

static bool esNumeroValido(
    const String& texto,
    size_t maxDigitos
) {

    if (
        texto.length() == 0 ||
        texto.length() > maxDigitos
    ) {
        return false;
    }

    for (size_t i = 0; i < texto.length(); i++) {

        if (texto[i] < '0' || texto[i] > '9') {

            return false;
        }
    }

    return true;
}

// ======================================
// ESTADO DEL MOTOR
// ======================================

String generarEstado() {

    String estado;

    estado.reserve(192);

    estado = "STATUS";

    estado += ";ACTIVO=";
    estado += sistemaMotorActivo() ? "SI" : "NO";

    estado += ";ESTADO=";
    estado += obtenerEstadoTexto();

    estado += ";PWM=";
    estado += String(obtenerVelocidad());

    estado += ";DIR=";
    estado += obtenerSentidoTexto();

    estado += ";MODO=";
    estado += obtenerModoTexto();

    estado += ";GIRO=";
    estado += String(obtenerTiempoGiro());

    estado += ";PAUSA=";
    estado += String(obtenerTiempoPausa());

    return estado;
}

// ======================================
// ESTADO DE SEGURIDAD
// ======================================

String generarEstadoSeguridad() {

    String estado;

    estado.reserve(192);

    estado = "SEGURIDAD";

    estado += ";MODULO=";
    estado += seguridadInicializada()
        ? "ACTIVO"
        : "INACTIVO";

    estado += ";PROPIETARIO=";
    estado += obtenerPropietarioTexto();

    estado += ";HEARTBEAT=";
    estado += heartbeatVigente()
        ? "VIGENTE"
        : "NO_VIGENTE";

    estado += ";SIN_HEARTBEAT_MS=";
    estado += String(obtenerTiempoSinHeartbeat());

    estado += ";TIMEOUT_MS=";
    estado += String(HEARTBEAT_TIMEOUT_MS);

    estado += ";MOTIVO=";
    estado += obtenerMotivoDetencionTexto();

    return estado;
}

// ======================================
// MENU DE COMANDOS
// ======================================

static const char MENU_COMANDOS[] PROGMEM =

    "\n================================\n"
    "   DOSIFICADORA DE BOMBONES\n"
    "         VERSION 1.3\n"
    "================================\n"
    "CONTROL Y SEGURIDAD\n"
    "?CONTROL:TAKE*     Solicitar control\n"
    "?CONTROL:RELEASE*  Liberar control\n"
    "?CONTROL:STATUS*   Consultar propietario\n"
    "?HEARTBEAT*        Renovar permiso\n"
    "?SEGURIDAD*        Diagnostico\n"
    "--------------------------------\n"
    "CONTROL DEL MOTOR\n"
    "?START*            Arrancar motor\n"
    "?STOP*             Paro general y revocar control\n"
    "?HALT*             Detener conservando control\n"
    "?PWM:80*           Establecer PWM\n"
    "?PWM:UP*           Aumentar PWM\n"
    "?PWM:DOWN*         Disminuir PWM\n"
    "?DIR:CW*           Giro horario\n"
    "?DIR:CCW*          Giro antihorario\n"
    "--------------------------------\n"
    "MODOS DE OPERACION\n"
    "?MODO:CICLICO*     Modo ciclico\n"
    "?MODO:CONTINUO*    Modo continuo\n"
    "--------------------------------\n"
    "TEMPORIZADORES\n"
    "?GIRO:10000*       Tiempo de giro\n"
    "?GIRO:UP*          Aumentar giro\n"
    "?GIRO:DOWN*        Disminuir giro\n"
    "?PAUSA:3000*       Tiempo de pausa\n"
    "?PAUSA:UP*         Aumentar pausa\n"
    "?PAUSA:DOWN*       Disminuir pausa\n"
    "--------------------------------\n"
    "?STATUS*           Consultar estado\n"
    "?MENU*             Consultar comandos\n"
    "================================\n"
    "Tiempos expresados en milisegundos\n"
    "Heartbeat requerido para operar\n"
    "================================\n";

void mostrarMenuComandos() {

    Serial.print(MENU_COMANDOS);
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
// PROCESAMIENTO DE COMANDOS
// ======================================

String procesarComando(
    const String& comando,
    CanalControl canal
) {

    String cmd = comando;

    cmd.trim();

    cmd.toUpperCase();

    if (cmd.length() == 0) {

        return "ERROR:COMANDO_VACIO";
    }

    if (cmd.length() > MAX_COMANDO) {

        return "ERROR:COMANDO_DEMASIADO_LARGO";
    }

    // ==================================
    // PARO MANUAL
    // Disponible desde cualquier canal.
    // ==================================

    if (cmd == "STOP") {

        detenerPorSeguridad(
            MotivoDetencion::PARO_MANUAL
        );

        return "OK:STOP;" + generarEstado();
    }

    // Supervisar permisos antes de
    // procesar cualquier otra orden.

    actualizarSeguridad();

    // ==================================
    // CONSULTAS PUBLICAS
    // ==================================

    if (cmd == "STATUS") {

        return generarEstado();
    }

    if (cmd == "SEGURIDAD") {

        return generarEstadoSeguridad();
    }

    if (cmd == "CONTROL:STATUS") {

        return generarEstadoSeguridad();
    }

    if (cmd == "MENU") {

        return String(MENU_COMANDOS);
    }

    // ==================================
    // VALIDACION DEL CANAL
    // ==================================

    if (canal == CanalControl::NINGUNO) {

        return "ERROR:CANAL_INVALIDO";
    }

    if (!seguridadInicializada()) {

        return "ERROR:SEGURIDAD_NO_INICIALIZADA";
    }

    // ==================================
    // SOLICITAR CONTROL
    // ==================================

    if (cmd == "CONTROL:TAKE") {

        if (!solicitarControl(canal)) {

            return "ERROR:CONTROL_NO_DISPONIBLE";
        }

        return "OK:CONTROL_TAKE;" +
               generarEstadoSeguridad();
    }

    // ==================================
    // LIBERAR CONTROL
    // ==================================

    if (cmd == "CONTROL:RELEASE") {

        if (!liberarControl(canal)) {

            return "ERROR:SIN_AUTORIZACION";
        }

        return "OK:CONTROL_RELEASE;" +
               generarEstadoSeguridad();
    }

    // ==================================
    // HEARTBEAT
    // ==================================

    if (cmd == "HEARTBEAT") {

        if (!registrarHeartbeat(canal)) {

            return "ERROR:HEARTBEAT_RECHAZADO";
        }

        return "OK:HEARTBEAT";
    }

    // ==================================
    // ARRANQUE DEL MOTOR
    // ==================================

    // Paro operativo: conserva el permiso, no renueva heartbeat.
    if (cmd == "HALT") {
        if (!tieneControl(canal)) return "ERROR:SIN_AUTORIZACION";
        if (!heartbeatVigente()) {
            actualizarSeguridad();
            return "ERROR:HEARTBEAT_EXPIRADO";
        }
        detenerMotor();
        return "OK:HALT;" + generarEstado();
    }

    if (cmd == "START") {

        if (!autorizarArranque(canal)) {

            return "ERROR:ARRANQUE_NO_AUTORIZADO";
        }

        arrancarMotor();

        if (!sistemaMotorActivo()) {

            return "ERROR:ARRANQUE_RECHAZADO";
        }

        return "OK:START;" + generarEstado();
    }

    // ==================================
    // IDENTIFICAR COMANDOS DE CONFIGURACION
    // ==================================

    bool esConfiguracion =

        cmd.startsWith("PWM:") ||
        cmd.startsWith("DIR:") ||
        cmd.startsWith("MODO:") ||
        cmd.startsWith("GIRO:") ||
        cmd.startsWith("PAUSA:");

    if (!esConfiguracion) {

        return "ERROR:COMANDO_DESCONOCIDO";
    }

    // ==================================
    // VERIFICAR AUTORIZACION
    // ==================================

    if (!tieneControl(canal)) {

        return "ERROR:SIN_AUTORIZACION";
    }

    if (!heartbeatVigente()) {

        actualizarSeguridad();

        return "ERROR:HEARTBEAT_EXPIRADO";
    }

    if (sistemaMotorActivo()) {

        return "ERROR:SISTEMA_ACTIVO";
    }

    if (!autorizarConfiguracion(canal)) {

        return "ERROR:CONFIGURACION_NO_AUTORIZADA";
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

        if (!esNumeroValido(valor, 3)) {

            return "ERROR:VALOR_INVALIDO";
        }

        int porcentaje = valor.toInt();

        if (porcentaje < 0 || porcentaje > 100) {

            return "ERROR:PWM_FUERA_DE_RANGO";
        }

        establecerVelocidad(porcentaje);

        return "OK:PWM;" + generarEstado();
    }

    // ==================================
    // SENTIDO DE GIRO
    // ==================================

    if (cmd == "DIR:CW") {

        establecerSentido(
            SentidoMotor::HORARIO
        );

        return "OK:DIR;" + generarEstado();
    }

    if (cmd == "DIR:CCW") {

        establecerSentido(
            SentidoMotor::ANTIHORARIO
        );

        return "OK:DIR;" + generarEstado();
    }

    if (cmd.startsWith("DIR:")) {

        return "ERROR:DIRECCION_INVALIDA";
    }

    // ==================================
    // MODO DE OPERACION
    // ==================================

    if (cmd == "MODO:CICLICO") {

        if (!establecerModo(ModoMotor::CICLICO)) {

            return "ERROR:MODO_RECHAZADO";
        }

        return "OK:MODO;" + generarEstado();
    }

    if (cmd == "MODO:CONTINUO") {

        if (!establecerModo(ModoMotor::CONTINUO)) {

            return "ERROR:MODO_RECHAZADO";
        }

        return "OK:MODO;" + generarEstado();
    }

    if (cmd.startsWith("MODO:")) {

        return "ERROR:MODO_INVALIDO";
    }

    // ==================================
    // TIEMPO DE GIRO
    // ==================================

    if (cmd == "GIRO:UP") {

        if (!aumentarTiempoGiro()) {

            return "ERROR:GIRO_FUERA_DE_RANGO";
        }

        return "OK:GIRO;" + generarEstado();
    }

    if (cmd == "GIRO:DOWN") {

        if (!disminuirTiempoGiro()) {

            return "ERROR:GIRO_FUERA_DE_RANGO";
        }

        return "OK:GIRO;" + generarEstado();
    }

    if (cmd.startsWith("GIRO:")) {

        String valor = cmd.substring(5);

        if (!esNumeroValido(valor, 6)) {

            return "ERROR:VALOR_INVALIDO";
        }

        unsigned long tiempo =
            static_cast<unsigned long>(
                valor.toInt()
            );

        if (
            tiempo < GIRO_MINIMO ||
            tiempo > GIRO_MAXIMO
        ) {

            return "ERROR:GIRO_FUERA_DE_RANGO";
        }

        if (!establecerTiempoGiro(tiempo)) {

            return "ERROR:GIRO_RECHAZADO";
        }

        return "OK:GIRO;" + generarEstado();
    }

    // ==================================
    // TIEMPO DE PAUSA
    // ==================================

    if (cmd == "PAUSA:UP") {

        if (!aumentarTiempoPausa()) {

            return "ERROR:PAUSA_FUERA_DE_RANGO";
        }

        return "OK:PAUSA;" + generarEstado();
    }

    if (cmd == "PAUSA:DOWN") {

        if (!disminuirTiempoPausa()) {

            return "ERROR:PAUSA_FUERA_DE_RANGO";
        }

        return "OK:PAUSA;" + generarEstado();
    }

    if (cmd.startsWith("PAUSA:")) {

        String valor = cmd.substring(6);

        if (!esNumeroValido(valor, 6)) {

            return "ERROR:VALOR_INVALIDO";
        }

        unsigned long tiempo =
            static_cast<unsigned long>(
                valor.toInt()
            );

        if (
            tiempo < PAUSA_MINIMA ||
            tiempo > PAUSA_MAXIMA
        ) {

            return "ERROR:PAUSA_FUERA_DE_RANGO";
        }

        if (!establecerTiempoPausa(tiempo)) {

            return "ERROR:PAUSA_RECHAZADA";
        }

        return "OK:PAUSA;" + generarEstado();
    }

    return "ERROR:COMANDO_DESCONOCIDO";
}

// ======================================
// COMUNICACION SERIAL USB
// ======================================

void actualizarComandosSerial() {

    size_t bytesProcesados = 0;

    while (
        Serial.available() > 0 &&
        bytesProcesados < MAX_COMANDO
    ) {

        char caracter =
            static_cast<char>(Serial.read());

        bytesProcesados++;

        // Inicio de una nueva trama.

        if (caracter == INICIO_COMANDO) {

            bufferComando = "";

            recibiendo = true;

            continue;
        }

        if (!recibiendo) {

            continue;
        }

        // Fin de la trama.

        if (caracter == FIN_COMANDO) {

            recibiendo = false;

            String respuesta = procesarComando(
                bufferComando,
                CanalControl::USB
            );

            Serial.println(respuesta);

            bufferComando = "";

            continue;
        }

        // Proteccion contra desbordamiento.

        if (bufferComando.length() >= MAX_COMANDO) {

            bufferComando = "";

            recibiendo = false;

            Serial.println(
                "ERROR:COMANDO_DEMASIADO_LARGO"
            );

            continue;
        }

        bufferComando += caracter;
    }
}
