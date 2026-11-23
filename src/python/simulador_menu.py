
import os
import time

# ========================================
# SIMULADOR LCD 16x2 - ESP32-S3
# DOSIFICADORA DE BOMBONES
# ========================================

opciones = [
    "INICIAR MAQ.",
    "VER SENSORES",
    "ESTADO SIST.",
    "CONFIGURACION"
]

opciones_config = [
    "TIEMPO DOSIFIC.",
    "VELOCIDAD MOTOR",
    "VOLVER"
]

opcion_actual = 0
opcion_config = 0

# Parametros de dosificacion
tiempo_dosificacion = 5
TIEMPO_MIN = 1
TIEMPO_MAX = 30


def limpiar():
    os.system("cls" if os.name == "nt" else "clear")


def lcd(linea1, linea2):
    print("+----------------+")
    print("|" + linea1[:16].ljust(16) + "|")
    print("|" + linea2[:16].ljust(16) + "|")
    print("+----------------+")


def mostrar_lista(elementos, seleccionado):
    primera = min(seleccionado, len(elementos) - 2)

    linea1 = (
        (">" if seleccionado == primera else " ")
        + elementos[primera]
    )

    linea2 = (
        (">" if seleccionado == primera + 1 else " ")
        + elementos[primera + 1]
    )

    lcd(linea1, linea2)


def mostrar_menu():
    global opcion_actual

    while True:
        limpiar()
        print("DOSIFICADORA DE BOMBONES")
        print("SIMULADOR ESP32-S3\n")

        mostrar_lista(opciones, opcion_actual)

        print("\nW = ARRIBA | S = ABAJO")
        print("E = OK     | Q = SALIR")

        tecla = input("\nSelecciona: ").lower().strip()

        if tecla == "w":
            opcion_actual = max(0, opcion_actual - 1)

        elif tecla == "s":
            opcion_actual = min(
                len(opciones) - 1,
                opcion_actual + 1
            )

        elif tecla == "e":
            if opcion_actual == 3:
                menu_configuracion()
            else:
                limpiar()
                lcd("OPCION ELEGIDA", opciones[opcion_actual])
                input("\nENTER para volver...")

        elif tecla == "q":
            return


def editar_tiempo():
    global tiempo_dosificacion

    temporal = tiempo_dosificacion

    while True:
        limpiar()

        lcd(
            "TIEMPO DOSIFIC.",
            f"> {temporal} SEG"
        )

        print("\nW = +1 SEGUNDO")
        print("S = -1 SEGUNDO")
        print("E = GUARDAR")
        print("Q = CANCELAR")

        tecla = input("\nSelecciona: ").lower().strip()

        if tecla == "w":
            temporal = min(TIEMPO_MAX, temporal + 1)

        elif tecla == "s":
            temporal = max(TIEMPO_MIN, temporal - 1)

        elif tecla == "e":
            tiempo_dosificacion = temporal
            limpiar()
            lcd("TIEMPO GUARDADO", f"{tiempo_dosificacion} SEG")
            time.sleep(1)
            return

        elif tecla == "q":
            return


def menu_configuracion():
    global opcion_config

    opcion_config = 0

    while True:
        limpiar()
        print("SUBMENU CONFIGURACION\n")

        mostrar_lista(opciones_config, opcion_config)

        print("\nW = ARRIBA | S = ABAJO")
        print("E = OK     | Q = VOLVER")

        tecla = input("\nSelecciona: ").lower().strip()

        if tecla == "w":
            opcion_config = max(0, opcion_config - 1)

        elif tecla == "s":
            opcion_config = min(
                len(opciones_config) - 1,
                opcion_config + 1
            )

        elif tecla == "e":

            if opcion_config == 0:
                editar_tiempo()

            elif opcion_config == 1:
                limpiar()
                lcd("VELOCIDAD MOTOR", "PENDIENTE")
                input("\nENTER para volver...")

            elif opcion_config == 2:
                return

        elif tecla == "q":
            return


def main():
    limpiar()
    lcd("  BIENVENIDOS", " SISTEMA ESP32")
    time.sleep(3)

    mostrar_menu()
    print("\nSimulador finalizado.")


if __name__ == "__main__":
    main()

  
#python simulador_menu.py