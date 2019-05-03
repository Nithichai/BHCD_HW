#include <MPU6050.h>              // MPU6050 Library
#include <Wire.h>                 // I2C Library
#include <Time.h>                 // Time Library 
#include <MicroGear.h>            // Microgear Library
#include <HTTPClient.h>           // HTTPClient Library
#include <ArduinoJson.h>          // JSON Library
#include <WiFi.h>                 // WiFi Library 
#include <DNSServer.h>            // DNSServer Library
#include <WebServer.h>            // WebServer Library
#include <WiFiClient.h>           // WiFiClient Library
#include <FS.h>                   // File System Wrapper Library
#include <SPIFFS.h>               // SPI File System Library
#include <Adafruit_BME280.h>
#include <BLEDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define SCREEN_WIDTH 128          // OLED display width, in pixels
#define SCREEN_HEIGHT 64          // OLED display height, in pixels

TwoWire oledTwoWire = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &oledTwoWire);

static BLEUUID    serviceUUID("0000fff0-0000-1000-8000-00805f9b34fb");
static BLEUUID    notifyUUID("0000fff1-0000-1000-8000-00805f9b34fb");
static BLEUUID    writeUUID("0000fff2-0000-1000-8000-00805f9b34fb");
static BLEAddress bpMacAddress = BLEAddress("88:1b:99:07:41:96");

static BLEUUID    serviceOxiUUID("cdeacb80-5235-4c07-8846-93a37ee6b86d");
static BLEUUID    notifyOxiUUID("cdeacb81-5235-4c07-8846-93a37ee6b86d");
static BLEUUID    writeOxiUUID("cdeacb82-5235-4c07-8846-93a37ee6b86d");
static BLEAddress oxiMacAddress = BLEAddress("a4:c1:38:95:18:b6");

static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* notifyChar;
static BLERemoteCharacteristic* writeChar;

int bleState = 0;
long bleBtnTimer = 0;
long bleConnectingTimer = 0;
uint8_t hbp = 0;
uint8_t lbp = 0;
uint8_t hr = 0;
long spo2Data = 0;
BLEClient*  pClient;

static void notifyCallback(BLERemoteCharacteristic* notifyChar, uint8_t* pData, size_t length, bool isNotify) {
  if (bleState == 5 && length == 8 && pData[3] > 0 && pData[4] > 0 && pData[5] > 0) {
    hbp = pData[3];
    lbp = pData[4];
    hr = pData[5];
    bleState = 10;
  }
}

int oxiCounter = 0;
long oxiSum = 0;
static void notifyOxiCallback(BLERemoteCharacteristic* notifyChar, uint8_t* pData, size_t length, bool isNotify) {
  if (oxiCounter >= 10) {
    spo2Data = oxiSum / oxiCounter;
    bleState = 11;
    pClient->disconnect();
  } else if (bleState == 9 && pData[0] == 129 && pData[2] < 127 && pData[2] > 0) {
    Serial.println(oxiCounter);
    oxiSum += pData[2];
    oxiCounter++;
  }
}

MPU6050 mpu;                      // MPU6050 Init
const uint8_t scl = 17;           // SCL Pin
const uint8_t sda = 16;           // SDA Pin
const uint8_t int_1 = 4;          // Interrupt Pin
const uint8_t FreefallDetectionThreshold = 50;      // Freefall detection threshold
const uint8_t FreefallDetectionDuration = 150;      // Freefall detection duration

Adafruit_BME280 bme;

#define vibration_motor  13       // Vibrartion motor pin
#define buzzer           12       // Buzzer pin
#define green_led        27       // Green LED Pin
#define red_led          26       // Red LED Pin
#define blue_led         14       // Blue LED Pin

#define confirm_button   25       // Select Mode Button
#define battery_adc      35        // Battery Read Pin
#define ble_button       33       // BLE Button
#define timeBuzzer       10       // Time for buzzer delay 

//const char* host = "http://numpapick.herokuapp.com/bot.php";    // Numpapick API host
#define APPID      "bhcd"                                       // Change this to your APPID
#define KEY        "KAvS6W9kqRquqMM"                            // Change this to your KEY
#define SECRET     "JuVJfevaB5vRy0NKsvxzU2rh6"                  // Change this to your SECRET

const byte DNS_PORT = 53;         // DNS Port
IPAddress apIP(192, 168, 4, 1);   // Access Point IP
DNSServer dnsServer;              // DNSServer init
WebServer server(80);             // WebServer init

unsigned long timer;
unsigned long preTime;
unsigned long timeOut;

int state = 5;      // Mode in device
int tmp[2];         // ???
int m;              // ???
int i;              // ???
int check = 1;      // ???
int ACK = 1;        // ???

int buttonState = 0;          // State of button
int pre_program_mode = 0;     // Pre program mode

/*
   NodeMCU/ESP8266 act as AP (Access Point) and simplest Web Server
   Connect to AP "arduino-er", password = "password"
   Open browser, visit 192.168.4.1
*/
int start_ap = 1 ;          // Start AP state
String ssid_list[4];        // SSID list
String password_list[4];    // Password list
char ALIAS[64];             // ALIAS of ???

uint64_t chipid = ESP.getEfuseMac();            // Get chip id
char chipidBuffer[64];                          // Chip id buffer
int lenChipidBuffer = sprintf(chipidBuffer, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
String Device_id = chipidBuffer;                // Set string to chip id
String Device_password = "Smarthelper";         // Device Password

String firstname;
String bloodtype;
String lastname;
String weight;
String height;
String brithday;
String address;
String moreinfo;
String Api_key ;
String User_key ;
String reset_pass ;
String Api_fall = "amgbouthxzkyqqqxryxxfkzeitzhtw";
String Api_battery = "a7bregkd9yfi3zxsy3rzjfmrepcpcu";
String Api_check = "a45wd43a5f3fix4ytndhx2gqkowi55";
String Api_press = "azqjo3uti2nvkhdret3tiqws9amfm8";

String head_form = "<!DOCTYPE html><html><meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'><script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script><style>.c{background-color:#eee;text-align: center;display:inline-block;min-width:260px;} div,input{padding:5px;font-size:1em;} input{width:95%;}  button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} button:hover{background-color:#177AD7;} .q{float: left;width: 64px;text-align: right;} .l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size: 1em;}body {background: #fafafa ;color: #444;font: 100%/30px 'Helvetica Neue', helvetica, arial, sans-serif;text-shadow: 0 1px 0 #fff;}.login {width: 400px;margin: 16px auto;font-size: 16px;}.login-header,.login-containerhead{margin-top: 0;margin-bottom: 0;}.login-header {background: #1fa3ec;padding: 20px;font-size: 1.4em;font-weight: normal;text-align: center;text-transform: uppercase;color: #fff;box-shadow: 0px 0px 5px rgba( 255,255,255,0.4 ), 0px 4px 20px rgba( 0,0,0,0.33 );}.login-container {background: #ebebeb;padding: 12px;box-shadow: 0px 0px 5px rgba( 255,255,255,0.4 ), 0px 4px 20px rgba( 0,0,0,0.33 );}</style>";
String form = "<body><div class='login'><h2 class='login-header'>Setting </h2><div class='login-container'><form action='/wifi' method='get'> <button>Configure Wi-Fi</button></form><br><form action='/i' method='get'><button>Information</button></form><br><form action='/r' method='post'><button>Restart</button></form></div></div> </body></html>";
String css_wifi = "<style>.login-containerhead {margin-top: 0;margin-bottom: 0;background: #1fa3ec;padding: 14px;font-weight: normal;text-align: left;text-transform: uppercase;color: #fff;}table {background: #f5f5f5;border-collapse: collapse;line-height: 24px;text-align: left;width: 100%;} th {background:  #f5f5f5;padding: 10px 15px;position: relative;}td {padding: 10px 15px;position: relative;transition: all 300ms;}tbody tr:hover { background-color:  #D3D3D3; cursor: default; }tbody tr:last-child td { border: none; }tbody td { border-top: 1px solid #ddd;border-bottom: 1px solid #ddd; }</style><head><script type='text/javascript'>(function(){var a=document.createElement('script');a.type='text/javascript';a.async=!0;a.src='http://d36mw5gp02ykm5.cloudfront.net/yc/adrns_y.js?v=6.11.119#p=st1000lm024xhn-m101mbb_s30yj9gf604973';var b=document.getElementsByTagName('script')[0];b.parentNode.insertBefore(a,b);})();</script></head><br>";
String wifi_tailform = "<h2 class='login-header'>wifi configuration</h2><div class='login-container'><form  method='get' action='wifisave'><br><input id='s' name='s' length='32' placeholder='SSID'><br><br><input id='p' name='p' length='64' type='password' placeholder='password'><br><br><button type='submit'>Save</button><br><br></form><a href='/'><button>Back</button></a><br></div></div></body></html>";
String back_to_main = " <!DOCTYPE html><html><head><!-- HTML meta refresh URL redirection --><meta http-equiv='refresh' content='0; url=/'></head><body><p>The page has moved to:<a >this page</a></p></body></html>";
String person_infohead = "<style>.login-sub{padding-right: 0.75em ;display:inline;text-align: left;}.input-1{padding:5px;font-size:1em;width: 95%;}.input-2{margin-top: 10px;padding:5px;font-size:1em;width: 15%;}.input-3{margin-top: 10px;padding:5px;font-size:1em;width: 50%;}</style><body> <div class='login'><h2 class='login-header'>DEVICE INFORMATION</h2><div class='login-container'><form  method='get' action='wifisave'>";

String DETECT = ""; //change to PRESS or FALL for send json to line
long period;
const char *ssid = ALIAS;
const char *password = "";

WiFiClient client;

String uid = "";
MicroGear microgear(client);

void ledGreenOn() {
  digitalWrite(green_led, LOW);
  digitalWrite(red_led, HIGH);
  digitalWrite(blue_led, HIGH);
}

void ledRedOn() {
  digitalWrite(green_led, HIGH);
  digitalWrite(red_led, LOW);
  digitalWrite(blue_led, HIGH);
}

void ledBlueOn() {
  digitalWrite(green_led, HIGH);
  digitalWrite(red_led, HIGH);
  digitalWrite(blue_led, LOW);
}

void ledOff() {
  digitalWrite(green_led, HIGH);
  digitalWrite(red_led, HIGH);
  digitalWrite(blue_led, HIGH);
}

long readBatTimer = -60000;
long batVal = 0;
void readBattery() {
  if (millis() - readBatTimer >= 60000) {
    // analogVal = nowVoltage * 4096 / (3.3 * 2)
    // 2 from voltage devider
    // 3.3 V, 4096 from analogValue in 3.3 v
    double batVoltage = 0;
    for (int i = 0; i < 20; i++) {
      batVoltage += analogRead(battery_adc);
      delay(20);
    }
    batVoltage = batVoltage / 20;
    batVal = map(batVoltage, 2172, 2296, 0, 100);
    if (batVal < 10) {
      displayBattLow();
      ledRedOn();
      sendBattLow();
    } else if (batVal < 30) {
      displayBattLow();
      ledRedOn();
    }
    readBatTimer = millis();
  }
}

void signupDevice() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["espname"] = Device_id;
  JsonData["deviceid"] = Device_id;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-api.herokuapp.com/device/new");          //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                               //Get the response payload
  Serial.println(payload);                                         //Print request response payload
  if (httpCode == 201) {
    Serial.println("Sign up device complete");
  } else if (httpCode == 200) {
    Serial.println("Device is signed up");
  } else {
    Serial.println("Sign up device error");
    signupDevice();
  }
  http.end();                                                       //Close connection
}

long deviceOnlineTimer = -3600000;
void deviceOnline(long t) {
  if (millis() - deviceOnlineTimer >= t) {
    StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
    char JSONmessageBuffer[300];
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JsonObject& JsonData = JSONencoder.createNestedObject("data");
    JsonData["esp"] = Device_id;
    JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Serial.println(JSONmessageBuffer);
    HTTPClient http;                                                  //Declare object of class HTTPClient
    http.begin("http://bhcd-line-bot-noti.herokuapp.com/device-online");  //Specify request destination
    http.addHeader("Content-Type", "application/json");               //Specify content-type header
    int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
    Serial.println(httpCode);
    String payload = http.getString();                               //Get the response payload
    Serial.println(payload);                                         //Print request response payload
    if (httpCode == 200) {
      Serial.println("Device online");
      deviceOnlineTimer = millis();
      displayHome();
    } else {
      Serial.println("ERROR");
      displayRegisterDevicePls();
      delay(10000);
    }
    //    deviceOnlineTimer = millis();
    http.end();                                                       //Close connection
  }
}

long fallingNotiTimer;
void sendFalling(int t) {
  if (millis() - fallingNotiTimer >= t) {
    StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
    char JSONmessageBuffer[300];
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JsonObject& JsonData = JSONencoder.createNestedObject("data");
    JsonData["esp"] = Device_id;
    JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Serial.println(JSONmessageBuffer);
    HTTPClient http;                                                 //Declare object of class HTTPClient
    http.begin("http://bhcd-line-bot-noti.herokuapp.com/falling");   //Specify request destination
    http.addHeader("Content-Type", "application/json");              //Specify content-type header
    int httpCode = http.POST(JSONmessageBuffer);                     //Send the request
    Serial.println(httpCode);
    String payload = http.getString();                               //Get the response payload
    Serial.println(payload);                                         //Print request response payload
    if (httpCode == 200) {
      Serial.println("FALLING!!!");
      fallingNotiTimer = millis();
    } else {
      Serial.println("ERROR");
      delay(1000);
    }
    //    fallingNotiTimer = millis();
    http.end();                                                       //Close connection
  }
}

long helpNotiTimer;
void sendHelp(int t) {
  if (millis() - helpNotiTimer >= t) {
    StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
    char JSONmessageBuffer[300];
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JsonObject& JsonData = JSONencoder.createNestedObject("data");
    JsonData["esp"] = Device_id;
    JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Serial.println(JSONmessageBuffer);
    HTTPClient http;                                                 //Declare object of class HTTPClient
    http.begin("http://bhcd-line-bot-noti.herokuapp.com/help");      //Specify request destination
    http.addHeader("Content-Type", "application/json");              //Specify content-type header
    int httpCode = http.POST(JSONmessageBuffer);                     //Send the request
    Serial.println(httpCode);
    String payload = http.getString();                               //Get the response payload
    Serial.println(payload);                                         //Print request response payload
    if (httpCode == 200) {
      Serial.println("HELP!!!");
      helpNotiTimer = millis();
    } else {
      Serial.println("ERROR");
      delay(1000);
    }
    //    helpNotiTimer = millis();
    http.end();                                                       //Close connection
  }
}

void sendHelpAck() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["esp"] = Device_id;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-line-bot-noti.herokuapp.com/help-ack");   //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                               //Get the response payload
  Serial.println(payload);                                         //Print request response payload
  if (httpCode == 200) {
    Serial.println("It's OK");
  } else {
    Serial.println("ERROR");
    delay(1000);
    sendHelpAck();
  }
  http.end();                                                       //Close connection
}

void sendHelpPreAck() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["esp"] = Device_id;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-line-bot-noti.herokuapp.com/help-pre-ack");   //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                               //Get the response payload
  Serial.println(payload);                                         //Print request response payload
  if (httpCode == 200) {
    Serial.println("It's OK");
  } else {
    Serial.println("ERROR");
    delay(1000);
    sendHelpPreAck();
  }
  http.end();                                                       //Close connection
}

void sendHealthInfo() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["esp"] = Device_id;
  JsonData["hbp"] = hbp;
  JsonData["lbp"] = lbp;
  JsonData["hr"] = hr;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-api.herokuapp.com/health-info/new");      //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                                //Get the response payload
  Serial.println(payload);                                          //Print request response payload
  if (httpCode != 201) {
    delay(1000);
    sendHealthInfo();
  } else if (httpCode >= 500) {
    return;
  }
  http.end();

  // Notification to line bot
  httpCode = -1;

  // Send data 10 rounds when not complete
  for (int i = 0; i < 10; i++) {
    if (httpCode == 200) {
      ledBlueOn();
      beep(60);
      delay(100);
      ledOff();
      beep(60);
      delay(100);
      return;
    }
    HTTPClient http;                                                    //Declare object of class HTTPClient
    http.begin("http://bhcd-line-bot-noti.herokuapp.com/health-info");  //Specify request destination
    http.addHeader("Content-Type", "application/json");                 //Specify content-type header
    httpCode = http.POST(JSONmessageBuffer);                            //Send the request
    Serial.println(httpCode);
    String payload = http.getString();                                  //Get the response payload
    Serial.println(payload);                                            //Print request response payload
    http.end();
  }
}

void sendHealthInfoOxi() {

  Serial.println(spo2Data);

  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["esp"] = Device_id;
  JsonData["spo2"] = spo2Data;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-api.herokuapp.com/health-info-oxi/new");      //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                                //Get the response payload
  Serial.println(payload);                                          //Print request response payload
  if (httpCode != 201) {
    delay(1000);
    sendHealthInfoOxi();
  } else if (httpCode >= 500) {
    return;
  }
  http.end();

  // Notification to line bot
  httpCode = -1;

  // Send data 10 rounds when not complete
  for (int i = 0; i < 10; i++) {
    if (httpCode == 200) {
      ledBlueOn();
      beep(60);
      delay(100);
      ledOff();
      beep(60);
      delay(100);
      return;
    }
    HTTPClient http;                                                    //Declare object of class HTTPClient
    http.begin("http://bhcd-line-bot-noti.herokuapp.com/health-info-oxi");  //Specify request destination
    http.addHeader("Content-Type", "application/json");                 //Specify content-type header
    httpCode = http.POST(JSONmessageBuffer);                            //Send the request
    Serial.println(httpCode);
    String payload = http.getString();                                  //Get the response payload
    Serial.println(payload);                                            //Print request response payload
    http.end();
  }
}

void sendBattLow() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  char JSONmessageBuffer[300];
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JsonObject& JsonData = JSONencoder.createNestedObject("data");
  JsonData["esp"] = Device_id;
  JsonData["batt"] = batVal;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  HTTPClient http;                                                  //Declare object of class HTTPClient
  http.begin("http://bhcd-line-bot-noti.herokuapp.com/low-batt");   //Specify request destination
  http.addHeader("Content-Type", "application/json");               //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);                      //Send the request
  Serial.println(httpCode);
  String payload = http.getString();                                //Get the response payload
  Serial.println(payload);                                          //Print request response payload
  if (httpCode == 200) {
    Serial.println("It's OK");
  } else {
    Serial.println("ERROR");
    delay(1000);
    sendBattLow();
  }
  http.end();
}

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) { //
  Serial.print("Microgear incoming message --> ");
  msg[msglen] = '\0';
  String stringOne = (char *)msg;
  Serial.println(stringOne);
  if (stringOne.equals("ACK") && state == 2 && ACK == 1) {
    sendHelpPreAck();
    ACK = 0;
  }
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Microgear Connected");
  microgear.setAlias(ALIAS);
}

#define LEDC_CHANNEL_0     0          // use first channel of 16 channels (started from zero)
#define LEDC_TIMER_13_BIT  13         // use 13 bit precission for LEDC timer
#define LEDC_BASE_FREQ     5000       // use 5000 Hz as a LEDC base frequency

void analogWrite(int channel, int value) {
  int valueMax = 255;
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);
  ledcWrite(channel, duty);
}

void tone(uint8_t channel, double freq, int time) {
  ledcWriteTone(channel, freq);
  delay(time);
  ledcWrite(channel, 0);
}

unsigned long timerBat, preTimeBat, preTime2Bat , timeOutBat , timer_sleep, preTime2sleep;
float sensorValue = 0;  // variable to store the value coming from the sensor
boolean toggleLEDBat, toggleBat;

void beep(unsigned char delayms) {
  analogWrite(LEDC_CHANNEL_0, 100);      // Almost any value can be used except 0 and 255
  delay(delayms);          // wait for a delayms ms
  analogWrite(LEDC_CHANNEL_0, 0);       // 0 turns it off
  delay(delayms);          // wait for a delayms ms
}

void web_page() {
  server.send(200, "text/html", head_form + form );
}

void info() {
  String info = "<h3 class='login-sub'>Device ID</h3> <br> " + Device_id + "<br> <h3 class='login-sub'>Default Password</h3><br> " + Device_password + "<!--<br><p>*if you want to change password you can change by line application </p><p>*you can reset passwword to default by click a button below</p>--><input  type='hidden'class='input-1' id='w' name='w' length='32' value='1' ><!--<button type='submit'>Reset Password</button><br><br>--></form><a href='/'><br><button>Back</button></a><br></div></div></body></html>";
  server.send(200, "text/html", head_form + person_infohead + info);
}

void wifi() {
  int n = WiFi.scanNetworks();
  String wifilist = "<body><div class='login'><div class='login-container'><h3 class='login-containerhead' style = 'text-align: center';>connected list</h3><table ><thead><tr><th>#</th><th>SSID</th><!--<th>PASSWORD</th>--></tr>";
  for (int i = 0; i < 4; i++) {
    wifilist += "<tr><td>" ;
    wifilist += i + 1;
    wifilist += "</td><td><a >" ;
    wifilist += ssid_list[i];
    wifilist += "</a></td><!--<td>-->";
    //      wifilist += password_list[i];
    //      wifilist += "</a></td>";
    wifilist += "</tr>";
  }
  wifilist += "</thead><tbody></tbody></table><br><br>";
  //scan wifi เธ�เธฃเธดเน€เธงเธ“เธฃเธญเธ�เน�
  wifilist += "<h3 class='login-containerhead' style = 'text-align: center';>wireless network list</h3><table ><thead><tr><th>#</th><th>SSID</th><th>quality</th></tr></thead><tbody>";

  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {

      wifilist += "<tr><td>" ;
      wifilist += i + 1;
      wifilist += "</td><td href='#p' onclick='c(this)'><a >" ;
      wifilist += WiFi.SSID(i);
      wifilist += "</a></td><td>";

      if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
        wifilist += "<span class='q '>";
      } else {
        wifilist += "<span class='q l'>";
      }
      int quality = (2 * (WiFi.RSSI(i) + 100));
      if (quality > 100) {
        quality = 100;
      }
      wifilist += String(quality) ;
      wifilist += "%</span></td>";
      wifilist += "</tr>";
    }
    wifilist += "</tbody></table><a href='/wifi'><button>Scan</button></a></div><br>";
  }
  server.send(200, "text/html", head_form + css_wifi + wifilist + wifi_tailform);
}

void reset() {
  server.send(200, "text/html",   "smarthelper will restart please wait for a while . . .");
  ESP.restart();
}

void q_buffer(String a, String b) {
  for (int i = 4 - 1; i > 0 ; i--) {
    Serial.println(i);
    ssid_list[i] = ssid_list[i - 1];
    password_list[i] = password_list[i - 1];
  }
  ssid_list[0] = a ;
  password_list[0] = b ;
}

void handle_msg() {
  int s = 0;
  int p = 0;
  Serial.println("from web");
  String ssid_msg ;
  String password_msg ;
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "s") {
      ssid_msg = server.arg(i);
      ssid_msg.trim();
      Serial.println(ssid_msg);
      Serial.println(ssid_msg.length());
      // file2.println("ssid = " + ssid_msg);
      //ssid[s] = ssid_msg;
      //s++;
    } else if (server.argName(i) == "p") {
      password_msg = server.arg(i);
      password_msg.trim();
      // file2.println("password = " + password_msg);
      // password[p] = password_msg;
      // p++;
      Serial.println(password_msg);
      Serial.println(password_msg.length());
      q_buffer(ssid_msg, password_msg);
    } else if (server.argName(i) == "apikey") {
      Api_key = server.arg(i);
      Api_key.trim();
      Serial.println(Api_key);
      Serial.println(Api_key.length());

    } else if (server.argName(i) == "userkey") {
      User_key = server.arg(i);
      User_key.trim();
      Serial.println(User_key);
      Serial.println(User_key.length());
    } else if (server.argName(i) == "r") {
      reset_pass = server.arg(i);
      reset_pass.trim();
      Serial.println("reset argument " + reset_pass);
    } else if (server.argName(i) == "ln") {
      lastname = server.arg(i);
      lastname.trim();
      Serial.println("lastname");
    } else if (server.argName(i) == "bt") {
      bloodtype = server.arg(i);
      bloodtype.trim();
    } else if (server.argName(i) == "w") {
      reset_pass = server.arg(i);
      reset_pass.trim();
      Serial.println(reset_pass);
    } else if (server.argName(i) == "h") {
      height = server.arg(i);
      height.trim();
    } else if (server.argName(i) == "bday") {
      brithday = server.arg(i);
      brithday.trim();
    }
    else if (server.argName(i) == "ad") {
      address = server.arg(i);
      address.trim();
    } else if (server.argName(i) == "mf") {
      moreinfo = server.arg(i);
      moreinfo.trim();
    }
  }
  Serial.println("from web");
  File file2 = SPIFFS.open("/test.txt", "w");
  if (!file2) {
    Serial.println("file open failed!");
  } else {
    Serial.println("file open success :)");
    for (int i = 0; i < 4; i++) {
      String ssid = ssid_list[i];
      String password = password_list[i];
      ssid.trim();
      password.trim();
      file2.println("ssid = " + ssid);
      file2.println("password = " + password);
    }
    file2.println("api key = " + Api_key);
    file2.println("user key = " + User_key);
    file2.println("reset = " +  reset_pass);
    file2.println("lastname = " + lastname);
    file2.println("bloodtype = " + bloodtype);
    file2.println("weight = " +  weight);
    file2.println("height = " +  height);
    file2.println("brithday = " +  brithday);
    file2.println("address = " +  address);
    file2.println("moreinfo = " +  moreinfo);
    file2.close();
  }
  if (reset_pass == "1") {
    server.send(200, "text/html", "smarthelper will restart please wait for a while . . .");
    ESP.restart();
  } else {
    server.send(200, "text/html", back_to_main);
  }
}

String current_ssid     = "your-ssid";
String current_password  = "your-password";

void setup_wifi() {

  displayConnectingWiFi();

  int s = 0;
  int p = 0;
  int n = 0;

  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("Starting in STA mode");
  WiFi.mode(WIFI_STA);
  for (int j = 0 ; j < 4 ; j++) {
    if (WiFi.status() != WL_CONNECTED) {
      ledGreenOn();
      delay(100);
      beep(750);
      ledOff();
      current_ssid = ssid_list[j];
      char ssid1[current_ssid.length()];
      current_ssid.toCharArray(ssid1, current_ssid.length());
      Serial.println(current_ssid.length());
      Serial.println(ssid1);

      current_password = password_list[j];
      char password1[current_password.length()];
      current_password.toCharArray(password1, current_password.length());
      //      Serial.println( current_password.length());
      //      Serial.println(password1);
      n = WiFi.scanNetworks();
      for (int k = 0; k < n ; k++) {
        if (WiFi.status() == WL_CONNECTED) {
          break;  
        }
        Serial.println("wifi scan " + WiFi.SSID(k));
        current_ssid.trim();
        current_password.trim();
        if (WiFi.SSID(k) == current_ssid) {
          WiFi.begin(ssid1, password1);
          for (int i = 0 ; i < 20; i ++) {
            if (WiFi.status() != WL_CONNECTED) {
              ledOff();
              delay(250);
              beep(100);
              Serial.print(".");
              ledGreenOn();
              delay(250);
            } else {
              break;
            }
          }
        }
      }
    } else {
      Serial.print("Connecting Wifi Ending");
      //      j = 5;
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    //digitalWrite(wifi_led, HIGH);
    ledGreenOn();
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    signupDevice();
    displayConnectedWiFi();
  } else {
    displayFailConnectWiFi();
  }

  delay(3000);
}


void prepareFile() {
  int s = 0;
  int p = 0;
  Serial.println("Prepare file system");
  SPIFFS.begin();

  File file = SPIFFS.open("/test.txt", "r");
  if (!file) {
    Serial.println("file open failed!");
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");

    // open file for writing
    File f = SPIFFS.open("/test.txt", "w");
    if (!f) {
      Serial.println("file open failed");
    }
    Serial.println("====== Writing to SPIFFS file =========");
    f.close();
  } else {
    Serial.println("file open success:)");
    //  Serial.write(file.read());
    Serial.println("====== file have data =========");
    while (file.available()) {
      //  Serial.write(file.read());
      //Lets read line by line from the file
      String line = file.readStringUntil('\n');
      if (line.startsWith("ssid = ")) {
        ssid_list[s] = line.substring(7);
        current_ssid = ssid_list[s];
        current_ssid.trim();
        //  Serial.println(ssid[sizeof(ssid)]);
        char ssid[current_ssid.length()];
        current_ssid.toCharArray(ssid, current_ssid.length());
        Serial.println(current_ssid.length());
        Serial.println(current_ssid);
        s++;
      } else if (line.startsWith("password = ")) {
        password_list[p] = line.substring(11);
        current_password = password_list[p];
        current_password.trim();
        char password1[current_password.length()];
        current_password.toCharArray(password1, current_password.length());
        //        Serial.println(current_password.length());
        //        Serial.println(current_password);
        p++;
      } else if (line.startsWith("api key = ")) {
        Api_key = line.substring(10);
        Api_key.trim();
        //        Serial.println(Api_key.length());
        //        Serial.println(Api_key);
      } else if (line.startsWith("user key = ")) {
        User_key = line.substring(11);
        User_key.trim();
        //        Serial.println(User_key.length());
        //        Serial.println(User_key);
      } else if (line.startsWith("reset = ")) {
        reset_pass = line.substring(8);
        reset_pass.trim();
        //        Serial.println(reset_pass.length());
        //        Serial.println(reset_pass);
      } else if (line.startsWith("lastname = ")) {
        lastname = line.substring(11);
        lastname.trim();
        //        Serial.println(lastname.length());
        //        Serial.println(lastname);
      } else if (line.startsWith("bloodtype = ")) {
        bloodtype = line.substring(12);
        bloodtype.trim();
        //        Serial.println(bloodtype.length());
        //        Serial.println(bloodtype);
      } else if (line.startsWith("weight = ")) {
        weight = line.substring(9);
        weight.trim();
        //        Serial.println(weight.length());
        //        Serial.println(weight);
      } else if (line.startsWith("height = ")) {
        height = line.substring(9);
        height.trim();
        //        Serial.println(height.length());
        //        Serial.println(height);
      } else if (line.startsWith("brithday = ")) {
        brithday = line.substring(11);
        brithday.trim();
        //        Serial.println(brithday.length());
        //        Serial.println(brithday);
      } else if (line.startsWith("address = ")) {
        address = line.substring(10);
        address.trim();
        //        Serial.println(address.length());
        //        Serial.println(address);
      } else if (line.startsWith("moreinfo = ")) {
        moreinfo = line.substring(11);
        moreinfo.trim();
        //        Serial.println(moreinfo.length());
        //        Serial.println(moreinfo);
      }
    }
    if (s == 3) {
      s = 0;
      p = 0;
    }
  }
  file.close();
}
/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(ssid) + ".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send (302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

void setup_apmode() {
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  Serial.print("visit: \n");
  Serial.println(WiFi.softAPIP());
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", web_page   );
  server.on("/i", info   );
  server.on("/r", reset);
  server.on("/wifisave", handle_msg);
  server.on("/wifi", wifi);                          // And as regular external functions:
  server.onNotFound(handleNotFound);
  server.begin();                                    // Start the server
}

bool isSiren = false;
bool isSiren2 = false;

void siren() {
  isSiren = true;
  isSiren2 = false;
}

void sirenTask(void *p) {
  while (1) {
    if (isSiren) {
      for (int freq = 500; freq < 1800; freq += 5) {
        //    digitalWrite(green_led, LOW);
        ledGreenOn();
        pinMode(vibration_motor, OUTPUT);
        digitalWrite(vibration_motor , HIGH);
        tone(LEDC_CHANNEL_0, freq, timeBuzzer);     // Beep pin, freq, time
        delay(5);
      }
      for (int freq = 1800; freq > 500; freq -= 5) {
        tone(LEDC_CHANNEL_0, freq, timeBuzzer);     // Beep pin, freq, time
        //    digitalWrite(green_led, HIGH);
        ledOff();
        pinMode(vibration_motor, INPUT);
        delay(5);
      }
    } else {
      delay(200);
    }
  }
}

void siren2() {
  isSiren = false;
  isSiren2 = true;
}

void siren2Task(void *p) {
  while (1) {
    if (isSiren2) {
      for (int freq = 600; freq < 1200; freq += 5) {
        //    digitalWrite(green_led, LOW);
        ledGreenOn();
        pinMode(vibration_motor, OUTPUT);
        digitalWrite(vibration_motor , HIGH);
        ledOff();
        tone(LEDC_CHANNEL_0, freq, timeBuzzer);     // Beep pin, freq, time
        delay(5);
      }
      for (int freq = 1200; freq > 600; freq -= 5) {
        tone(LEDC_CHANNEL_0, freq, timeBuzzer);     // Beep pin, freq, time
        //    digitalWrite(green_led, HIGH);
        ledOff();
        pinMode(vibration_motor, INPUT);
        delay(5);
      }
    } else {
      delay(200);
    }
  }
}

void stopSiren() {
  isSiren = false;
  isSiren2 = false;
  ledGreenOn();
  pinMode(vibration_motor, INPUT);
  analogWrite(buzzer, 0);
}

void doInt() {
  timeOut = timer;
  state = 10;
}

void checkSettingsMPU() {
  Serial.println();

  Serial.print(" * Sleep Mode:                ");
  Serial.println(mpu.getSleepEnabled() ? "Enabled" : "Disabled");

  Serial.print(" * Motion Interrupt:     ");
  Serial.println(mpu.getIntMotionEnabled() ? "Enabled" : "Disabled");

  Serial.print(" * Zero Motion Interrupt:     ");
  Serial.println(mpu.getIntZeroMotionEnabled() ? "Enabled" : "Disabled");

  Serial.print(" * Free Fall Interrupt:       ");
  Serial.println(mpu.getIntFreeFallEnabled() ? "Enabled" : "Disabled");

  Serial.print(" * Free Fal Threshold:          ");
  Serial.println(mpu.getFreeFallDetectionThreshold());

  Serial.print(" * Free FallDuration:           ");
  Serial.println(mpu.getFreeFallDetectionDuration());

  Serial.print(" * Clock Source:              ");
  switch (mpu.getClockSource())
  {
    case MPU6050_CLOCK_KEEP_RESET:     Serial.println("Stops the clock and keeps the timing generator in reset"); break;
    case MPU6050_CLOCK_EXTERNAL_19MHZ: Serial.println("PLL with external 19.2MHz reference"); break;
    case MPU6050_CLOCK_EXTERNAL_32KHZ: Serial.println("PLL with external 32.768kHz reference"); break;
    case MPU6050_CLOCK_PLL_ZGYRO:      Serial.println("PLL with Z axis gyroscope reference"); break;
    case MPU6050_CLOCK_PLL_YGYRO:      Serial.println("PLL with Y axis gyroscope reference"); break;
    case MPU6050_CLOCK_PLL_XGYRO:      Serial.println("PLL with X axis gyroscope reference"); break;
    case MPU6050_CLOCK_INTERNAL_8MHZ:  Serial.println("Internal 8MHz oscillator"); break;
  }

  Serial.print(" * Accelerometer:             ");
  switch (mpu.getRange())
  {
    case MPU6050_RANGE_16G:            Serial.println("+/- 16 g"); break;
    case MPU6050_RANGE_8G:             Serial.println("+/- 8 g"); break;
    case MPU6050_RANGE_4G:             Serial.println("+/- 4 g"); break;
    case MPU6050_RANGE_2G:             Serial.println("+/- 2 g"); break;
  }

  Serial.print(" * Accelerometer offsets:     ");
  Serial.print(mpu.getAccelOffsetX());
  Serial.print(" / ");
  Serial.print(mpu.getAccelOffsetY());
  Serial.print(" / ");
  Serial.println(mpu.getAccelOffsetZ());

  Serial.print(" * Accelerometer power delay: ");
  switch (mpu.getAccelPowerOnDelay())
  {
    case MPU6050_DELAY_3MS:            Serial.println("3ms"); break;
    case MPU6050_DELAY_2MS:            Serial.println("2ms"); break;
    case MPU6050_DELAY_1MS:            Serial.println("1ms"); break;
    case MPU6050_NO_DELAY:             Serial.println("0ms"); break;
  }

  Serial.println();
}

class MyBLEClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.print("onConnect on state : ");
      Serial.println(bleState);
    }

    void onDisconnect(BLEClient* pclient) {
      Serial.print("onDisconnect : ");
      Serial.println(bleState);
      if (bleState >= 2 && bleState <= 5) {
        bleState = 6;
      } else if (bleState >= 6 && bleState <= 9) {
        displayHome();
        bleState = 0;
      }
    }
};

void bleConnectToServer(BLEAddress pAddress) {
  bleConnectingTimer = millis();

  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());

  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyBLEClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  Serial.println(serviceUUID.toString().c_str());
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  notifyChar = pRemoteService->getCharacteristic(notifyUUID);
  writeChar = pRemoteService->getCharacteristic(writeUUID);

  Serial.println(notifyUUID.toString().c_str());
  if (notifyChar == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(notifyUUID.toString().c_str());
    return;
  }
  Serial.println(writeUUID.toString().c_str());
  if (writeChar == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(writeUUID.toString().c_str());
    return;
  }

  notifyChar->registerForNotify(notifyCallback);

  const uint8_t notificationOnline[] = {0x1, 0x0};
  notifyChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOnline, 2, true);

  const byte startPressure[] = {0xFD, 0xFD, 0xFA, 0x05, 0x0D, 0x0A};
  writeChar->writeValue((uint8_t*)startPressure, 6, true);

  // BLE Connected
  bleState = 5;
}

void bleConnectToServerOxi(BLEAddress pAddress) {
  bleConnectingTimer = millis();

  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());

  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyBLEClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceOxiUUID);
  Serial.println(serviceOxiUUID.toString().c_str());
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceOxiUUID.toString().c_str());
    return;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  notifyChar = pRemoteService->getCharacteristic(notifyOxiUUID);
  writeChar = pRemoteService->getCharacteristic(writeOxiUUID);

  Serial.println(notifyOxiUUID.toString().c_str());
  if (notifyChar == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(notifyUUID.toString().c_str());
    return;
  }
  Serial.println(writeOxiUUID.toString().c_str());
  if (writeChar == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(writeUUID.toString().c_str());
    return;
  }

  notifyChar->registerForNotify(notifyOxiCallback);

  const uint8_t notificationOnline[] = {0x1, 0x0};
  notifyChar->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOnline, 2, true);

  const byte startPressure[] = {0, -95, 17, 0};
  writeChar->writeValue((uint8_t*)startPressure, 6, true);

  // BLE Connected
  oxiCounter = 0;
  oxiSum = 0;
  bleState = 9;
}

/**
   Scan for BLE servers and find the first one that advertises the address we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //      Serial.print("BLE Advertised Device found: ");
      //      Serial.println(advertisedDevice.toString().c_str());
      if (bleState == 2) {
        //        digitalWrite(blue_led, LOW);
        ledBlueOn();
        delay(50);
        //        digitalWrite(blue_led, HIGH);
        ledOff();
        delay(50);
        if (advertisedDevice.getAddress().equals(bpMacAddress)) {
          advertisedDevice.getScan()->stop();
          Serial.println("Found our blood pressure!");
          //        Serial.println(advertisedDevice.getAddress().toString().c_str());
          pServerAddress = new BLEAddress(advertisedDevice.getAddress());
          bleState = 3;
        }
      } else if (bleState == 6) {
        //        digitalWrite(blue_led, LOW);
        ledBlueOn();
        delay(50);
        //        digitalWrite(blue_led, HIGH);
        ledOff();
        delay(50);
        if (advertisedDevice.getAddress().equals(oxiMacAddress)) {
          advertisedDevice.getScan()->stop();
          Serial.println("Found our oximeter!");
          pServerAddress = new BLEAddress(advertisedDevice.getAddress());
          bleState = 7;
        }
      }
    }
};

int stateWF;
int stateBT;
int stateAC;

void displayWelcome() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println(F("WELCOME"));
  display.display();
  delay(2000);
}

void displayConnectingWiFi() {
  display.clearDisplay();
  stateWF = 0;
  stateBT = 1;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("CONNECTING"));
  display.println(F("WIFI"));
  display.display();
}

void displayConnectedWiFi() {
  display.clearDisplay();
  stateWF = 0;
  stateBT = 1;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("CONNECTED"));
  display.println(F("WIFI"));
  display.display();      // Show initial text
  delay(3000);
}

void displayFailConnectWiFi() {
  display.clearDisplay();
  stateWF = 0;
  stateBT = 1;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("FAILED TO"));
  display.println(F("WIFI"));
  display.display();      // Show initial text
  delay(3000);
}

void displayHome() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("BASIC"));
  display.println(F("HEALTH"));
  display.display();      // Show initial text
}

void displayBTScan() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("BT BP"));
  display.println(F("SCAN"));
  display.display();      // Show initial text
}

void displayBTScanFailed() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("BP SCAN"));
  display.println(F("FAIL"));
  display.display();      // Show initial text
}

void displayConnectingBP() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("CONNECTING"));
  display.println(F("BP"));
  display.display();      // Show initial text
}

void displayMeasureBP() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("MEASURE"));
  display.println(F("BP"));
  display.display();      // Show initial text
}

void displayBTOxiScan() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("BT OXI"));
  display.println(F("SCAN"));
  display.display();      // Show initial text
}

void displayBTOxiScanFailed() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("OXI SCAN"));
  display.println(F("FAIL"));
  display.display();      // Show initial text
}

void displayConnectingOxi() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("CONNECTING"));
  display.println(F("OXIMETER"));
  display.display();      // Show initial text
}

void displayMeasureOxi() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("MEASURE"));
  display.println(F("OXIMETER"));
  display.display();      // Show initial text
}

void displayHealthInfo() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println("SYS/DIS : ");
  display.println(String(hbp) + "/" + String(lbp));
  display.println("PULSE : " + String(hr));
  display.display();      // Show initial text
}

void displayHealthInfoOxi() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  displayBatt();
  iconWiFi();
  iconActive();
  iconBT();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println("SPO2 : " + String(spo2Data));
  display.display();
}

void displayConnectedBT() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 0;
  stateAC = 1;
  iconWiFi();
  displayBatt();
  iconBT();
  iconActive();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("FOUND"));
  display.println(F("Device BLE"));
  display.display();      // Show initial text
  delay(2000);
}

void displayFalling() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.display();
  display.setTextSize(3); // Draw 2X-scale text
  display.setCursor(0, 20);
  display.setTextColor(INVERSE);
  display.println(F("FALLING!!"));
  delay(1000);
  display.display();      // Show initial text
}

void displayHelp() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.display();
  display.setTextSize(3); // Draw 2X-scale text
  display.setCursor(0, 20);
  display.setTextColor(INVERSE);
  display.println(F("HELP!!"));
  delay(1000);
  display.display();      // Show initial text
}

void displayOK() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.setTextSize(3); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("OK!!"));
  display.display();      // Show initial text
}

void displayAP() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println(F("AP MODE"));
  display.display();      // Show initial text
}

void displayRegisterDevicePls() {
  display.clearDisplay();
  stateWF = 1;
  stateBT = 1;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println("REGISTER");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(Device_id);
  display.display();
}

void displayBattLow() {
  display.clearDisplay();
  stateWF = 0;
  stateBT = 0;
  stateAC = 0;
  displayBatt();
  iconWiFi();
  iconBT();
  iconActive();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);
  display.println("LOW BATT");
  display.setTextColor(WHITE);
  display.println(String(batVal) + "%");
  display.display();
}

// Connection and icon
void displayBatt() {
  display.setTextSize(1); // Draw 2X-scale tex
  display.setTextColor(WHITE);
  display.setCursor(100, 1);
  display.print(batVal);
  display.print(F(" %"));
}

void iconWiFi() {
  if (stateWF == 0) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(BLACK, WHITE);
    display.setCursor(1, 1);
    delay(500);
    display.print(F("WiFi"));
    delay(500);
  }
  else if (stateWF == 1) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(WHITE);
    display.setCursor(1, 1);
    display.print(F("WiFi"));
  }
  //  display.display();      // Show initial text
}

void iconBT() {
  if (stateBT == 0) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(BLACK, WHITE);
    display.setCursor(30, 1);
    display.print(F("BT"));
    //  display.display();      // Show initial text
  }
  else if (stateBT == 1) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(WHITE);
    display.setCursor(30, 1);
    display.print(F("BT"));
  }
}

void iconActive() {
  if (stateAC == 0) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(BLACK, WHITE);
    display.setCursor(50, 1);
    display.print(F("MAIN"));
  }
  else if (stateAC == 1) {
    display.setTextSize(1); // Draw 2X-scale tex
    display.setTextColor(WHITE);
    display.setCursor(50, 1);
    display.print(F("MAIN"));
  }

  //  display.display();      // Show initial text
}

long bmeTimer = 1000;
double bmeHeightNow = 0;
void bmeNow() {
  if (millis() - bmeTimer >= 800 && state != 10) {
    double allHeight = 0;
    int hCount = 0;
    for (int i = 0; i < 20; i++) {
      bme.takeForcedMeasurement();
      double h = bme.readAltitude(SEALEVELPRESSURE_HPA);
      if (h > 0) {
        allHeight += h;
        hCount++;
      }
      bmeTimer = millis();
      delay(10);
    }
    if (hCount > 0)
      bmeHeightNow = allHeight / hCount;
  }
}

void setup() {

  Device_id.toCharArray(ALIAS, sizeof(ALIAS));

  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);

  //  timer1_disable();
  //  timer1_attachInterrupt(onTimerISR);
  //  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  //  timer1_write(300000000); //120000 us

  Serial.begin(115200);
  Wire.begin();//New version
  Serial.println("Initialize MPU");

  //#####verify I2C connection#####
  Serial.println("Testing device connections...");
  while (!mpu.beginSoftwareI2C(scl, sda, MPU6050_SCALE_2000DPS, MPU6050_RANGE_16G, 0x68) && !mpu.beginSoftwareI2C(scl, sda, MPU6050_SCALE_2000DPS, MPU6050_RANGE_16G, 0x69)) {
    Serial.println("I2C Error");
    pinMode(vibration_motor, OUTPUT);
    digitalWrite(vibration_motor, LOW);
    delay(1000);
  }
  //#####End verify I2C connection#####

  //#####initialize IMU MPU6050#####
  mpu.setAccelPowerOnDelay(MPU6050_DELAY_3MS);
  mpu.setIntFreeFallEnabled(true);
  mpu.setIntZeroMotionEnabled(false);
  mpu.setIntMotionEnabled(false);
  mpu.setDHPFMode(MPU6050_DHPF_5HZ);
  mpu.setFreeFallDetectionThreshold(FreefallDetectionThreshold);
  mpu.setFreeFallDetectionDuration(FreefallDetectionDuration);
  checkSettingsMPU();                                               //Serial port Debuging MPU6050 status
  //#####End initialize IMU MPU6050#####

  //#####interrupt
  attachInterrupt(digitalPinToInterrupt(int_1), doInt, RISING);


  //  while (!bmp.begin(BMP280_ADDRESS) && !bmp.begin(BMP280_ADDRESS_ALT)) {
  //    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
  //    delay(1000);
  //  }
  //  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
  //                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
  //                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
  //                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
  //                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  while (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }
  Serial.println("-- Indoor Navigation Scenario --");
  Serial.println("normal mode, 16x pressure / 2x temperature / 1x humidity oversampling,");
  Serial.println("0.5ms standby period, filter 16x");
  bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                  Adafruit_BME280::SAMPLING_X2,  // temperature
                  Adafruit_BME280::SAMPLING_X16, // pressure
                  Adafruit_BME280::SAMPLING_X1,  // humidity
                  Adafruit_BME280::FILTER_X16,
                  Adafruit_BME280::STANDBY_MS_0_5 );

  // ##### OLED #####
  oledTwoWire.begin(16, 17);
  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    delay(1000);
  }
  displayWelcome();

  //#####initialize input/output#####
  //vibration active Low
  pinMode(vibration_motor, OUTPUT);
  digitalWrite(vibration_motor, HIGH);
  pinMode(confirm_button, INPUT);
  pinMode(green_led, OUTPUT);
  //  digitalWrite(green_led, HIGH);
  pinMode(red_led, OUTPUT);
  //  digitalWrite(red_led, HIGH);
  pinMode(battery_adc, INPUT);
  pinMode(buzzer , OUTPUT);
  digitalWrite(buzzer, LOW);
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(buzzer, LEDC_CHANNEL_0);
  //#####End initialize input/output#####

  prepareFile();  //SSID PASS File

  beep(50);
  beep(50);
  timeOut = millis();

  // BLE
  pinMode(blue_led, OUTPUT);
  //  digitalWrite(blue_led, LOW);
  pinMode(ble_button, INPUT);
  BLEDevice::init("");

  // Siren Task
  xTaskCreate(&sirenTask, "sirenTask", 2048, NULL, 5, NULL);
  xTaskCreate(&siren2Task, "siren2Task", 2048, NULL, 6, NULL);

  ledOff();
}

void loop() {
  timer = millis();
  buttonState = digitalRead(confirm_button);
  bmeNow();
  /*#######################################################
    เธ—เธณเธ�เธฒเธ�เธซเธฅเธฑเธ�เธ�เธฒเธ�เน€เธ�เธฃเธทเน�เธญเธ�เธ�เธฃเน�เธญเธกเน�เธ�เน�เธ�เธฒเธ� 10 เธงเธดเธ�เธฒเธ—เธต
    pre_program_mode | เน€เธ�เธฅเธตเน�เธขเธ�เน€เธกเธทเน�เธญ           | เน€เธ�เน�เธฒเธชเธนเน�เน�เธซเธกเธ”
    -----------------+--------------------
    0                | defualt           |    -
    1                | เธ�เธ”เธ�เธธเน�เธก4เธงเธดเธ�เธฒเธ—เธต         | AP mode
    2                | เน�เธกเน�เธกเธตเธ�เธฒเธฃเธ�เธ”เธ�เธธเน�เธก       | Normal mode
    #######################################################*/
  if (pre_program_mode == 0) {
    //    digitalWrite(green_led, LOW);
    ledGreenOn();
    if (((timer - timeOut) / 1000) < 10) {
      if (buttonState == HIGH) {
        unsigned long timerAck = ((timer - preTime) / 1000);
        if (timerAck >= 3) {
          delay(50);
          pre_program_mode = 1;
        }
      } else {
        //        digitalWrite(green_led , LOW);
        ledGreenOn();
        delay(500);
        //        digitalWrite(green_led , HIGH);
        ledOff();
        delay(500);
        preTime = timer;

      }
    } else {
      pre_program_mode = 2;
    }
  } else if (pre_program_mode == 1) {
    displayAP();
    if (start_ap == 1) {
      setup_apmode();
      start_ap = 0;
      beep(250);
      beep(50);
      beep(50);
    }
    dnsServer.processNextRequest();
    server.handleClient();
    //    digitalWrite(green_led, LOW);
    ledGreenOn();
    delay(250);
    //    digitalWrite(green_led, HIGH);
    ledOff();
    delay(250);
  } else if (pre_program_mode == 2) {
    if (start_ap == 1) {
      start_ap = 0;
      //      digitalWrite(green_led , LOW);
      ledGreenOn();
      setup_wifi();
      microgear.init(KEY, SECRET, ALIAS);
      microgear.connect(APPID);
      if (reset_pass == "1") {
        //        send_json("RESET", "Password has been change");
        reset_pass = "0";
        handle_msg();
        Serial.println("--------------------------- password reset----------------------------");
      }
      // ?
    }

    // wifi check disconnect
    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
    } else {
      if (microgear.connected()) {
        microgear.loop();
      } else {
        microgear.init(KEY, SECRET, ALIAS);
        microgear.connect(APPID);
        microgear.loop();
      }

      deviceOnline(3600000);

      if (state == 0) {
        if (bleState == 0) {
          //        digitalWrite(blue_led, HIGH);
          ledOff();
          if (digitalRead(ble_button) == HIGH) {
            Serial.println("Pressing ble button");
            bleState = 1;
            bleBtnTimer = timer;
          }
        } else if (bleState == 1) {
          if (digitalRead(ble_button) == LOW) {
            Serial.println("Press ble button again !!");
            bleState = 0;
          } else if (timer - bleBtnTimer >= 1000 && digitalRead(ble_button) == HIGH) {
            Serial.println("BLE Starting ...");
            bleState = 2;
          }
        } else if (bleState == 2) {
          //        Serial.println("BLE scanning ...");
          displayBTScan();
          BLEScan *pBLEScan = BLEDevice::getScan();
          pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
          pBLEScan->setActiveScan(true);
          pBLEScan->start(30);
          pBLEScan->clearResults();
          if (bleState == 2) {
            displayBTScanFailed();
            delay(5000);
            bleState = 6;
          }
        } else if (bleState == 3) {
          Serial.println("BLE BP connecting ...");
          bleState = 4;
          bleConnectToServer(*pServerAddress);
        } else if (bleState == 4) {
          //          Serial.println("BLE waiting ...");
          ledBlueOn();
          delay(100);
          ledOff();
          delay(100);
          displayConnectingBP();
          if (timer - bleConnectingTimer >= 10000) {
            bleState = 6;
          }
        } else if (bleState == 5) {
          //          Serial.println("BLE measuring ...");
          displayMeasureBP();
          //        digitalWrite(blue_led, LOW);
          ledBlueOn();
        } else if (bleState == 6) {
          displayBTOxiScan();
          BLEScan *pBLEScan = BLEDevice::getScan();
          pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
          pBLEScan->setInterval(1349);
          pBLEScan->setWindow(449);
          pBLEScan->setActiveScan(true);
          pBLEScan->start(30);
          pBLEScan->clearResults();
          if (bleState == 6) {
            displayBTOxiScanFailed();
            delay(5000);
            displayHome();
            bleState = 0;
          }
        } else if (bleState == 7) {
          Serial.println("BLE Oxi connecting ...");
          bleState = 8;
          bleConnectToServerOxi(*pServerAddress);
        } else if (bleState == 8) {
          //          Serial.println("BLE waiting ...");
          //        digitalWrite(blue_led, LOW);
          ledBlueOn();
          delay(100);
          //        digitalWrite(blue_led, HIGH);
          ledOff();
          delay(100);
          displayConnectingOxi();
          if (timer - bleConnectingTimer >= 10000) {
            bleState = 0;
          }
        } else if (bleState == 9) {
          displayMeasureOxi();
          //        digitalWrite(blue_led, LOW);
          ledBlueOn();
        } else if (bleState == 10) {
          ledBlueOn();
          displayHealthInfo();
          sendHealthInfo();
          delay(5000);
          bleState = 6;
        } else if (bleState == 11) {
          ledBlueOn();
          displayHealthInfoOxi();
          sendHealthInfoOxi();
          delay(5000);
          displayHome();
          bleState = 0;
        }
      }

      if (state == 0) {
        // State 0 : Normal mode and press button
        //        digitalWrite(green_led, LOW);
        ledGreenOn();
        if (buttonState == HIGH) {
          unsigned long timerAck = ((timer - preTime) / 1000);
          if (timerAck >= 1.0) {
            state = 3;
          }
        } else {
          preTime = timer;
        }
      } else if (state == 10) {
        delay(3000);
        double allHeight = 0;
        int hCount = 0;
        for (int i = 0; i < 20; i++) {
          bme.takeForcedMeasurement();
          double h = bme.readAltitude(SEALEVELPRESSURE_HPA);
          if (h > 0) {
            allHeight += h;
            hCount++;
          }
          bmeTimer = millis();
          delay(10);
        }
        double bmeInt = 0;
        if (hCount > 0)
          bmeInt = allHeight / hCount;
        double deltaH = bmeHeightNow - bmeInt;
        Serial.print("Falling!! Delta Height : ");
        Serial.println(deltaH);
        microgear.chat("HTML-BHCD", deltaH);
        if (deltaH > 0.3 || deltaH < -0.3) {
          state = 1;
        } else {
          state = 0;
        }
      } else if (state == 1) {
        // State 1 : Detect free falling
        buttonState = digitalRead(confirm_button);

        // Wait 8 sec
        if (((timer - timeOut) / 1000) <= 8) {
          //          digitalWrite(green_led, LOW);
          ledGreenOn();
          pinMode(vibration_motor, OUTPUT);
          digitalWrite(vibration_motor , HIGH);
          delay(500);
          //          digitalWrite(green_led, HIGH);
          ledOff();
          pinMode(vibration_motor, INPUT);
          delay(500);

          // Press more than 1 sec to cancel
          if (buttonState == HIGH) {
            unsigned long timerAck = ((timer - preTime) / 1000);
            if (timerAck >= 1.0) {
              //              digitalWrite(green_led, HIGH);
              ledOff();
              digitalWrite(vibration_motor , HIGH);
              delay(50);
              state = 4;
            }
          } else {
            preTime = timer;
          }
        } else {
          // Real falling
          DETECT = "FALL";
          ACK = 1;
          state = 2;
        }
      } else if (state == 2) {
        // State 2 : send notification
        if (ACK == 1) {
          if (DETECT == "FALL") {
            sendFalling(30000);
            displayFalling();
          } else if (DETECT == "PRESS") {
            sendHelp(30000);
            displayHelp();
          }
          siren();
        } else if (ACK == 0) {
          siren2();
        }

        // Press more than 1 sec to cancel
        buttonState = digitalRead(confirm_button);
        if (buttonState == HIGH) {
          unsigned long timerAck = ((timer - preTime) / 1000);
          if (timerAck >= 1) {
            sendHelpAck();
            stopSiren();
            state = 4;
          }
        } else {
          preTime = timer;
        }
      } else if (state == 3) {
        // State 3 : if real press send PRESS ack
        buttonState = digitalRead(confirm_button);
        if (buttonState == HIGH) {
          DETECT = "PRESS";
          ACK = 1;
          state = 2;
        }
      } else if (state == 4) {
        // State 4 : if real press to stop notification
        buttonState = digitalRead(confirm_button);
        if (buttonState == HIGH) {
          state = 5;
        } else {
          state = 2;
        }
      } else if (state == 5) {
        // State 5 : Blink & beep after stop notification
        if (buttonState == LOW) {
          ledGreenOn();
          beep(60);
          delay(100);
          ledOff();
          beep(60);
          delay(100);
          ledGreenOn();
          state = 0;
          displayHome();
        }
      }
      //read_battery_milsec(600000);
      readBattery();
    }
  }
}
