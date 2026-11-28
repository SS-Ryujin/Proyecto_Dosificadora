
#ifndef COMANDOS_H
#define COMANDOS_H

#include <Arduino.h>
#include "Motor.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// Protocolo de comunicacion - Version 1.0
// ======================================

// Delimitadores del protocolo
constexpr char INICIO_COMANDO = '?';
constexpr char FIN_COMANDO = '*';

// Longitud maxima del comando
constexpr size_t MAX_COMANDO = 64;

// Inicializacion
void inicializarComandos();

// Recepcion de datos por puerto serial
void actualizarComandosSerial();

// Procesamiento de instrucciones
String procesarComando(const String& comando);

// Consulta de informacion
String generarEstado();

// Menu de instrucciones
void mostrarMenuComandos();

#endif
