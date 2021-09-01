# Arduino_Security
## **Componentes IoT**

- Módulo wifi esp8266 NodeMCU
- Módulo Sensor De Movimiento Pir Hc-sr 505 Arduino
- Esp32-cam

## **IDE**
- Arduino 1.8.13 o otra versión deseable.

## **Añadir librerias**
Añadir las siguientes URLs en preferencias en el campo de Gestor de URLs Adicionales de Tarjetas. Esto permite añadir nuevas librerias a instalar.
- https://dl.espressif.com/dl/package_esp32_index.json
- http://arduino.esp8266.com/stable/package_esp8266com_index.json
- https://arduino.esp8266.com/stable/package_esp8266com_index.json

Instalar las librerias(en caso de ser necesario) de:
  - #include <HTTPClient.h>
  - #include "esp_camera.h"
  - #include <WiFi.h>
  - #include "base64.h"
  - #include <ArduinoJson.h>
  - #include <ArduinoJson.hpp>
  - #include <WiFiClient.h>
  - #include <TimeLib.h>

## **Para ejecutar el programa arduino debe establecer:**
- Placa: ESP32 Dev Module
- Upload Speed: 115200
- CPU Frequency: 80MHz (Wifi/BT)
- Flash Frequency: 80MHz
- Flash Mode: QIO
- Flash Size: 4MB (32Mb)
- Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
- Core Debug Level : Ninguno
- PSRAM : Disabled
- Puerto: COM4(depende donde sea conectada la placa)

## **Cargar el programa**
- Cargar el programa a la placa.
- En la mitad de la carga del programa presionar RST en la cámara ESP32.
- Antes de cargar el programa, conectar GPIO0 a GND. Una vez que se cargue el programa, retirar este puente y presionar el botón RST en la cámara ESP32.
- Abrir el monitor serie y restablezcer el ESP32. Puede verse la dirección IP en la que se ha iniciado el servidor web de cámara.
