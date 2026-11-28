
#include "Config.h"
#include "Motor.h"
#include "Comandos.h"
#include "ComunicacionBLE.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// Firmware V1.2
// ESP32 - Bluetooth BLE
// ======================================

static bool sistemaListo = false;
static bool bleListo = false;

// ======================================
// CONFIGURACION INICIAL
// ======================================

void setup() {

    Serial.begin(SERIAL_BAUDIOS);

    sistemaListo = inicializarMotor();

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" DOSIFICADORA DE BOMBONES V1.2");
    Serial.println("================================");

    Serial.print("Microcontrolador: ");
    Serial.println(ESP.getChipModel());

    if (!sistemaListo) {

        detenerMotor();

        Serial.println("ERROR: Inicializacion del motor");

        return;
    }

    Serial.println("Motor: OK");
    Serial.println("PWM: OK");

    // Comunicacion serial
    inicializarComandos();

    Serial.println("Comunicacion serial: OK");

    // Comunicacion Bluetooth
    bleListo = inicializarBluetooth();

    if (bleListo) {

        Serial.println("Bluetooth BLE: OK");

    } else {

        detenerMotor();

        Serial.println("ERROR: Bluetooth BLE");

        sistemaListo = false;

        return;
    }

    Serial.println("--------------------------------");
    Serial.println("Sistema listo");
    Serial.println("Esperando instrucciones");
    Serial.println("--------------------------------");

    Serial.println(generarEstado());
}

// ======================================
// CICLO PRINCIPAL
// ======================================

void loop() {

    if (!sistemaListo) {

        delay(100);

        return;
    }

    // Procesamiento de comandos seriales
    actualizarComandosSerial();

    // Procesamiento de comandos Bluetooth
    if (bleListo) {

        actualizarBluetooth();
    }

    // Control automatico del motor
    actualizarMotor();

    // FUTURAS IMPLEMENTACIONES:
    // actualizarPantalla();
    // actualizarSensores();
    // actualizarEncoder();

    delay(1);
}
