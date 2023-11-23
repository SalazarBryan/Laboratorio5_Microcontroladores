#!/usr/bin/python3
import serial
import paho.mqtt.client as mqtt
import datetime

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
broker = "iot.eie.ucr.ac.cr"
port = 1883
topic = "v1/devices/me/telemetry"
username = "VOICE_DET"
password = "ls6xz"

# Abrir el archivo en modo de escritura
archivo_salida = 'datos_del_microfono.txt'

# Establecer los parámetros de conexión MQTT
client = mqtt.Client()
client.on_publish = on_publish
client.username_pw_set(username, password)
client.connect(broker, port, keepalive=60)

switches = {
    'Luces': False,
    'Television': False,
    'Calefaccion': False
}

while True:
    # Leer una línea desde el puerto serial
    linea = ser.readline().decode().strip()
    print(linea)
    # Obtener la fecha y hora actual
    tiempo_actual = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    # Etiquetas para los comandos de voz
    etiquetas = ['Luces', 'Television', 'Calefaccion']

    # Verificar si la línea contiene alguna palabra clave de comando
    for comando in etiquetas:
        if comando.lower() == linea.lower():
            # Toggle switches
            switches[comando] = ~switches[comando]
            # Escribir en el archivo
            with open(archivo_salida, 'a') as archivo:
                archivo.write(f'{tiempo_actual} - {comando}: {linea}\n')

            # Mostrar en la consola para verificar
            print(f'{tiempo_actual} - {comando}: {linea}')

            # Enviar datos a ThingsBoard
            payload = {comando: switches[comando]}
            client.publish(topic, str(payload))