
#ifndef SEGURIDAD_H
#define SEGURIDAD_H

#include <Arduino.h>

// ======================================
// CONFIGURACION DEL HEARTBEAT
// ======================================

// Intervalo esperado entre señales de vida
constexpr uint32_t HEARTBEAT_INTERVALO_MS = 500;

// Tiempo maximo permitido sin señal de vida
constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 1500;

// ======================================
// CANALES DE COMUNICACION
// ======================================

enum class CanalControl : uint8_t {

    NINGUNO = 0,
    USB,
    BLE,
    WIFI

};

// ======================================
// MOTIVOS DE DETENCION
// ======================================

enum class MotivoDetencion : uint8_t {

    NINGUNO = 0,
    INICIALIZACION,
    PARO_MANUAL,
    HEARTBEAT_EXPIRADO,
    DESCONEXION,
    LIBERACION_CONTROL,
    FALLO_SISTEMA

};

// ======================================
// INICIALIZACION Y SUPERVISION
// ======================================

void inicializarSeguridad();

void actualizarSeguridad();

// ======================================
// ADMINISTRACION DEL CONTROL
// ======================================

bool solicitarControl(CanalControl canal);

bool liberarControl(CanalControl canal);

bool tieneControl(CanalControl canal);

CanalControl obtenerPropietarioControl();

const char* obtenerPropietarioTexto();

// ======================================
// HEARTBEAT
// ======================================

bool registrarHeartbeat(CanalControl canal);

bool heartbeatVigente();

uint32_t obtenerTiempoSinHeartbeat();

// ======================================
// AUTORIZACION DE OPERACIONES
// ======================================

bool autorizarArranque(CanalControl canal);

bool autorizarConfiguracion(CanalControl canal);

// ======================================
// DETENCION DEL SISTEMA
// ======================================

void detenerPorSeguridad(MotivoDetencion motivo);

void notificarDesconexion(CanalControl canal);

// ======================================
// DIAGNOSTICO
// ======================================

MotivoDetencion obtenerMotivoDetencion();

const char* obtenerMotivoDetencionTexto();

bool seguridadInicializada();

#endif
