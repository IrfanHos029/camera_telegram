/*
  diedit tanggal 07-04-2022

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
*/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

const char* ssid = "Irfan.A";
const char* password = "irfan0204";

// Initialize Telegram BOT
String BOTtoken = "5290000728:AAFj2pNyRgWuMOE_iki8pCGK3RK72QpcHyk";  // your Bot Token (Get from Botfather)

#define lampu1 14
#define lampu2 2
#define pir1   13
#define pir2   15
#define indikator 12

byte pir_1=0;
byte pir_2=0;
bool stateLam1 = false;
bool stateLam2 = false;
bool statePir1 = false;
bool statePir2 = false;
bool stateInd = false;
bool stateSensor = false;
bool stateSecurity = false;

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
String CHAT_ID = "5121769062";

bool sendPhoto = false;
 
WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

#define FLASH_LED_PIN 4
bool flashState = LOW;

unsigned long Sindi = 0;
byte Dindi = 1000;
bool stateI = false;

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


void configInitCamera(){
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

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
}

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      String welcome = "Welcome , " + from_name + "\n";
      welcome += "Untuk menggunakan fitur dalam system ini tekan perintah dibawah ini \n";
      welcome += "/photo : untuk mengambil gambar\n";
      welcome += "/flash : menyalakan flash \n";
      welcome += "/lampu1_ON : menyalakan lampu 1 \n";
      welcome += "/lampu1_OFF : mematikan lampu 1\n";
      welcome += "/lampu2_ON : menyalakan lampu 2\n";
      welcome += "/lampu2_OFF : mematikan lampu 2\n";
      //welcome += "/save : menyimpan foto ke SD CARD\n";
      welcome += "/security_ON : mengaktifkan fitur keamanan\n";
      welcome += "/security_OFF : mematikan fitur keamanan\n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }
    if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      if(flashState == true){ bot.sendMessage(CHAT_ID, "lampu flash AKTIF", "");}
      else{bot.sendMessage(CHAT_ID, "lampu flash NON AKTIF", "");}
      Serial.println("Change flash LED state");
    }
    if (text == "/photo") {
      sendPhoto = true;
      Serial.println("New photo request");
    }

    if (text == "/lampu1_ON"){
      stateLam1 = true;
      Serial.println("lampu 1 ON");
      bot.sendMessage(CHAT_ID, "lampu 1 dinyalakan", "");
    }

    if (text == "/lampu1_OFF"){
      stateLam1 = false;
      Serial.println("lampu 1 OFF");
      bot.sendMessage(CHAT_ID, "lampu 1 dimatikan", "");
    }

    if (text == "/lampu2_ON"){
      stateLam2 = true;
      Serial.println("lampu 2 ON");
      bot.sendMessage(CHAT_ID, "lampu 2 dinyalakan", "");
    }

    if (text == "/lampu2_OFF"){
      stateLam2 = false;
      Serial.println("lampu 2 OFF");
      bot.sendMessage(CHAT_ID, "lampu 2 dimatikan", "");
    }

    if (text == "/security_ON"){
      stateSecurity = true;
      Serial.println("SYSTEM SECURITY AKTIF");
      bot.sendMessage(CHAT_ID, "SYSTEM SECURITY AKTIF", "");
    }

    if (text == "/security_OFF"){
      stateLam1=false;
      stateLam2=false; 
      stateSecurity = false;
      Serial.println("SYSTEM SECURITY NON AKTIF");
      bot.sendMessage(CHAT_ID, "SYSTEM SECURITY NON AKTIF", "");
    }
  }
}

String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));


  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  // Init Serial Monitor
  Serial.begin(115200);

  // Set LED Flash as output
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);
  //pinMode(33,OUTPUT);
  pinMode(lampu1,OUTPUT);
  pinMode(lampu2,OUTPUT);
  pinMode(pir_1,INPUT);
  pinMode(pir_2,INPUT);
  pinMode(indikator,OUTPUT);
  

  // Config and init the camera
  configInitCamera();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
   // digitalWrite(33,HIGH);
    digitalWrite(indikator,LOW);
    Serial.print(".");
    delay(500);
  }
  //digitalWrite(33,LOW);
  digitalWrite(indikator,HIGH);
  bot.sendMessage(CHAT_ID, "SYSTEM SECURITY CAMERA AKTIF", "");
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); 
}

void loop() {
  Indikator();
  if (sendPhoto) {
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }
  if(stateSecurity==true){
    sensorPripare();
    if(stateSensor==true){
      bot.sendMessage(CHAT_ID, "OBJEK TERDETEKSI", "");
      stateLam1=true;
      stateLam2=true;
      bot.sendMessage(CHAT_ID, "MENG-AKTIFKAN LAMPU DARURAT", "");
      sendPhotoTelegram();
    }

    else if(stateSensor == false){
      stateLam1=false;
      stateLam2=false;
    }
  }

  
  
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  systemSecurity(false);
  outLamp();
}

void systemSecurity(bool state){

  
}

void sensorPripare(){
  pir_1 = digitalRead(pir1);
  pir_2 = digitalRead(pir2);

  if(pir_1 == HIGH && pir_2 == HIGH){
    stateSensor = true;
  }
  if(pir_1 == HIGH && pir_2 == LOW){
    stateSensor = false;
  }
  if(pir_1 == LOW && pir_2 == HIGH){
    stateSensor = false;
  }
  if(pir_1 == LOW && pir_2 == LOW){
    stateSensor = false;
  }
  Serial.println(pir_1);
  Serial.println(pir_2);
}

void outLamp(){
  if(stateLam1 == true){
    digitalWrite(lampu1,HIGH);
  }
  else if(stateLam1 == false){
    digitalWrite(lampu1,LOW);
  }

  if(stateLam2 == true){
    digitalWrite(lampu2,HIGH);
  }
  else if(stateLam2 == false){
    digitalWrite(lampu2,LOW);  
  }

  if(stateI == true){
    digitalWrite(indikator,HIGH);
  }
  else if(stateI == false){
    digitalWrite(indikator,LOW);
  }
}

void Indikator(){
  unsigned long tmr = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    if(tmr - Sindi > Dindi){
      Sindi = tmr;
      stateI = !stateI;
      //digitalWrite(33,HIGH);
    }
  }
  else{
    stateI = true; }
    //digitalWrite(33,LOW);}
}
