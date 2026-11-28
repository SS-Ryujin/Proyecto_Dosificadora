
#include "ComunicacionBLE.h"
#include "Config.h"
#include "Comandos.h"
#include "Motor.h"
#include "Seguridad.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>

// ======================================
// DOSIFICADORA DE BOMBONES
// Comunicacion Bluetooth BLE - V1.3
// ======================================

static BLEServer* servidorBLE = nullptr;
static BLECharacteristic* caracteristicaTX = nullptr;

static std::atomic<bool> conectado{false};
static std::atomic<bool> desconexionPendiente{false};
static std::atomic<bool> saturacionPendiente{false};
static std::atomic<bool> reiniciarPublicidad{false};

static std::atomic<uint32_t> sesionBLE{0};

struct MensajeBLE {

    char datos[MAX_COMANDO + 1];

    uint32_t sesion;
};

static QueueHandle_t colaBLE = nullptr;

static String bufferBLE = "";

static bool recibiendoBLE = false;

static uint32_t sesionProcesada = 0;

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

        uint32_t sesionActual = sesionBLE.load();

        if (!conectado.load() || colaBLE == nullptr) {

            return;
        }

        String dato = caracteristica->getValue();

        if (
            dato.length() == 0 ||
            dato.length() > MAX_COMANDO
        ) {

            return;
        }

        MensajeBLE mensaje{};

        dato.toCharArray(
            mensaje.datos,
            sizeof(mensaje.datos)
        );

        mensaje.sesion = sesionActual;

        // Verificar que la conexion siga vigente.

        if (
            !conectado.load() ||
            sesionActual != sesionBLE.load() ||
            desconexionPendiente.load() ||
            saturacionPendiente.load()
        ) {

            return;
        }

        // Notificar saturacion al ciclo principal.

        if (xQueueSend(colaBLE, &mensaje, 0) != pdTRUE) {

            saturacionPendiente.store(true);
        }
    }
};

// ======================================
// INICIALIZACION BLUETOOTH
// ======================================

bool inicializarBluetooth() {

    colaBLE = xQueueCreate(
        10,
        sizeof(MensajeBLE)
    );

    if (colaBLE == nullptr) {

        return false;
    }

    bufferBLE.reserve(MAX_COMANDO);

    BLEDevice::init(NOMBRE_BLUETOOTH);

    servidorBLE = BLEDevice::createServer();

    servidorBLE->setCallbacks(
        new EventosServidor()
    );

    BLEService* servicio =
        servidorBLE->createService(
            BLE_SERVICIO_UUID
        );

    // TX: ESP32 hacia telefono.

    caracteristicaTX = servicio->createCharacteristic(

        BLE_TX_UUID,

        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    caracteristicaTX->addDescriptor(
        new BLE2902()
    );

    // RX: Telefono hacia ESP32.

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

    publicidad->addServiceUUID(
        BLE_SERVICIO_UUID
    );

    publicidad->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.println("Bluetooth BLE inicializado");

    Serial.print("Dispositivo: ");
    Serial.println(NOMBRE_BLUETOOTH);

    Serial.println("Autorizacion BLE: HABILITADA");

    return true;
}

// ======================================
// ESTADO DE CONEXION
// ======================================

bool bluetoothConectado() {

    return conectado.load();
}

// ======================================
// VALIDAR SESION
// ======================================

static bool sesionValida(uint32_t sesion) {

    return conectado.load() &&
           !desconexionPendiente.load() &&
           !saturacionPendiente.load() &&
           sesion == sesionBLE.load();
}

// ======================================
// LIMPIAR RECEPCION
// ======================================

static void limpiarRecepcionBLE() {

    bufferBLE = "";

    recibiendoBLE = false;

    if (colaBLE != nullptr) {

        xQueueReset(colaBLE);
    }
}

// ======================================
// SUPERVISAR EVENTOS DE SEGURIDAD
// ======================================

static bool atenderEventosSeguridadBLE() {

    bool desconexion =
        desconexionPendiente.exchange(false);

    bool saturacion =
        saturacionPendiente.exchange(false);

    uint32_t sesionActual = sesionBLE.load();

    bool cambioSesion =
        sesionActual != sesionProcesada;

    if (desconexion || saturacion || cambioSesion) {

        // Invalidar el permiso de la sesion anterior.
        // No detener si otro canal tiene el control.

        if (desconexion || cambioSesion) {

            notificarDesconexion(
                CanalControl::BLE
            );
        }

        if (saturacion) {

            if (tieneControl(CanalControl::BLE)) {

                detenerPorSeguridad(
                    MotivoDetencion::FALLO_SISTEMA
                );
            }

            Serial.println("BLE: Cola saturada");
        }

        limpiarRecepcionBLE();

        sesionProcesada = sesionActual;

        Serial.println("BLE: Recepcion reiniciada");

        return true;
    }

    return false;
}

// ======================================
// ENVIAR INFORMACION AL TELEFONO
// ======================================

void enviarBluetooth(const String& mensaje) {

    uint32_t sesionActual = sesionBLE.load();

    if (
        !sesionValida(sesionActual) ||
        caracteristicaTX == nullptr
    ) {

        return;
    }

    String trama = mensaje + "\n";

    // Fragmentos compatibles con MTU BLE basico.

    for (size_t i = 0; i < trama.length(); i += 20) {

        if (!sesionValida(sesionActual)) {

            break;
        }

        String fragmento =
            trama.substring(i, i + 20);

        caracteristicaTX->setValue(fragmento);

        caracteristicaTX->notify();
    }
}

// ======================================
// ACTUALIZACION BLUETOOTH
// ======================================

void actualizarBluetooth() {

    static bool conexionAnterior = false;

    // Prioridad: atender eventos de seguridad.

    atenderEventosSeguridadBLE();

    // Reiniciar publicidad tras desconexion.

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

    MensajeBLE mensaje{};

    // Procesar un paquete por iteracion.

    if (xQueueReceive(colaBLE, &mensaje, 0) != pdTRUE) {

        return;
    }

    // Descartar mensajes de sesiones anteriores.

    if (
        !sesionValida(mensaje.sesion) ||
        mensaje.sesion != sesionProcesada
    ) {

        return;
    }

    // ==================================
    // PROCESAMIENTO DE TRAMAS
    // ==================================

    for (
        size_t i = 0;
        i < sizeof(mensaje.datos) &&
        mensaje.datos[i] != '\0';
        i++
    ) {

        if (!sesionValida(mensaje.sesion)) {

            break;
        }

        char caracter = mensaje.datos[i];

        // Inicio de trama.

        if (caracter == INICIO_COMANDO) {

            bufferBLE = "";

            recibiendoBLE = true;

        // Fin de trama.

        } else if (
            caracter == FIN_COMANDO &&
            recibiendoBLE
        ) {

            if (!sesionValida(mensaje.sesion)) {

                break;
            }

            String comando = bufferBLE;

            bufferBLE = "";

            recibiendoBLE = false;

            // Procesar mediante autorizacion BLE.

            String respuesta = procesarComando(

                comando,

                CanalControl::BLE
            );

            // Comprobar si ocurrio una desconexion
            // durante el procesamiento.

            if (
                !sesionValida(mensaje.sesion) ||
                atenderEventosSeguridadBLE()
            ) {

                atenderEventosSeguridadBLE();

                break;
            }

            // Evitar saturar el monitor serial
            // con mensajes repetitivos de heartbeat.

            String comandoNormalizado = comando;

            comandoNormalizado.trim();

            comandoNormalizado.toUpperCase();

            if (comandoNormalizado != "HEARTBEAT") {

                Serial.print("BLE RX: ");
                Serial.println(comando);

                Serial.print("BLE TX: ");
                Serial.println(respuesta);
            }

            enviarBluetooth(respuesta);

            actualizarSeguridad();

        } else if (recibiendoBLE) {

            if (bufferBLE.length() < MAX_COMANDO) {

                bufferBLE += caracter;

            } else {

                bufferBLE = "";

                recibiendoBLE = false;

                enviarBluetooth(
                    "ERROR:COMANDO_LARGO"
                );
            }
        }
    }

    atenderEventosSeguridadBLE();
}
