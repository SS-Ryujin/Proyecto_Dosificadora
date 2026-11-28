
#include "Config.h"
#include "Motor.h"
#include "Comandos.h"
#include "ComunicacionBLE.h"
#include "ComunicacionWiFi.h"
#include "Seguridad.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// FIRMWARE V1.3
// ======================================

static bool sistemaListo = false;
static bool bleListo = false;
static bool wifiListo = false;

// ======================================
// INICIALIZACION DEL SISTEMA
// ======================================

void setup() {

    Serial.begin(SERIAL_BAUDIOS);

    // Inicializar el motor en estado detenido.

    sistemaListo = inicializarMotor();

    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" DOSIFICADORA DE BOMBONES V1.3");
    Serial.println("================================");

    Serial.print("Microcontrolador: ");
    Serial.println(ESP.getChipModel());

    // ==================================
    // VERIFICACION DEL MOTOR
    // ==================================

    if (!sistemaListo) {

        detenerMotor();

        Serial.println("ERROR: Inicializacion del motor");

        return;
    }

    Serial.println("Motor: OK");
    Serial.println("PWM: OK");

    // ==================================
    // MODULO DE SEGURIDAD
    // ==================================

    inicializarSeguridad();

    if (!seguridadInicializada()) {

        detenerMotor();

        sistemaListo = false;

        Serial.println("ERROR: Modulo de seguridad");

        return;
    }

    Serial.println("Seguridad: OK");

    // ==================================
    // COMUNICACION SERIAL
    // ==================================

    inicializarComandos();

    Serial.println("Comunicacion serial: OK");

    // ==================================
    // BLUETOOTH BLE
    // ==================================

    bleListo = inicializarBluetooth();

    if (bleListo) {

        Serial.println("Bluetooth BLE: OK");

    } else {

        detenerPorSeguridad(
            MotivoDetencion::FALLO_SISTEMA
        );

        sistemaListo = false;

        Serial.println("ERROR: Bluetooth BLE");

        return;
    }

    // ==================================
    // COMUNICACION WIFI
    // ==================================

    wifiListo = inicializarWiFi();

    if (wifiListo) {

        Serial.println("Comunicacion WiFi: OK");

    } else {

        detenerPorSeguridad(
            MotivoDetencion::FALLO_SISTEMA
        );

        sistemaListo = false;

        Serial.println("ERROR: Comunicacion WiFi");

        return;
    }

    // ==================================
    // DIAGNOSTICO DE MEMORIA
    // ==================================

    Serial.println("--------------------------------");
    Serial.println("DIAGNOSTICO DE MEMORIA");
    Serial.println("--------------------------------");

    Serial.print("Memoria Flash total: ");
    Serial.print(ESP.getFlashChipSize());
    Serial.println(" bytes");

    Serial.print("RAM libre: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    Serial.print("RAM libre minima: ");
    Serial.print(ESP.getMinFreeHeap());
    Serial.println(" bytes");

    // ==================================
    // DIAGNOSTICO DE COMUNICACIONES
    // ==================================

    Serial.println("--------------------------------");

    Serial.print("Direccion IP: ");
    Serial.println(obtenerIPWiFi());

    Serial.print("Clientes WiFi conectados: ");
    Serial.println(obtenerClientesWiFi());

    // ==================================
    // DIAGNOSTICO DE SEGURIDAD
    // ==================================

    Serial.println("--------------------------------");
    Serial.println("DIAGNOSTICO DE SEGURIDAD");
    Serial.println("--------------------------------");

    Serial.print("Modulo de seguridad: ");

    Serial.println(
        seguridadInicializada()
        ? "ACTIVO"
        : "INACTIVO"
    );

    Serial.print("Propietario de control: ");
    Serial.println(obtenerPropietarioTexto());

    Serial.print("Heartbeat: ");

    Serial.println(
        heartbeatVigente()
        ? "VIGENTE"
        : "NO VIGENTE"
    );

    Serial.print("Motivo de detencion: ");
    Serial.println(obtenerMotivoDetencionTexto());

    Serial.print("Timeout Heartbeat: ");
    Serial.print(HEARTBEAT_TIMEOUT_MS);
    Serial.println(" ms");

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

    // Supervisar seguridad.

    actualizarSeguridad();

    // Procesar comunicacion serial.

    actualizarComandosSerial();

    actualizarSeguridad();

    // Procesar Bluetooth.

    if (bleListo) {

        actualizarBluetooth();
    }

    actualizarSeguridad();

    // Actualizar el motor.

    actualizarMotor();

    // Procesar comunicacion WiFi.

    if (wifiListo) {

        actualizarWiFi();
    }

    actualizarSeguridad();

    // ==================================
    // FUTURAS IMPLEMENTACIONES
    // ==================================

    // Actualizar pantalla LCD.
    // Actualizar sensores.
    // Actualizar encoder.
    // Supervisar paro de emergencia.

    delay(1);
}
