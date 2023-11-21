#include <ArduinoSound.h>

// Configurar el micrófono
const int sampleWindow = 50; // Tamaño de la ventana para el promedio de sonido
const int micPin = A0;       // Pin analógico para el micrófono

void setup()
{
  Serial.begin(9600);

  // Iniciar el micrófono
  Sound.begin();

  Serial.println("Listo para capturar y enviar datos del micrófono por USB");
}

void loop()
{
  // Obtener el valor del micrófono
  int soundValue = analogRead(micPin);

  // Enviar el valor por el puerto serie (USB)
  Serial.println(soundValue);

  // Esperar un poco antes de la siguiente lectura
  delay(100);
}
