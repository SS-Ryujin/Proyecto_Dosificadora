
#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include "Config.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// Control del motor - V1.2
// ======================================

// ESTADOS DEL MOTOR
enum class EstadoMotor {
    DETENIDO,
    GIRANDO,
    PAUSA
};

// SENTIDOS DE GIRO
enum class SentidoMotor {
    HORARIO,
    ANTIHORARIO
};

// MODOS DE FUNCIONAMIENTO
enum class ModoMotor {
    CICLICO,
    CONTINUO
};

// ======================================
// CONTROL PRINCIPAL
// ======================================

bool inicializarMotor();

void arrancarMotor();
void detenerMotor();
void actualizarMotor();

// ======================================
// CONTROL PWM
// ======================================

void establecerVelocidad(int porcentaje);
void aumentarVelocidad();
void disminuirVelocidad();

int obtenerVelocidad();

// ======================================
// CONTROL DE DIRECCION
// ======================================

void cambiarSentido();
void establecerSentido(SentidoMotor sentido);

SentidoMotor obtenerSentidoMotor();

const char* obtenerSentidoTexto();

// ======================================
// MODOS DE FUNCIONAMIENTO
// ======================================

bool establecerModo(ModoMotor modo);

ModoMotor obtenerModoMotor();

const char* obtenerModoTexto();

// ======================================
// CONTROL DEL TIEMPO DE GIRO
// ======================================

bool establecerTiempoGiro(unsigned long tiempo);
bool aumentarTiempoGiro();
bool disminuirTiempoGiro();

unsigned long obtenerTiempoGiro();

// ======================================
// CONTROL DEL TIEMPO DE PAUSA
// ======================================

bool establecerTiempoPausa(unsigned long tiempo);
bool aumentarTiempoPausa();
bool disminuirTiempoPausa();

unsigned long obtenerTiempoPausa();

// ======================================
// CONSULTA DEL ESTADO
// ======================================

bool sistemaMotorActivo();

EstadoMotor obtenerEstadoMotor();

const char* obtenerEstadoTexto();

#endif
