
#ifndef COMUNICACION_BLE_H
#define COMUNICACION_BLE_H

#include <Arduino.h>

// ======================================
// DOSIFICADORA DE BOMBONES
// Comunicacion Bluetooth BLE - V1.0
// ======================================

// Identificadores del servicio Bluetooth
constexpr char BLE_SERVICIO_UUID[] =
    "6e400001-b5a3-f393-e0a9-e50e24dcca9e";

// Caracteristica de recepcion (RX)
constexpr char BLE_RX_UUID[] =
    "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

// Caracteristica de transmision (TX)
constexpr char BLE_TX_UUID[] =
    "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

// Inicializacion Bluetooth
bool inicializarBluetooth();

// Actualizacion de la comunicacion
void actualizarBluetooth();

// Consultar estado de conexion
bool bluetoothConectado();

// Enviar informacion al telefono
void enviarBluetooth(const String& mensaje);

#endif
