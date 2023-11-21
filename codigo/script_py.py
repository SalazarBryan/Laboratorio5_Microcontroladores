#!/usr/bin/python3
import serial
import paho.mqtt.client as mqtt
import json
import datetime
import requests

# Functions
def on_publish(client, userdata, result):
    print("Data published to ThingsBoard\n")
    pass

# Serial connection
ser = serial.Serial(
    port='/dev/ttyACM0',\
    baudrate=115200,\
    parity=serial.PARITY_NONE,\
    stopbits=serial.STOPBITS_ONE,\
    bytesize=serial.EIGHTBITS,\
    timeout=200
)
print("Connected to: " + ser.portstr)

# Datos para la conexión con el cliente MQTT:
line = []
dictionary = dict()
broker = "iot.eie.ucr.ac.cr"
port = 1883
topic = "v1/devices/me/telemetry"
username = "STM"
password = "ozwnzj5hfdmn1nyjlxev"

# Abrir el archivo en modo de escritura
archivo_salida = 'datos_del_microfono.txt'

# Establecer los parámetros de conexión MQTT
client = mqtt.Client(username)
client.on_publish = on_publish
client.username_pw_set(password)
client.connect(broker, port, keepalive=60)

while True:
    # Leer una línea desde el puerto serie
    linea = ser.readline().decode().strip()

    # Obtener la fecha y hora actual
    tiempo_actual = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # Etiquetas para los comandos de voz
    etiquetas = {
        'Luces': 'ComandoLuces',
        'Televisión': 'ComandoTV',
        'Calefacción': 'ComandoCalefaccion'
    }

    # Etiqueta del comando de voz (por defecto es desconocido)
    etiqueta_comando = 'ComandoDesconocido'

    # Verificar si la línea contiene alguna palabra clave de comando
    for comando, etiqueta in etiquetas.items():
        if comando.lower() in linea.lower():
            etiqueta_comando = etiqueta
            break

    # Escribir en el archivo
    with open(archivo_salida, 'a') as archivo:
        archivo.write(f'{tiempo_actual} - {etiqueta_comando}: {linea}\n')

    # Mostrar en la consola para verificar
    print(f'{tiempo_actual} - {etiqueta_comando}: {linea}')

    # Enviar datos a ThingsBoard
    payload = {etiqueta_comando: linea}
    client.publish(topic, json.dumps(payload))
