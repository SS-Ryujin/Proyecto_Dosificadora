
#include "ComunicacionBLE.h"
#include "Config.h"
#include "Comandos.h"
#include "Motor.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>

// ======================================
// DOSIFICADORA DE BOMBONES
// Comunicacion Bluetooth BLE - V1.0
// ======================================

static BLEServer* servidorBLE = nullptr;
static BLECharacteristic* caracteristicaTX = nullptr;

static std::atomic<bool> conectado{false};
static std::atomic<bool> desconexionPendiente{false};
static std::atomic<bool> reiniciarPublicidad{false};

static std::atomic<uint32_t> sesionBLE{0};

struct MensajeBLE {
    char datos[65];
    uint32_t sesion;
};

static QueueHandle_t colaBLE = nullptr;

static String bufferBLE = "";
static bool recibiendoBLE = false;

// ======================================
// EVENTOS DE CONEXION
// ======================================

class EventosServidor : public BLEServerCallbacks {

    void onConnect(BLEServer* servidor) override {
        sesionBLE.fetch_add(1);
        conectado.store(true);
    }

    void onDisconnect(BLEServer* servidor) override {
        conectado.store(false);
        sesionBLE.fetch_add(1);

        desconexionPendiente.store(true);
        reiniciarPublicidad.store(true);
    }
};

// ======================================
// RECEPCION DE DATOS
// ======================================

class EventosRecepcion : public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* caracteristica) override {

        String dato = caracteristica->getValue();

        if (!conectado.load() || colaBLE == nullptr) {
            return;
        }

        if (dato.length() == 0 || dato.length() > 64) {
            return;
        }

        MensajeBLE mensaje{};

        dato.toCharArray(
            mensaje.datos,
            sizeof(mensaje.datos)
        );

        mensaje.sesion = sesionBLE.load();

        // Si se satura la cola, detener por seguridad.
        if (xQueueSend(colaBLE, &mensaje, 0) != pdTRUE) {
            desconexionPendiente.store(true);
        }
    }
};

// ======================================
// INICIALIZACION BLUETOOTH
// ======================================

bool inicializarBluetooth() {

    colaBLE = xQueueCreate(10, sizeof(MensajeBLE));

    if (colaBLE == nullptr) {
        return false;
    }

    bufferBLE.reserve(64);

    BLEDevice::init(NOMBRE_BLUETOOTH);

    servidorBLE = BLEDevice::createServer();

    servidorBLE->setCallbacks(
        new EventosServidor()
    );

    BLEService* servicio =
        servidorBLE->createService(BLE_SERVICIO_UUID);

    // TX: ESP32 hacia telefono
    caracteristicaTX = servicio->createCharacteristic(
        BLE_TX_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    caracteristicaTX->addDescriptor(new BLE2902());

    // RX: Telefono hacia ESP32
    BLECharacteristic* caracteristicaRX =
        servicio->createCharacteristic(
            BLE_RX_UUID,
            BLECharacteristic::PROPERTY_WRITE
        );

    caracteristicaRX->setCallbacks(
        new EventosRecepcion()
    );

    servicio->start();

    BLEAdvertising* publicidad =
        BLEDevice::getAdvertising();

    publicidad->addServiceUUID(BLE_SERVICIO_UUID);
    publicidad->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.println("Bluetooth BLE inicializado");
    Serial.print("Dispositivo: ");
    Serial.println(NOMBRE_BLUETOOTH);

    return true;
}

// ======================================
// ESTADO DE CONEXION
// ======================================

bool bluetoothConectado() {
    return conectado.load();
}

// ======================================
// ENVIAR INFORMACION AL TELEFONO
// ======================================

void enviarBluetooth(const String& mensaje) {

    if (!conectado.load() || caracteristicaTX == nullptr) {
        return;
    }

    String trama = mensaje + "\n";

    // Fragmentos compatibles con MTU BLE basico.
    for (size_t i = 0; i < trama.length(); i += 20) {

        if (!conectado.load()) {
            break;
        }

        String fragmento = trama.substring(i, i + 20);

        caracteristicaTX->setValue(fragmento);
        caracteristicaTX->notify();
    }
}

// ======================================
// ACTUALIZACION BLUETOOTH
// ======================================

void actualizarBluetooth() {

    static bool conexionAnterior = false;

    // Prioridad: detener ante desconexion o saturacion.
    if (desconexionPendiente.exchange(false)) {

        detenerMotor();

        bufferBLE = "";
        recibiendoBLE = false;

        if (colaBLE != nullptr) {
            xQueueReset(colaBLE);
        }

        Serial.println("BLE: Motor detenido por seguridad");
    }

    if (reiniciarPublicidad.exchange(false)) {

        if (!conectado.load()) {
            BLEDevice::startAdvertising();
        }
    }

    bool conexionActual = conectado.load();

    if (conexionActual != conexionAnterior) {

        if (conexionActual) {
            Serial.println("BLE: Telefono conectado");
        } else {
            Serial.println("BLE: Telefono desconectado");
        }

        conexionAnterior = conexionActual;
    }

    if (!conexionActual || colaBLE == nullptr) {
        return;
    }

    MensajeBLE mensaje;

    // Procesar un paquete por iteracion.
    if (xQueueReceive(colaBLE, &mensaje, 0) != pdTRUE) {
        return;
    }

    // Descartar mensajes de conexiones anteriores.
    if (mensaje.sesion != sesionBLE.load()) {
        return;
    }

    for (size_t i = 0; mensaje.datos[i] != '\0'; i++) {

        if (!conectado.load() ||
            desconexionPendiente.load() ||
            mensaje.sesion != sesionBLE.load()) {
            break;
        }

        char caracter = mensaje.datos[i];

        if (caracter == '?') {

            bufferBLE = "";
            recibiendoBLE = true;

        } else if (caracter == '*' && recibiendoBLE) {

            String respuesta = procesarComando(bufferBLE);

            Serial.print("BLE RX: ");
            Serial.println(bufferBLE);

            Serial.print("BLE TX: ");
            Serial.println(respuesta);

            enviarBluetooth(respuesta);

            bufferBLE = "";
            recibiendoBLE = false;

        } else if (recibiendoBLE) {

            if (bufferBLE.length() < 64) {

                bufferBLE += caracter;

            } else {

                bufferBLE = "";
                recibiendoBLE = false;

                enviarBluetooth("ERROR:COMANDO_LARGO");
            }
        }
    }
}
