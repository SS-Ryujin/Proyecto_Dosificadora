#include <Arduino.h>
#include <LiquidCrystal.h>

// LCD 16x2, interfaz paralela de 4 bits
constexpr uint8_t PIN_RS = 4, PIN_EN = 5;
constexpr uint8_t PIN_D4 = 6, PIN_D5 = 7, PIN_D6 = 15, PIN_D7 = 16;
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);

// Botones conectados entre GPIO y GND (INPUT_PULLUP)
constexpr uint8_t BTN_ARRIBA = 17, BTN_ABAJO = 18;
constexpr uint8_t BTN_OK = 8, BTN_VOLVER = 9;
const uint8_t pinesBotones[4] = {BTN_ARRIBA, BTN_ABAJO, BTN_OK, BTN_VOLVER};

// Solo simulacion: NO se accionan pines de motor o piston.
constexpr uint32_t APERTURA_MS = 2000, CIERRE_MS = 2000;
constexpr uint8_t DOSIS_MIN = 1, DOSIS_MAX = 30;
uint8_t tiempoDosificacion = 5, tiempoTemporal = 5;

// Tipos declarados antes de las funciones para Arduino CLI
enum class Boton : uint8_t { NINGUNO, ARRIBA, ABAJO, OK_BTN, VOLVER };
enum class Pantalla : uint8_t {
  PRINCIPAL, CONFIRMACION, CICLO, SENSORES, ESTADO,
  CONFIGURACION, EDITAR_TIEMPO, VELOCIDAD_MOTOR
};
enum class Fase : uint8_t { INACTIVA, ABRIENDO, DOSIFICANDO, CERRANDO, TERMINADO, CANCELADO };

Boton leerBoton();
void procesarBoton(Boton boton);
void mostrarPantalla();
void actualizarCiclo();
void iniciarCiclo();
void cancelarCiclo();
uint32_t tiempoOperacionSegundos();

Pantalla pantallaActual = Pantalla::PRINCIPAL;
Fase fase = Fase::INACTIVA;
uint8_t opcionActual = 0, sensorActual = 0, estadoActual = 0, opcionConfiguracion = 0;
bool confirmarSi = false;
uint32_t inicioFaseMs = 0, inicioOperacionMs = 0, acumuladoOperacionMs = 0;
bool operacionActiva = false;
uint8_t dosisCiclo = 5;

const char* opciones[] = {"INICIAR MAQ.", "VER SENSORES", "ESTADO SIST.", "CONFIGURACION"};
const char* sensores[] = {"CAPACITIVO", "VIBRACION", "PARO EMERGENCIA", "MOTOR PASO", "PISTON"};
const char* estados[] = {"ESTADO MAQUINA", "TIEMPO OPERACION", "COMUNICACION", "SEGURIDAD", "DIAGNOSTICO"};
const char* opcionesConfig[] = {"TIEMPO DOSIFIC.", "VELOCIDAD MOTOR", "VOLVER"};

bool estadoEstable[4] = {HIGH, HIGH, HIGH, HIGH};
bool ultimaLectura[4] = {HIGH, HIGH, HIGH, HIGH};
uint32_t ultimoCambio[4] = {0, 0, 0, 0};
constexpr uint32_t REBOTE_MS = 40;

Boton leerBoton() {
  Boton evento = Boton::NINGUNO;
  uint32_t ahora = millis();
  for (uint8_t i = 0; i < 4; ++i) {
    bool lectura = digitalRead(pinesBotones[i]);
    if (lectura != ultimaLectura[i]) {
      ultimaLectura[i] = lectura;
      ultimoCambio[i] = ahora;
    }
    if (ahora - ultimoCambio[i] >= REBOTE_MS && lectura != estadoEstable[i]) {
      estadoEstable[i] = lectura;
      if (lectura == LOW && evento == Boton::NINGUNO)
        evento = static_cast<Boton>(i + 1);
    }
  }
  return evento;
}

void escribirLinea(uint8_t fila, const char* texto) {
  lcd.setCursor(0, fila);
  char linea[17];
  snprintf(linea, sizeof(linea), "%-16.16s", texto);
  lcd.print(linea);
}

void mostrarLista(const char* const* items, uint8_t total, uint8_t seleccionado) {
  uint8_t primera = seleccionado >= total - 1 ? total - 2 : seleccionado;
  char linea[17];
  snprintf(linea, sizeof(linea), "%c%.15s", seleccionado == primera ? '>' : ' ', items[primera]);
  escribirLinea(0, linea);
  snprintf(linea, sizeof(linea), "%c%.15s", seleccionado == primera + 1 ? '>' : ' ', items[primera + 1]);
  escribirLinea(1, linea);
}

void iniciarCiclo() {
  dosisCiclo = tiempoDosificacion; // Congelar valor hasta finalizar el ciclo
  fase = Fase::ABRIENDO;
  inicioFaseMs = millis();
  inicioOperacionMs = inicioFaseMs;
  operacionActiva = true;
  pantallaActual = Pantalla::CICLO;
  Serial.println("SIMULACION: orden de apertura (sin actuadores)");
}

void finalizarConteo() {
  if (operacionActiva) {
    acumuladoOperacionMs += millis() - inicioOperacionMs;
    operacionActiva = false;
  }
}

void cancelarCiclo() {
  // Cancelacion SOLO de simulacion; NO es un paro de emergencia real.
  finalizarConteo();
  fase = Fase::CANCELADO;
  Serial.println("SIMULACION: ciclo cancelado");
}

uint32_t tiempoOperacionSegundos() {
  uint32_t total = acumuladoOperacionMs;
  if (operacionActiva) total += millis() - inicioOperacionMs;
  return total / 1000UL;
}

void actualizarCiclo() {
  if (pantallaActual != Pantalla::CICLO) return;
  uint32_t transcurrido = millis() - inicioFaseMs;
  if (fase == Fase::ABRIENDO && transcurrido >= APERTURA_MS) {
    fase = Fase::DOSIFICANDO;
    inicioFaseMs = millis();
    Serial.println("SIMULACION: apertura confirmada virtualmente; dosificando");
    mostrarPantalla();
  } else if (fase == Fase::DOSIFICANDO && transcurrido >= (uint32_t)dosisCiclo * 1000UL) {
    fase = Fase::CERRANDO;
    inicioFaseMs = millis();
    Serial.println("SIMULACION: orden de cierre");
    mostrarPantalla();
  } else if (fase == Fase::CERRANDO && transcurrido >= CIERRE_MS) {
    finalizarConteo();
    fase = Fase::TERMINADO;
    Serial.println("SIMULACION: cierre confirmado virtualmente; ciclo finalizado");
    mostrarPantalla();
  }
}

void mostrarPantalla() {
  char linea[17];
  switch (pantallaActual) {
    case Pantalla::PRINCIPAL:
      mostrarLista(opciones, 4, opcionActual);
      break;
    case Pantalla::CONFIRMACION:
      escribirLinea(0, "INICIAR MAQUINA?");
      escribirLinea(1, confirmarSi ? " NO       >SI" : ">NO        SI");
      break;
    case Pantalla::CICLO:
      switch (fase) {
        case Fase::ABRIENDO:
          escribirLinea(0, "ABRIENDO (SIM)");
          escribirLinea(1, "ESPERE 2 SEG");
          break;
        case Fase::DOSIFICANDO:
          escribirLinea(0, "DOSIFICANDO SIM");
          snprintf(linea, sizeof(linea), "DURACION: %u SEG", (unsigned)dosisCiclo);
          escribirLinea(1, linea);
          break;
        case Fase::CERRANDO:
          escribirLinea(0, "CERRANDO (SIM)");
          escribirLinea(1, "ESPERE 2 SEG");
          break;
        case Fase::TERMINADO:
          escribirLinea(0, "CICLO TERMINADO");
          escribirLinea(1, "OK PARA VOLVER");
          break;
        case Fase::CANCELADO:
          escribirLinea(0, "CICLO CANCELADO");
          escribirLinea(1, "OK PARA VOLVER");
          break;
        default: break;
      }
      break;
    case Pantalla::SENSORES:
      escribirLinea(0, sensores[sensorActual]);
      escribirLinea(1, "SIN CONECTAR");
      break;
    case Pantalla::ESTADO:
      escribirLinea(0, estados[estadoActual]);
      switch (estadoActual) {
        case 0: escribirLinea(1, operacionActiva ? "CICLO SIMULADO" : "EN ESPERA"); break;
        case 1: {
          uint32_t s = tiempoOperacionSegundos();
          snprintf(linea, sizeof(linea), "%02lu:%02lu:%02lu", (unsigned long)(s / 3600UL),
                   (unsigned long)((s / 60UL) % 60UL), (unsigned long)(s % 60UL));
          escribirLinea(1, linea);
          break;
        }
        case 2: escribirLinea(1, "NO VERIFICADA"); break;
        case 3: escribirLinea(1, "NO VERIFICADA"); break;
        case 4: escribirLinea(1, "ESP32-S3 (SIM)"); break;
      }
      break;
    case Pantalla::CONFIGURACION:
      mostrarLista(opcionesConfig, 3, opcionConfiguracion);
      break;
    case Pantalla::EDITAR_TIEMPO:
      escribirLinea(0, "TIEMPO DOSIFIC.");
      snprintf(linea, sizeof(linea), "> %u SEG", (unsigned)tiempoTemporal);
      escribirLinea(1, linea);
      break;
    case Pantalla::VELOCIDAD_MOTOR:
      escribirLinea(0, "VELOCIDAD MOTOR");
      escribirLinea(1, "PENDIENTE");
      break;
  }
}

void procesarBoton(Boton boton) {
  if (boton == Boton::NINGUNO) return;
  switch (pantallaActual) {
    case Pantalla::PRINCIPAL:
      if (boton == Boton::ARRIBA && opcionActual > 0) --opcionActual;
      else if (boton == Boton::ABAJO && opcionActual < 3) ++opcionActual;
      else if (boton == Boton::OK_BTN) {
        if (opcionActual == 0) { confirmarSi = false; pantallaActual = Pantalla::CONFIRMACION; }
        else if (opcionActual == 1) { sensorActual = 0; pantallaActual = Pantalla::SENSORES; }
        else if (opcionActual == 2) { estadoActual = 0; pantallaActual = Pantalla::ESTADO; }
        else { opcionConfiguracion = 0; pantallaActual = Pantalla::CONFIGURACION; }
      }
      break;
    case Pantalla::CONFIRMACION:
      if (boton == Boton::ARRIBA) confirmarSi = false;
      else if (boton == Boton::ABAJO) confirmarSi = true;
      else if (boton == Boton::VOLVER) pantallaActual = Pantalla::PRINCIPAL;
      else if (boton == Boton::OK_BTN) {
        if (confirmarSi) iniciarCiclo();
        else pantallaActual = Pantalla::PRINCIPAL;
      }
      break;
    case Pantalla::CICLO:
      if (fase == Fase::TERMINADO || fase == Fase::CANCELADO) {
        if (boton == Boton::OK_BTN || boton == Boton::VOLVER) pantallaActual = Pantalla::PRINCIPAL;
      } else if (boton == Boton::VOLVER) cancelarCiclo();
      break;
    case Pantalla::SENSORES:
      if (boton == Boton::ARRIBA && sensorActual > 0) --sensorActual;
      else if (boton == Boton::ABAJO && sensorActual < 4) ++sensorActual;
      else if (boton == Boton::VOLVER) pantallaActual = Pantalla::PRINCIPAL;
      break;
    case Pantalla::ESTADO:
      if (boton == Boton::ARRIBA && estadoActual > 0) --estadoActual;
      else if (boton == Boton::ABAJO && estadoActual < 4) ++estadoActual;
      else if (boton == Boton::VOLVER) pantallaActual = Pantalla::PRINCIPAL;
      break;
    case Pantalla::CONFIGURACION:
      if (boton == Boton::ARRIBA && opcionConfiguracion > 0) --opcionConfiguracion;
      else if (boton == Boton::ABAJO && opcionConfiguracion < 2) ++opcionConfiguracion;
      else if (boton == Boton::VOLVER) pantallaActual = Pantalla::PRINCIPAL;
      else if (boton == Boton::OK_BTN) {
        if (opcionConfiguracion == 0) {
          tiempoTemporal = tiempoDosificacion;
          pantallaActual = Pantalla::EDITAR_TIEMPO;
        } else if (opcionConfiguracion == 1) pantallaActual = Pantalla::VELOCIDAD_MOTOR;
        else pantallaActual = Pantalla::PRINCIPAL;
      }
      break;
    case Pantalla::EDITAR_TIEMPO:
      if (boton == Boton::ARRIBA && tiempoTemporal < DOSIS_MAX) ++tiempoTemporal;
      else if (boton == Boton::ABAJO && tiempoTemporal > DOSIS_MIN) --tiempoTemporal;
      else if (boton == Boton::OK_BTN) {
        tiempoDosificacion = tiempoTemporal;
        pantallaActual = Pantalla::CONFIGURACION;
      } else if (boton == Boton::VOLVER) {
        tiempoTemporal = tiempoDosificacion;
        pantallaActual = Pantalla::CONFIGURACION;
      }
      break;
    case Pantalla::VELOCIDAD_MOTOR:
      if (boton == Boton::VOLVER || boton == Boton::OK_BTN)
        pantallaActual = Pantalla::CONFIGURACION;
      break;
  }
  mostrarPantalla();
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 4; ++i) pinMode(pinesBotones[i], INPUT_PULLUP);
  lcd.begin(16, 2);
  escribirLinea(0, "  BIENVENIDOS");
  escribirLinea(1, " SISTEMA ESP32");
  delay(3000); // Solo durante la bienvenida
  mostrarPantalla();
  Serial.println("Dosificadora: simulacion lista");
}

void loop() {
  procesarBoton(leerBoton());
  actualizarCiclo();
  static uint32_t ultimaPantallaMs = 0;
  if (pantallaActual == Pantalla::ESTADO && estadoActual == 1 &&
      millis() - ultimaPantallaMs >= 1000UL) {
    ultimaPantallaMs = millis();
    mostrarPantalla();
  }
}
