
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ======================================
// DOSIFICADORA DE BOMBONES
// Configuracion general - V1.2
// ======================================

// PINES DEL MOTOR - L298N
constexpr uint8_t PIN_ENA = 25;
constexpr uint8_t PIN_IN1 = 26;
constexpr uint8_t PIN_IN2 = 27;

// CONFIGURACION PWM
constexpr uint32_t PWM_FRECUENCIA = 1000;
constexpr uint8_t PWM_RESOLUCION = 8;

constexpr int PWM_INICIAL = 50;
constexpr int PWM_MINIMO = 0;
constexpr int PWM_MAXIMO = 100;
constexpr int PWM_INCREMENTO = 10;

// TIEMPOS INICIALES (MILISEGUNDOS)
constexpr unsigned long TIEMPO_GIRO = 5000;
constexpr unsigned long TIEMPO_PAUSA = 1000;

// LIMITES DEL TIEMPO DE GIRO
constexpr unsigned long GIRO_MINIMO = 1000;
constexpr unsigned long GIRO_MAXIMO = 600000;

// LIMITES DEL TIEMPO DE PAUSA
constexpr unsigned long PAUSA_MINIMA = 1000;
constexpr unsigned long PAUSA_MAXIMA = 600000;

// INCREMENTO Y DECREMENTO DE TIEMPO
constexpr unsigned long TIEMPO_INCREMENTO = 1000;

// COMUNICACION SERIAL
constexpr unsigned long SERIAL_BAUDIOS = 115200;

// COMUNICACION BLUETOOTH
constexpr char NOMBRE_BLUETOOTH[] = "Dosificadora_ESP32";

// PANTALLA LCD I2C
constexpr uint8_t LCD_SDA = 21;
constexpr uint8_t LCD_SCL = 22;
constexpr uint8_t LCD_COLUMNAS = 16;
constexpr uint8_t LCD_FILAS = 2;

#endif
