import asyncio
from bleak import BleakClient

MAC_ESP32 = "00:70:07:A2:59:22"


async def prueba():
    print("Conectando con la ESP32...")

    async with BleakClient(MAC_ESP32) as cliente:

        print("Conexion BLE:", cliente.is_connected)

        if cliente.is_connected:
            print("ESP32 conectada correctamente")
            print("Manteniendo conexion durante 5 segundos...")

            await asyncio.sleep(5)

    print("ESP32 desconectada")


if __name__ == "__main__":
    asyncio.run(prueba())