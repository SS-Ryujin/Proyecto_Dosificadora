
#include "Seguridad.h"
#include "Motor.h"

// ======================================
// VARIABLES INTERNAS
// ======================================

static bool moduloInicializado = false;

static CanalControl propietario =
    CanalControl::NINGUNO;

static MotivoDetencion motivoDetencion =
    MotivoDetencion::INICIALIZACION;

static uint32_t ultimoHeartbeat = 0;

// ======================================
// INICIALIZACION
// ======================================

void inicializarSeguridad() {

    detenerMotor();

    propietario = CanalControl::NINGUNO;

    motivoDetencion =
        MotivoDetencion::INICIALIZACION;

    ultimoHeartbeat = 0;

    moduloInicializado = true;

    Serial.println("--------------------------------");
    Serial.println("Modulo de seguridad inicializado");
    Serial.println("Propietario: NINGUNO");

    Serial.print("Heartbeat intervalo: ");
    Serial.print(HEARTBEAT_INTERVALO_MS);
    Serial.println(" ms");

    Serial.print("Heartbeat timeout: ");
    Serial.print(HEARTBEAT_TIMEOUT_MS);
    Serial.println(" ms");

    Serial.println("Motor detenido");
    Serial.println("--------------------------------");
}

// ======================================
// SUPERVISION DEL HEARTBEAT
// ======================================

void actualizarSeguridad() {

    if (!moduloInicializado) {
        return;
    }

    if (propietario == CanalControl::NINGUNO) {

        // No se permite operar sin propietario.

        if (sistemaMotorActivo()) {

            detenerPorSeguridad(
                MotivoDetencion::FALLO_SISTEMA
            );
        }

        return;
    }

    if (!heartbeatVigente()) {

        detenerPorSeguridad(
            MotivoDetencion::HEARTBEAT_EXPIRADO
        );
    }
}

// ======================================
// SOLICITAR CONTROL
// ======================================

bool solicitarControl(CanalControl canal) {

    if (!moduloInicializado) {
        return false;
    }

    if (canal == CanalControl::NINGUNO) {
        return false;
    }

    actualizarSeguridad();

    // El propietario actual conserva el control,
    // pero esta solicitud no renueva el heartbeat.

    if (propietario == canal) {
        return heartbeatVigente();
    }

    // Rechazar si otro canal tiene el control.

    if (propietario != CanalControl::NINGUNO) {
        return false;
    }

    // No adquirir control con el motor activo.

    if (sistemaMotorActivo()) {
        return false;
    }

    propietario = canal;

    ultimoHeartbeat = millis();

    Serial.print("Control asignado a: ");
    Serial.println(obtenerPropietarioTexto());

    return true;
}

// ======================================
// LIBERAR CONTROL
// ======================================

bool liberarControl(CanalControl canal) {

    if (!tieneControl(canal)) {
        return false;
    }

    detenerPorSeguridad(
        MotivoDetencion::LIBERACION_CONTROL
    );

    return true;
}

// ======================================
// VERIFICAR PROPIETARIO
// ======================================

bool tieneControl(CanalControl canal) {

    return moduloInicializado &&
           canal != CanalControl::NINGUNO &&
           propietario == canal;
}

CanalControl obtenerPropietarioControl() {

    return propietario;
}

const char* obtenerPropietarioTexto() {

    switch (propietario) {

        case CanalControl::USB:
            return "USB";

        case CanalControl::BLE:
            return "BLE";

        case CanalControl::WIFI:
            return "WIFI";

        default:
            return "NINGUNO";
    }
}

// ======================================
// REGISTRAR HEARTBEAT
// ======================================

bool registrarHeartbeat(CanalControl canal) {

    if (!tieneControl(canal)) {
        return false;
    }

    // Una señal tardia no puede recuperar
    // automaticamente un permiso expirado.

    if (!heartbeatVigente()) {

        detenerPorSeguridad(
            MotivoDetencion::HEARTBEAT_EXPIRADO
        );

        return false;
    }

    ultimoHeartbeat = millis();

    return true;
}

// ======================================
// VERIFICAR VIGENCIA DEL HEARTBEAT
// ======================================

bool heartbeatVigente() {

    if (!moduloInicializado) {
        return false;
    }

    if (propietario == CanalControl::NINGUNO) {
        return false;
    }

    uint32_t tiempoTranscurrido =
        millis() - ultimoHeartbeat;

    return tiempoTranscurrido <
           HEARTBEAT_TIMEOUT_MS;
}

// ======================================
// TIEMPO SIN HEARTBEAT
// ======================================

uint32_t obtenerTiempoSinHeartbeat() {

    if (propietario == CanalControl::NINGUNO) {
        return 0;
    }

    return millis() - ultimoHeartbeat;
}

// ======================================
// AUTORIZAR ARRANQUE
// ======================================

bool autorizarArranque(CanalControl canal) {

    actualizarSeguridad();

    if (!moduloInicializado) {
        return false;
    }

    if (!tieneControl(canal)) {
        return false;
    }

    if (!heartbeatVigente()) {
        return false;
    }

    if (sistemaMotorActivo()) {
        return false;
    }

    if (obtenerVelocidad() <= 0) {
        return false;
    }

    return true;
}

// ======================================
// AUTORIZAR CONFIGURACION
// ======================================

bool autorizarConfiguracion(CanalControl canal) {

    actualizarSeguridad();

    if (!moduloInicializado) {
        return false;
    }

    if (!tieneControl(canal)) {
        return false;
    }

    if (!heartbeatVigente()) {
        return false;
    }

    // Configuracion permitida solamente
    // cuando el sistema esta detenido.

    if (sistemaMotorActivo()) {
        return false;
    }

    return true;
}

// ======================================
// DETENCION POR SEGURIDAD
// ======================================

void detenerPorSeguridad(MotivoDetencion motivo) {

    // Primero ordenar la detencion del motor.

    detenerMotor();

    // Revocar la autorizacion de control.

    propietario = CanalControl::NINGUNO;

    ultimoHeartbeat = 0;

    motivoDetencion = motivo;

    Serial.println("--------------------------------");
    Serial.println("DETENCION DEL SISTEMA");

    Serial.print("Motivo: ");
    Serial.println(obtenerMotivoDetencionTexto());

    Serial.println("Control revocado");
    Serial.println("Propietario: NINGUNO");
    Serial.println("--------------------------------");
}

// ======================================
// NOTIFICAR DESCONEXION
// ======================================

void notificarDesconexion(CanalControl canal) {

    if (!moduloInicializado) {
        return;
    }

    // Solamente detener por desconexion
    // si pertenece al controlador autorizado.

    if (tieneControl(canal)) {

        detenerPorSeguridad(
            MotivoDetencion::DESCONEXION
        );
    }
}

// ======================================
// DIAGNOSTICO
// ======================================

MotivoDetencion obtenerMotivoDetencion() {

    return motivoDetencion;
}

const char* obtenerMotivoDetencionTexto() {

    switch (motivoDetencion) {

        case MotivoDetencion::NINGUNO:
            return "NINGUNO";

        case MotivoDetencion::INICIALIZACION:
            return "INICIALIZACION";

        case MotivoDetencion::PARO_MANUAL:
            return "PARO_MANUAL";

        case MotivoDetencion::HEARTBEAT_EXPIRADO:
            return "HEARTBEAT_EXPIRADO";

        case MotivoDetencion::DESCONEXION:
            return "DESCONEXION";

        case MotivoDetencion::LIBERACION_CONTROL:
            return "LIBERACION_CONTROL";

        case MotivoDetencion::FALLO_SISTEMA:
            return "FALLO_SISTEMA";

        default:
            return "DESCONOCIDO";
    }
}

bool seguridadInicializada() {

    return moduloInicializado;
}
