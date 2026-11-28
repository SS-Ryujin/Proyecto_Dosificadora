
import serial
import threading
import time
import queue


# ==========================================
# CONFIGURACION
# ==========================================

PUERTO = "COM18"
BAUDIOS = 115200

INTERVALO_HEARTBEAT = 0.5
TIMEOUT_HEARTBEAT = 1.5


# ==========================================
# BANCO DE PRUEBAS
# ==========================================

class BancoPruebas:

    def __init__(self):

        self.conexion = None

        self.ejecutando = threading.Event()

        self.heartbeat_activo = threading.Event()

        self.bloqueo_serial = threading.Lock()

        self.respuestas = queue.Queue()

        self.hilo_recepcion = None
        self.hilo_heartbeat = None

        self.ultimo_heartbeat = None


    # ======================================
    # CONEXION SERIAL
    # ======================================

    def conectar(self):

        self.conexion = serial.Serial(
            port=PUERTO,
            baudrate=BAUDIOS,
            timeout=0.1,
            write_timeout=0.5
        )

        print("\nConectando con la ESP32...")

        time.sleep(3)

        self.ejecutando.set()

        self.hilo_recepcion = threading.Thread(
            target=self.recibir,
            daemon=True
        )

        self.hilo_heartbeat = threading.Thread(
            target=self.enviar_heartbeat,
            daemon=True
        )

        self.hilo_recepcion.start()
        self.hilo_heartbeat.start()

        print(f"Conexion establecida: {PUERTO}")
        print(f"Baudios: {BAUDIOS}")


    # ======================================
    # ENVIO DE COMANDOS
    # ======================================

    def enviar(self, comando, mostrar=True):

        if not self.ejecutando.is_set():
            return False

        try:

            with self.bloqueo_serial:

                if not self.ejecutando.is_set():
                    return False

                trama = f"?{comando}*"

                self.conexion.write(
                    trama.encode("ascii")
                )

                if comando == "HEARTBEAT":

                    self.ultimo_heartbeat = (
                        time.monotonic()
                    )

                if mostrar:

                    print(f"\n[TX] {trama}")

            return True

        except (serial.SerialException, OSError) as error:

            print(f"\nError de transmision: {error}")

            self.heartbeat_activo.clear()
            self.ejecutando.clear()

            return False


    # ======================================
    # RECEPCION DE RESPUESTAS
    # ======================================

    def recibir(self):

        buffer = bytearray()

        while self.ejecutando.is_set():

            try:

                datos = self.conexion.read(
                    self.conexion.in_waiting or 1
                )

                if not datos:
                    continue

                buffer.extend(datos)

                while b"\n" in buffer:

                    linea, _, resto = buffer.partition(
                        b"\n"
                    )

                    buffer = bytearray(resto)

                    respuesta = linea.decode(
                        "utf-8",
                        errors="replace"
                    ).strip()

                    if not respuesta:
                        continue

                    instante = time.monotonic()

                    # No mostrar confirmaciones
                    # repetitivas del heartbeat.

                    if respuesta == "OK:HEARTBEAT":
                        continue

                    print(f"\n[RX] {respuesta}")

                    # Registrar eventos relevantes.

                    if (
                        respuesta.startswith("OK:") or
                        respuesta.startswith("ERROR:") or
                        respuesta.startswith("STATUS;") or
                        respuesta.startswith("SEGURIDAD;") or
                        respuesta.startswith("Motivo:")
                    ):

                        self.respuestas.put(
                            (instante, respuesta)
                        )

                    # Desactivar heartbeat ante
                    # revocacion de permisos.

                    if respuesta == "ERROR:HEARTBEAT_RECHAZADO":

                        self.heartbeat_activo.clear()

                    if respuesta == "Control revocado":

                        self.heartbeat_activo.clear()

                    if respuesta.startswith("OK:STOP;"):

                        self.heartbeat_activo.clear()

                    if respuesta.startswith(
                        "OK:CONTROL_RELEASE;"
                    ):

                        self.heartbeat_activo.clear()

            except (serial.SerialException, OSError) as error:

                print(f"\nError de recepcion: {error}")

                self.heartbeat_activo.clear()
                self.ejecutando.clear()

                break


    # ======================================
    # HEARTBEAT AUTOMATICO
    # ======================================

    def enviar_heartbeat(self):

        while self.ejecutando.is_set():

            if self.heartbeat_activo.is_set():

                with self.bloqueo_serial:

                    if (
                        self.heartbeat_activo.is_set()
                        and self.ejecutando.is_set()
                    ):

                        try:

                            self.conexion.write(
                                b"?HEARTBEAT*"
                            )

                            self.ultimo_heartbeat = (
                                time.monotonic()
                            )

                        except (
                            serial.SerialException,
                            OSError
                        ) as error:

                            print(
                                f"\nError Heartbeat: {error}"
                            )

                            self.heartbeat_activo.clear()
                            self.ejecutando.clear()

            time.sleep(INTERVALO_HEARTBEAT)


    # ======================================
    # LIMPIAR RESPUESTAS ANTERIORES
    # ======================================

    def limpiar_respuestas(self):

        while True:

            try:

                self.respuestas.get_nowait()

            except queue.Empty:

                break


    # ======================================
    # ESPERAR RESPUESTA
    # ======================================

    def esperar_respuesta(self, prefijo, timeout=1.0):

        limite = time.monotonic() + timeout

        while time.monotonic() < limite:

            restante = limite - time.monotonic()

            try:

                instante, respuesta = self.respuestas.get(
                    timeout=max(0, restante)
                )

                if respuesta.startswith(prefijo):

                    return instante, respuesta

                if respuesta.startswith("ERROR:"):

                    return None

            except queue.Empty:

                break

        return None


    # ======================================
    # SOLICITAR CONTROL
    # ======================================

    def solicitar_control(self):

        self.desactivar_heartbeat(mostrar=False)

        self.limpiar_respuestas()

        if not self.enviar("CONTROL:TAKE"):
            return False

        respuesta = self.esperar_respuesta(
            "OK:CONTROL_TAKE;"
        )

        if respuesta is None:

            print("\nNo se obtuvo autorizacion.")

            return False

        self.activar_heartbeat()

        print("\nControl USB autorizado.")

        return True


    # ======================================
    # LIBERAR CONTROL
    # ======================================

    def liberar_control(self):

        self.desactivar_heartbeat(mostrar=False)

        self.enviar("CONTROL:RELEASE")


    # ======================================
    # ACTIVAR HEARTBEAT
    # ======================================

    def activar_heartbeat(self):

        if self.enviar("HEARTBEAT", mostrar=False):

            self.heartbeat_activo.set()

            print("\nHeartbeat automatico habilitado.")


    # ======================================
    # DESACTIVAR HEARTBEAT
    # ======================================

    def desactivar_heartbeat(self, mostrar=True):

        with self.bloqueo_serial:

            self.heartbeat_activo.clear()

        if mostrar:

            print("\nHeartbeat automatico deshabilitado.")


    # ======================================
    # CONFIGURACION DEL MOTOR
    # ======================================

    def configurar_motor(self):

        print("\n================================")
        print(" CONFIGURACION DEL MOTOR")
        print("================================")

        print("1. Establecer PWM")
        print("2. Giro horario")
        print("3. Giro antihorario")
        print("4. Modo ciclico")
        print("5. Modo continuo")
        print("6. Tiempo de giro")
        print("7. Tiempo de pausa")
        print("0. Regresar")

        opcion = input("\nSelecciona: ").strip()

        if opcion == "1":

            valor = input("PWM (0-100): ").strip()

            if valor.isdigit() and 0 <= int(valor) <= 100:

                self.enviar(f"PWM:{valor}")

            else:

                print("PWM invalido.")

        elif opcion == "2":

            self.enviar("DIR:CW")

        elif opcion == "3":

            self.enviar("DIR:CCW")

        elif opcion == "4":

            self.enviar("MODO:CICLICO")

        elif opcion == "5":

            self.enviar("MODO:CONTINUO")

        elif opcion in ("6", "7"):

            valor = input(
                "Tiempo en milisegundos: "
            ).strip()

            if (
                valor.isdigit()
                and 1000 <= int(valor) <= 600000
            ):

                comando = (
                    "GIRO" if opcion == "6" else "PAUSA"
                )

                self.enviar(f"{comando}:{valor}")

            else:

                print("Tiempo invalido.")

        elif opcion != "0":

            print("Opcion invalida.")


    # ======================================
    # PARO MANUAL
    # ======================================

    def detener(self):

        self.desactivar_heartbeat(mostrar=False)

        self.enviar("STOP")


    # ======================================
    # PRUEBA DE EXPIRACION
    # ======================================

    def prueba_expiracion(self):

        print("\n================================")
        print(" PRUEBA DE EXPIRACION HEARTBEAT")
        print("================================")

        print("Realizar solamente sin alimentar el motor.")

        confirmar = input(
            "¿Los 12 V estan desconectados? (s/n): "
        ).strip().lower()

        if confirmar != "s":

            print("Prueba cancelada.")

            return

        # Confirmar que el firmware acepte STOP.

        self.limpiar_respuestas()

        self.detener()

        if self.esperar_respuesta("OK:STOP;") is None:

            print("No se confirmo el paro.")

            return

        # Solicitar control y mantener heartbeat.

        if not self.solicitar_control():

            return

        try:

            time.sleep(2)

            # Configurar PWM.

            self.limpiar_respuestas()

            self.enviar("PWM:80")

            if self.esperar_respuesta("OK:PWM;") is None:

                print("No se confirmo el PWM.")

                return

            # Arranque exclusivamente logico.

            self.limpiar_respuestas()

            self.enviar("START")

            respuesta = self.esperar_respuesta(
                "OK:START;"
            )

            if respuesta is None:

                print("Arranque no confirmado.")

                return

            if "ACTIVO=SI" not in respuesta[1]:

                print("El sistema no esta activo.")

                return

            time.sleep(2)

            # Eliminar respuestas anteriores.

            self.limpiar_respuestas()

            # Interrumpir heartbeat.

            self.desactivar_heartbeat()

            inicio = self.ultimo_heartbeat

            print("\nEsperando expiracion del permiso...")

            limite = time.monotonic() + 5

            detectado = False

            while time.monotonic() < limite:

                try:

                    instante, mensaje = self.respuestas.get(
                        timeout=0.1
                    )

                    if mensaje == "Motivo: HEARTBEAT_EXPIRADO":

                        detectado = True

                        print("\nEXPIRACION DETECTADA")

                        if inicio is not None:

                            tiempo_ms = (
                                instante - inicio
                            ) * 1000

                            print(
                                f"Tiempo observado: "
                                f"{tiempo_ms:.2f} ms"
                            )

                        print(
                            "Timeout configurado: "
                            f"{TIMEOUT_HEARTBEAT * 1000:.0f} ms"
                        )

                        break

                except queue.Empty:

                    continue

            if not detectado:

                print(
                    "\nADVERTENCIA: No se detecto "
                    "la expiracion."
                )

            # Confirmar el estado final.

            self.limpiar_respuestas()

            self.enviar("STATUS")

            estado = self.esperar_respuesta("STATUS;")

            self.enviar("SEGURIDAD")

            seguridad = self.esperar_respuesta(
                "SEGURIDAD;"
            )

            detenido = (
                estado is not None
                and "ACTIVO=NO" in estado[1]
                and "ESTADO=DETENIDO" in estado[1]
            )

            revocado = (
                seguridad is not None
                and "PROPIETARIO=NINGUNO" in seguridad[1]
                and "HEARTBEAT=NO_VIGENTE" in seguridad[1]
                and "MOTIVO=HEARTBEAT_EXPIRADO" in seguridad[1]
            )

            if detectado and detenido and revocado:

                print("\nPRUEBA LOGICA SUPERADA.")

            else:

                print("\nPRUEBA NO SUPERADA.")

        finally:

            self.desactivar_heartbeat(mostrar=False)

            # Si el estado final es incierto,
            # solicitar un paro adicional.

            if not (
                locals().get("detenido", False)
                and locals().get("revocado", False)
            ):

                self.enviar("STOP")


    # ======================================
    # MENU PRINCIPAL
    # ======================================

    def menu(self):

        while self.ejecutando.is_set():

            estado_heartbeat = (
                "ACTIVO"
                if self.heartbeat_activo.is_set()
                else "INACTIVO"
            )

            print("\n====================================")
            print("    DOSIFICADORA DE BOMBONES V1.3")
            print("     BANCO DE PRUEBAS DE SEGURIDAD")
            print("====================================")

            print(f"Puerto: {PUERTO}")
            print(f"Baudios: {BAUDIOS}")
            print(f"Heartbeat local: {estado_heartbeat}")

            print("------------------------------------")

            print("1. Solicitar control USB")
            print("2. Liberar control")
            print("3. Activar Heartbeat")
            print("4. Desactivar Heartbeat")
            print("5. Configurar motor")
            print("6. Arrancar motor")
            print("7. Detener motor")
            print("8. Consultar estado")
            print("9. Consultar seguridad")
            print("10. Prueba de expiracion")
            print("0. Salir")

            print("------------------------------------")

            opcion = input(
                "Selecciona una opcion: "
            ).strip()

            if opcion == "1":

                self.solicitar_control()

            elif opcion == "2":

                self.liberar_control()

            elif opcion == "3":

                self.activar_heartbeat()

            elif opcion == "4":

                self.desactivar_heartbeat()

            elif opcion == "5":

                self.configurar_motor()

            elif opcion == "6":

                self.enviar("START")

            elif opcion == "7":

                self.detener()

            elif opcion == "8":

                self.enviar("STATUS")

            elif opcion == "9":

                self.enviar("SEGURIDAD")

            elif opcion == "10":

                self.prueba_expiracion()

            elif opcion == "0":

                break

            else:

                print("\nOpcion invalida.")

            time.sleep(0.3)


    # ======================================
    # CIERRE DEL PROGRAMA
    # ======================================

    def cerrar(self):

        print("\nCerrando banco de pruebas...")

        self.desactivar_heartbeat(mostrar=False)

        if (
            self.conexion is not None
            and self.conexion.is_open
        ):

            if self.ejecutando.is_set():

                self.enviar("STOP")

                time.sleep(0.3)

            self.ejecutando.clear()

            if self.hilo_heartbeat is not None:

                self.hilo_heartbeat.join(timeout=1)

            if self.hilo_recepcion is not None:

                self.hilo_recepcion.join(timeout=1)

            self.conexion.close()

        self.ejecutando.clear()

        print("Puerto serial cerrado.")


# ==========================================
# PROGRAMA PRINCIPAL
# ==========================================

def main():

    banco = BancoPruebas()

    try:

        banco.conectar()

        banco.menu()

    except KeyboardInterrupt:

        print("\nInterrupcion del usuario.")

    except serial.SerialException as error:

        print(f"\nError serial: {error}")

    finally:

        banco.cerrar()


if __name__ == "__main__":

    main()
