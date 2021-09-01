#include <HTTPClient.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "base64.h"
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFiClient.h>
#include <TimeLib.h>

WiFiClient wifiClient;

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"

const char* ssid = "UTEQ-DOCENTES";
const char* password = "docenciauteq";

time_t fecha;

const int motionSensor = 16;
int idComponente;//lo que se envia

void startCameraServer();

void setup() {
  Serial.begin(115200);
  pinMode(motionSensor, INPUT_PULLUP);
  for (int i = 0; i > 30; i++) //Utilizamos un for para calibrar el sensor depende del tipo de sensor que utilicemos va a cambiar el tiempo de calibración
  {
    delay(1000);
  }
  delay(50);


  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);
  Serial.println("Conectando a la red wifi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  setTime(12, 55, 0, 26, 8, 2021);
}

void loop() {

  fecha = now();
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Conectado ");
    HTTPClient http;//Instancia de la clase

    //GET---------COMPONENTES
    bool estadoPir = true;
    http.begin(wifiClient, "http://wssecurity.herokuapp.com/api-seguridad/componentes/");
    int httpCodGet = http.GET();
    if (httpCodGet == 200)
    {
      String respuestacomp = http.getString();
      StaticJsonDocument<192> docComp;

      DeserializationError error = deserializeJson(docComp, respuestacomp);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      JsonObject componente_0 = docComp["componente"][0];
      estadoPir = componente_0["estado"]; // true
      Serial.println("El servidor respondio:");
      Serial.println(respuesta);
    } else {
      Serial.println(httpCodGet);
    }

    //GET---------SOLICITADO
    http.begin(wifiClient, "https://wssecurity.herokuapp.com/api-seguridad/solicitud/");
    int httpCodGetSolic = http.GET();
    if (httpCodGetSolic == 200)
    {
      String respuesta = http.getString();
      StaticJsonDocument<48> doc;
      DeserializationError error = deserializeJson(doc, respuesta);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      bool solicitud = doc["solicitud"]; 
      
      //+++++++++++++++++++++++++SOLICITADO
      if (solicitud) {
        http.begin(wifiClient, "http://wssecurity.herokuapp.com/api-seguridad/historial-anomalias/");
        http.addHeader("content-type", "application/json");
        camera_fb_t * fb = NULL;

        fb = esp_camera_fb_get();

        String imgDataB64 = base64::encode(fb->buf, fb->len);

        String jsonSolicitud;
        DynamicJsonDocument doc(12288);

        doc["componente_id"] = 1;
        doc["fecha"] = String(year(fecha)) + "-" + String(month(fecha)) + "-" + String(day(fecha));
        doc["tipo_historial"] = "Solicitado";

        JsonObject evidencias_0 = doc["evidencias"].createNestedObject();
        evidencias_0["hora"] = String(hour(fecha)) + ":" + String(minute(fecha)) + ":" + String(second(fecha));
        evidencias_0["foto"] = "data:image/PNG;base64," + imgDataB64;

        serializeJson(doc, jsonSolicitud);

        int httpCodPostSolic = http.POST(jsonSolicitud);


        if (httpCodPostSolic == 200)
        {
          String respuesta = http.getString();
          Serial.println("El servidor respondio:");
          Serial.println(respuesta);
        } else {
          Serial.println(httpCodPostSolic);
        }

        String jsonSolicitudModif;
        http.begin(wifiClient, "http://wssecurity.herokuapp.com/api-seguridad/solicitud/");
        http.addHeader("content-type", "application/json");

        StaticJsonDocument<16> docSolicitudModif;
        docSolicitudModif["estado"] = 0;
        serializeJson(docSolicitudModif, jsonSolicitudModif);
        int httpCodPutSolic = http.PUT(jsonSolicitudModif);
        if (httpCodPutSolic == 200)
        {
          String respuesta = http.getString();
          Serial.println("El servidor respondio:");
          Serial.println(respuesta);
        } else {
          Serial.println(httpCodPutSolic);
        }
      }
    } else {
      Serial.println(httpCodGetSolic);
    }


    //+++++++++++++++++++++++++ANOMALIA
    if (estadoPir &&  digitalRead(motionSensor) == HIGH) {
      idComponente = 1; //LATERAL DE DERECHO
      Serial.println(idComponente);

      //POST
      http.begin(wifiClient, "http://wssecurity.herokuapp.com/api-seguridad/historial-anomalias/");
      http.addHeader("content-type", "application/json");

      if (idComponente > 0) {
        camera_fb_t * fb = NULL;
        fb = esp_camera_fb_get();

        String imgDataB64 = base64::encode(fb->buf, fb->len);

        String json;
        DynamicJsonDocument doc(12288);

        doc["componente_id"] = idComponente;
        doc["fecha"] = String(year(fecha)) + "-" + String(month(fecha)) + "-" + String(day(fecha));
        doc["tipo_historial"] = "Anomalía";

        JsonObject evidencias_0 = doc["evidencias"].createNestedObject();
        evidencias_0["hora"] = String(hour(fecha)) + ":" + String(minute(fecha)) + ":" + String(second(fecha));
        evidencias_0["foto"] = "data:image/PNG;base64," + imgDataB64;

        Serial.println(imgDataB64);
        serializeJson(doc, json);

        Serial.println(json);
        int httpCod = http.POST(json);

          if (httpCod == 200)
          {
          String respuesta = http.getString();
          Serial.println("El servidor respondio:");
          Serial.println(respuesta);
          } else {
          Serial.println(httpCod);
          }
      }
      http.end();
    }
  }

  delay(5000);
}
