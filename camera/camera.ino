#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h" 
#include "soc/rtc_cntl_reg.h" 
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned long lastTime = millis();
unsigned long currentTime = millis();
// unsigned long streamTime = 10; //CHANGE FOR DURATION OF STREAM (MIN)
// unsigned long timerDelay = streamTime * 60 * 1000;
unsigned long timerDelay = 10000; 

SemaphoreHandle_t detectSemaphore;
void TaskPIR(void *pvParameters);
void TaskStream(void *pvParameters);

//board pin config
#define PWDN_GPIO_NUM       -1
#define RESET_GPIO_NUM      -1
#define XCLK_GPIO_NUM       32
#define SIOD_GPIO_NUM       13
#define SIOC_GPIO_NUM       12

#define Y9_GPIO_NUM         39
#define Y8_GPIO_NUM         36
#define Y7_GPIO_NUM         23
#define Y6_GPIO_NUM         18
#define Y5_GPIO_NUM         15
#define Y4_GPIO_NUM         4
#define Y3_GPIO_NUM         14
#define Y2_GPIO_NUM         5

#define VSYNC_GPIO_NUM      27
#define HREF_GPIO_NUM       25
#define PCLK_GPIO_NUM       19

#define I2C_SDA             21
#define I2C_SCL             22

#define BUTTON_1            34

#define SSD130_MODLE_TYPE   0   // 0 : GEOMETRY_128_64  // 1: GEOMETRY_128_32

#define AS312_PIN           33

#define ENABLE_IP5306

const char* ssid     = "Dragon"; // CHANGE HERE
const char* password = "12345678"; // CHANGE HERE

const char* websockets_server_host = "192.168.0.193"; //CHANGE HERE
const uint16_t websockets_server_port = 3002; // OPTIONAL CHANGE

camera_fb_t * fb = NULL;
size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;
uint8_t state = 0;

using namespace websockets;
WebsocketsClient client;

void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());
}

esp_err_t init_camera() {
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

  // parameters for image quality and size
  config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 15; //10-63 lower number means higher quality
  config.fb_count = 2;
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("camera init FAIL: 0x%x", err);
    return err;
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);
  Serial.println("camera init OK");
  return ESP_OK;
};


esp_err_t init_wifi() {
  WiFi.begin(ssid, password);
  Serial.println("Wifi init ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi OK");
  Serial.println("connecting to WS: ");
  client.onMessage(onMessageCallback);
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
  if (!connected) {
    Serial.println("WS connect failed!");
    Serial.println(WiFi.localIP());
    Serial.print("WebSocket server host: ");
    Serial.println(websockets_server_host);
    Serial.print("WebSocket server port: ");
    Serial.println(websockets_server_port);
    state = 3;
    return ESP_FAIL;
  }
  if (state == 3) {
    return ESP_FAIL;
  }

  Serial.println("WS OK");
  client.send("hello from ESP32 camera stream!");
  return ESP_OK;
};


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(AS312_PIN, INPUT);

  detectSemaphore = xSemaphoreCreateBinary();
  xSemaphoreTake(detectSemaphore, 0);

  xTaskCreatePinnedToCore(
    TaskPIR, "TaskPIR"  
    ,
    10000  
    ,
    NULL, 1  
    ,
    NULL, 0);

  xTaskCreatePinnedToCore(
    TaskStream, "TaskStream", 10000  
    ,
    NULL, 1  
    ,
    NULL, 1);
  init_camera();
  init_wifi();

}

void loop() {
  
}

bool PIRDetection(){
  // Serial.println(analogRead(AS312_PIN));
  return(analogRead(AS312_PIN) > 0);
}

void TaskPIR(void *pvParameters) {
  for (;;) {
    bool detected = PIRDetection();
    
    if (detected) {
      // Serial.println("Motion detected");
      xSemaphoreGive(detectSemaphore);
      vTaskDelay(pdMS_TO_TICKS(timerDelay));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


void TaskStream(void *pvParameters){
  for(;;) {
    if (xSemaphoreTake(detectSemaphore, pdMS_TO_TICKS(100)) == pdTRUE){
      // Serial.println(client.available());
      if (client.available()){
        lastTime = millis();
        currentTime = millis();
        while(currentTime - lastTime <= timerDelay){
          // Serial.print("Inside loop. currentTime: ");
          // Serial.print(currentTime);
          // Serial.print(", lastTime: ");
          // Serial.println(lastTime);
          
          camera_fb_t *fb = esp_camera_fb_get();
          if (!fb) {
            Serial.println("img capture failed");
            esp_camera_fb_return(fb);
            ESP.restart();
          }
          client.sendBinary((const char*) fb->buf, fb->len);
          Serial.println("image sent");
          esp_camera_fb_return(fb);
          client.poll();
          currentTime = millis();
        }
        // Serial.println("Motion capture Stop");
      }
    }
  }
}
