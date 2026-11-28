#ifndef COMUNICACION_WIFI_H
#define COMUNICACION_WIFI_H

#include <Arduino.h>

// Inicializa la red Wi-Fi y el servidor web.
bool inicializarWiFi();

// Atiende las solicitudes del servidor web.
void actualizarWiFi();

// Indica si la red Wi-Fi esta habilitada.
bool wifiDisponible();

// Devuelve la cantidad de dispositivos conectados.
uint8_t obtenerClientesWiFi();

// Devuelve la direccion IP de la ESP32.
String obtenerIPWiFi();

#endif