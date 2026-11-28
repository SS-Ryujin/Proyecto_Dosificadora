
#include "Motor.h"

// ======================================
// DOSIFICADORA DE BOMBONES
// Control del motor - V1.2
// ======================================

// VARIABLES INTERNAS

static int velocidad = PWM_INICIAL;

static bool sistemaActivo = false;
static bool pwmInicializado = false;

static EstadoMotor estado = EstadoMotor::DETENIDO;
static SentidoMotor sentido = SentidoMotor::HORARIO;
static ModoMotor modo = ModoMotor::CICLICO;

static unsigned long tiempoGiro = TIEMPO_GIRO;
static unsigned long tiempoPausa = TIEMPO_PAUSA;
static unsigned long tiempoAnterior = 0;

// ======================================
// FUNCIONES INTERNAS
// ======================================

static void aplicarPWM() {

    if (!pwmInicializado) {
        return;
    }

    int pwm = map(velocidad, 0, 100, 0, 255);

    if (sistemaActivo && estado == EstadoMotor::GIRANDO) {
        ledcWrite(PIN_ENA, pwm);
    } else {
        ledcWrite(PIN_ENA, 0);
    }
}

static void aplicarSentido() {

    if (sentido == SentidoMotor::HORARIO) {

        digitalWrite(PIN_IN1, HIGH);
        digitalWrite(PIN_IN2, LOW);

    } else {

        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, HIGH);
    }
}

static void desenergizarMotor() {

    if (pwmInicializado) {
        ledcWrite(PIN_ENA, 0);
    }

    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
}

// ======================================
// INICIALIZACION
// ======================================

bool inicializarMotor() {

    pinMode(PIN_ENA, OUTPUT);
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);

    digitalWrite(PIN_ENA, LOW);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);

    pwmInicializado = ledcAttach(
        PIN_ENA,
        PWM_FRECUENCIA,
        PWM_RESOLUCION
    );

    detenerMotor();

    return pwmInicializado;
}

// ======================================
// ARRANQUE Y DETENCION
// ======================================

void arrancarMotor() {

    if (!pwmInicializado ||
        sistemaActivo ||
        velocidad == 0) {
        return;
    }

    sistemaActivo = true;
    estado = EstadoMotor::GIRANDO;

    aplicarSentido();
    aplicarPWM();

    tiempoAnterior = millis();
}

void detenerMotor() {

    sistemaActivo = false;
    estado = EstadoMotor::DETENIDO;

    desenergizarMotor();
}

// ======================================
// ACTUALIZACION DEL MOTOR
// ======================================

void actualizarMotor() {

    if (!sistemaActivo) {
        return;
    }

    // MODO CONTINUO:
    // El motor permanece girando sin pausas.

    if (modo == ModoMotor::CONTINUO) {
        return;
    }

    // MODO CICLICO:
    // Alterna entre giro y pausa.

    unsigned long tiempoActual = millis();

    if (estado == EstadoMotor::GIRANDO) {

        if (tiempoActual - tiempoAnterior >= tiempoGiro) {

            estado = EstadoMotor::PAUSA;

            desenergizarMotor();

            tiempoAnterior = tiempoActual;
        }

    } else if (estado == EstadoMotor::PAUSA) {

        if (tiempoActual - tiempoAnterior >= tiempoPausa) {

            estado = EstadoMotor::GIRANDO;

            aplicarSentido();
            aplicarPWM();

            tiempoAnterior = tiempoActual;
        }
    }
}

// ======================================
// CONTROL PWM
// ======================================

void establecerVelocidad(int porcentaje) {

    velocidad = constrain(
        porcentaje,
        PWM_MINIMO,
        PWM_MAXIMO
    );

    if (velocidad == 0) {

        detenerMotor();

    } else {

        aplicarPWM();
    }
}

void aumentarVelocidad() {

    establecerVelocidad(
        velocidad + PWM_INCREMENTO
    );
}

void disminuirVelocidad() {

    establecerVelocidad(
        velocidad - PWM_INCREMENTO
    );
}

int obtenerVelocidad() {
    return velocidad;
}

// ======================================
// CONTROL DE DIRECCION
// ======================================

void establecerSentido(SentidoMotor nuevoSentido) {

    if (sentido != nuevoSentido) {

        detenerMotor();

        sentido = nuevoSentido;
    }
}

void cambiarSentido() {

    if (sentido == SentidoMotor::HORARIO) {

        establecerSentido(SentidoMotor::ANTIHORARIO);

    } else {

        establecerSentido(SentidoMotor::HORARIO);
    }
}

SentidoMotor obtenerSentidoMotor() {
    return sentido;
}

const char* obtenerSentidoTexto() {

    if (sentido == SentidoMotor::HORARIO) {
        return "HORARIO";
    }

    return "ANTIHORARIO";
}

// ======================================
// MODOS DE FUNCIONAMIENTO
// ======================================

bool establecerModo(ModoMotor nuevoModo) {

    if (sistemaActivo) {
        return false;
    }

    modo = nuevoModo;

    return true;
}

ModoMotor obtenerModoMotor() {
    return modo;
}

const char* obtenerModoTexto() {

    if (modo == ModoMotor::CONTINUO) {
        return "CONTINUO";
    }

    return "CICLICO";
}

// ======================================
// CONTROL DEL TIEMPO DE GIRO
// ======================================

bool establecerTiempoGiro(unsigned long tiempo) {

    if (sistemaActivo) {
        return false;
    }

    if (tiempo < GIRO_MINIMO ||
        tiempo > GIRO_MAXIMO) {
        return false;
    }

    tiempoGiro = tiempo;

    return true;
}

bool aumentarTiempoGiro() {

    if (tiempoGiro > GIRO_MAXIMO - TIEMPO_INCREMENTO) {
        return false;
    }

    return establecerTiempoGiro(
        tiempoGiro + TIEMPO_INCREMENTO
    );
}

bool disminuirTiempoGiro() {

    if (tiempoGiro < GIRO_MINIMO + TIEMPO_INCREMENTO) {
        return false;
    }

    return establecerTiempoGiro(
        tiempoGiro - TIEMPO_INCREMENTO
    );
}

unsigned long obtenerTiempoGiro() {
    return tiempoGiro;
}

// ======================================
// CONTROL DEL TIEMPO DE PAUSA
// ======================================

bool establecerTiempoPausa(unsigned long tiempo) {

    if (sistemaActivo) {
        return false;
    }

    if (tiempo < PAUSA_MINIMA ||
        tiempo > PAUSA_MAXIMA) {
        return false;
    }

    tiempoPausa = tiempo;

    return true;
}

bool aumentarTiempoPausa() {

    if (tiempoPausa > PAUSA_MAXIMA - TIEMPO_INCREMENTO) {
        return false;
    }

    return establecerTiempoPausa(
        tiempoPausa + TIEMPO_INCREMENTO
    );
}

bool disminuirTiempoPausa() {

    if (tiempoPausa < PAUSA_MINIMA + TIEMPO_INCREMENTO) {
        return false;
    }

    return establecerTiempoPausa(
        tiempoPausa - TIEMPO_INCREMENTO
    );
}

unsigned long obtenerTiempoPausa() {
    return tiempoPausa;
}

// ======================================
// CONSULTA DEL ESTADO
// ======================================

bool sistemaMotorActivo() {
    return sistemaActivo;
}

EstadoMotor obtenerEstadoMotor() {
    return estado;
}

const char* obtenerEstadoTexto() {

    switch (estado) {

        case EstadoMotor::DETENIDO:
            return "DETENIDO";

        case EstadoMotor::GIRANDO:
            return "GIRANDO";

        case EstadoMotor::PAUSA:
            return "PAUSA";

        default:
            return "DESCONOCIDO";
    }
}
