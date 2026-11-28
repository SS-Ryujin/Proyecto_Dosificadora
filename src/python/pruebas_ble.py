import asyncio
from bleak import BleakClient

MAC_ESP32 = "00:70:07:A2:59:22"

UUID_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
UUID_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

buffer_recepcion = bytearray()
respuestas = asyncio.Queue()


def recibir_respuesta(sender, datos):
    buffer_recepcion.extend(datos)

    while b"\n" in buffer_recepcion:
        posicion = buffer_recepcion.index(b"\n")

        mensaje = buffer_recepcion[:posicion].decode(
            "utf-8", errors="replace"
        ).strip()

        del buffer_recepcion[:posicion + 1]

        print("[RX BLE]", mensaje)
        respuestas.put_nowait(mensaje)


async def enviar_comando(cliente, comando, esperado):
    print("[TX BLE]", comando)

    await cliente.write_gatt_char(
        UUID_RX,
        comando.encode("utf-8"),
        response=True
    )

    respuesta = await asyncio.wait_for(
        respuestas.get(),
        timeout=1.0
    )

    if not respuesta.startswith(esperado):
        raise RuntimeError(
            f"Respuesta inesperada: {respuesta}"
        )

    return respuesta


async def prueba():

    confirmacion = input(
        "¿Los 12 V del motor estan desconectados? (SI/NO): "
    )

    if confirmacion.strip().upper() != "SI":
        print("Prueba cancelada por seguridad.")
        return

    print("\nConectando con la ESP32...")

    async with BleakClient(MAC_ESP32) as cliente:

        print("Conexion BLE:", cliente.is_connected)

        await cliente.start_notify(
            UUID_TX,
            recibir_respuesta
        )

        await asyncio.sleep(1)

        print("\nSolicitando control BLE...")

        await enviar_comando(
            cliente,
            "?CONTROL:TAKE*",
            "OK:CONTROL_TAKE;"
        )

        await enviar_comando(
            cliente,
            "?HEARTBEAT*",
            "OK:HEARTBEAT"
        )

        await enviar_comando(
            cliente,
            "?PWM:80*",
            "OK:PWM;"
        )

        await enviar_comando(
            cliente,
            "?HEARTBEAT*",
            "OK:HEARTBEAT"
        )

        print("\nActivando motor logicamente...")

        await enviar_comando(
            cliente,
            "?START*",
            "OK:START;"
        )

        for _ in range(4):

            await enviar_comando(
                cliente,
                "?HEARTBEAT*",
                "OK:HEARTBEAT"
            )

            await asyncio.sleep(0.5)

        estado = await enviar_comando(
            cliente,
            "?STATUS*",
            "STATUS;"
        )

        if "ACTIVO=SI;" not in estado:
            raise RuntimeError(
                "El sistema no esta activo."
            )

        await enviar_comando(
            cliente,
            "?HEARTBEAT*",
            "OK:HEARTBEAT"
        )

        print("\nINTERRUMPIENDO CONEXION BLUETOOTH...")

        await cliente.disconnect()

        print("Conexion BLE:", cliente.is_connected)

    print("\nDesconexion ejecutada.")
    print("Consultar los resultados en la terminal USB.")


if __name__ == "__main__":
    asyncio.run(prueba())