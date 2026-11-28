
#ifndef COMANDOS_H
#define COMANDOS_H

#include <Arduino.h>
#include "Motor.h"
#include "Seguridad.h"

// ======================================
// CONFIGURACION DEL PROTOCOLO
// ======================================

constexpr char INICIO_COMANDO = '?';
constexpr char FIN_COMANDO = '*';

constexpr size_t MAX_COMANDO = 64;

// ======================================
// INICIALIZACION
// ======================================

void inicializarComandos();

// ======================================
// COMUNICACION SERIAL
// ======================================

void actualizarComandosSerial();

// ======================================
// PROCESAMIENTO DE COMANDOS
// ======================================

String procesarComando(
    const String& comando,
    CanalControl canal
);

// ======================================
// GENERACION DE RESPUESTAS
// ======================================

String generarEstado();

String generarEstadoSeguridad();

// ======================================
// MENU DE COMANDOS
// ======================================

void mostrarMenuComandos();

#endif
