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

MPU6050 mpu;                      // MPU6050 Init
const uint8_t scl = 22;           // SCL Pin
const uint8_t sda = 21;           // SDA Pin
const uint8_t int_1 = 25;         // Interrupt Pin
const uint8_t FreefallDetectionThreshold = 50;      // Freefall detection threshold
const uint8_t FreefallDetectionDuration = 150;      // Freefall detection duration

#define vibration_motor  27       // Vibrartion motor pin
#define buzzer           26       // Buzzer pin
#define green_led        33       // Green LED Pin
#define red_led          32       // Red LED Pin
#define blue_led         36       // Blue LED Pin

#define confirm_button   34       // Select Mode Button
#define battery_adc      35       // Battery Read Pin
#define ble_button       39       // BLE Button
#define timeBuzzer       10       // Time for buzzer delay 

const char* host = "http://numpapick.herokuapp.com/bot.php";    // Numpapick API host
#define APPID      "numpapicklinebot"                           // Change this to your APPID
#define KEY        "U12xgTIw6lD02Sy"                            // Change this to your KEY
#define SECRET     "IHdml3MkM9OKCXXBG4MZ9tbNl"                  // Change this to your SECRET

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
int lenChipidBuffer = sprintf(chipidBuffer, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
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

// ThingSpeak Settings //
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "2TMDRASPYKMA8KRF";
WiFiClient client2;
// ThingSpeak Settings //

String uid = "";
MicroGear microgear(client);



void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) { //
  Serial.print("Incoming message -->");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
  String stringOne = (char *)msg;
  String stringTwo = "CHECK";
  if (stringOne.equals("CHECK")) {
    send_json("CHECK", "CHECK");
  } else if (stringOne.equals("ACK") && state == 2) {
    ACK = 0;
    send_json("DEVICEACK","DEVICEACK");
    Serial.print("Incoming message Ending2-->");
  }
  Serial.print("Incoming message Ending3-->");
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  //microgear.setName(ALIAS);
  microgear.setName("ALIAS");
  send_json("REGISTER", "Device Online");
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

int8_t readbattery = 1 ;
void read_battery_milsec(unsigned long t) {
  timerBat = millis();
  if (readbattery == 1) {
    sensorValue = analogRead( battery_adc);
    preTimeBat = millis();
    if (sensorValue < 820 ) {//น้อยกว่า3.5V จะเตือน//837
      Serial.println(sensorValue);//*3.3/1024
      digitalWrite(red_led, HIGH);
      if (WiFi.status() == WL_CONNECTED) {
        send_json("CHECK", "LOW");
      }
       
    }else if(sensorValue < 850 ) {
      digitalWrite(red_led, HIGH);
    }
    else {
      digitalWrite(red_led, LOW);
    }
  }

  if (timerBat - preTimeBat > t) {
    sensorValue = analogRead( battery_adc);
    Serial.println(timerBat - preTimeBat);
    if (sensorValue < 820 ) {//น้อยกว่า3.5V จะเตือน//837
      Serial.println(sensorValue);//*3.3/1024
      digitalWrite(red_led, HIGH);
      if (WiFi.status() == WL_CONNECTED) {
        send_json("CHECK", "LOW");
      }
       
    }else if(sensorValue < 850 ) {
      digitalWrite(red_led, HIGH);
    }
    else {
      digitalWrite(red_led, LOW);
    }
    preTimeBat = millis();

  } else {
    readbattery = 0;
  }

}


void check_status(unsigned long t ) {
  timerBat = millis();
  if (timerBat - preTime2Bat > t) {
    preTime2Bat = timerBat;
    send_json("CHECK", "CHECK");
    preTime2Bat = timerBat;
  }
}

void send_notification(unsigned long t) {
  timerBat = millis();
  if (timerBat - preTime2Bat > t) {
    preTime2Bat = timerBat;
    send_json("HELP", DETECT);
    preTime2Bat = timerBat;
  }
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
   for(int i = 0;i < 4;i++){
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
   //scan wifi บริเวณรอบๆ
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
void handle_msg()
{
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
    Serial.println("file open success:)");
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
    server.send(200, "text/html",   "smarthelper will restart please wait for a while . . .");
    ESP.restart();
  } else {
    server.send(200, "text/html", back_to_main);
  }
}

String current_ssid     = "your-ssid";
String  current_password  = "your-password";

void setup_wifi() {


  int s = 0;
  int p = 0;
  int n = 0;

  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("Starting in STA mode");
  WiFi.mode(WIFI_STA);
  for (int j = 0 ; j < 4 ; j++) {
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(green_led, HIGH);
      delay(100);
      beep(750);
      digitalWrite(green_led, LOW);
      current_ssid = ssid_list[j];
      char ssid1[current_ssid.length()];
      current_ssid.toCharArray(ssid1, current_ssid.length());
      Serial.println(current_ssid.length());
      Serial.println(ssid1);
      current_password = password_list[j];

      char password1[current_password.length()];
      current_password.toCharArray(password1, current_password.length());
      Serial.println( current_password.length());
      Serial.println(password1);
      n = WiFi.scanNetworks();
      for (int k = 0; k < n ; k++) {
        Serial.println("wifi scan " + WiFi.SSID(k));
        current_ssid.trim();
        current_password.trim();
        if (WiFi.SSID(k) == current_ssid) {
          WiFi.begin(ssid1, password1);
          for (int i = 0 ; i < 20; i ++) {
            if (WiFi.status() != WL_CONNECTED) {
              digitalWrite(green_led, LOW);
              delay(250);
              beep(100);
              Serial.print(".");
              digitalWrite(green_led, HIGH);
              delay(250);

            }

          }
        }

      }
    } else {
      Serial.print("Ending");
      j = 5;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    //digitalWrite(wifi_led, HIGH);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

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
    while (file.available()) {
      //  Serial.write(file.read());
      //Lets read line by line from the file
      Serial.println("====== file have data =========");
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
        Serial.println(current_password.length());
        Serial.println(current_password);

        p++;
      } else if (line.startsWith("api key = ")) {
        Api_key = line.substring(10);
        Api_key.trim();
        Serial.println(Api_key.length());
        Serial.println(Api_key);

      } else if (line.startsWith("user key = ")) {
        User_key = line.substring(11);
        User_key.trim();
        Serial.println(User_key.length());
        Serial.println(User_key);
      } else if (line.startsWith("reset = ")) {
        reset_pass = line.substring(8);
        reset_pass.trim();
        Serial.println(reset_pass.length());
        Serial.println(reset_pass);

      } else if (line.startsWith("lastname = ")) {
        lastname = line.substring(11);
        lastname.trim();
        Serial.println(lastname.length());
        Serial.println(lastname);

      } else if (line.startsWith("bloodtype = ")) {
        bloodtype = line.substring(12);
        bloodtype.trim();
        Serial.println(bloodtype.length());
        Serial.println(bloodtype);

      } else if (line.startsWith("weight = ")) {
        weight = line.substring(9);
        weight.trim();
        Serial.println(weight.length());
        Serial.println(weight);

      } else if (line.startsWith("height = ")) {
        height = line.substring(9);
        height.trim();
        Serial.println(height.length());
        Serial.println(height);

      } else if (line.startsWith("brithday = ")) {
        brithday = line.substring(11);
        brithday.trim();
        Serial.println(brithday.length());
        Serial.println(brithday);

      } else if (line.startsWith("address = ")) {
        address = line.substring(10);
        address.trim();
        Serial.println(address.length());
        Serial.println(address);

      } else if (line.startsWith("moreinfo = ")) {
        moreinfo = line.substring(11);
        moreinfo.trim();
        Serial.println(moreinfo.length());
        Serial.println(moreinfo);

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
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
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

void siren() {
  int freq;
  for (freq = 500; freq < 1800; freq += 5)
  {

    digitalWrite(green_led, HIGH);
    pinMode(vibration_motor, OUTPUT);
    digitalWrite(vibration_motor , LOW);
    tone(buzzer, freq, timeBuzzer);     // Beep pin, freq, time
    delay(5);
  }
  for (freq = 1800; freq > 500; freq -= 5)
  {

    tone(buzzer, freq, timeBuzzer);     // Beep pin, freq, time

    digitalWrite(green_led, LOW);
    pinMode(vibration_motor, INPUT);
    delay(5);
  }
} void siren2() {
  Serial.println("vibra5");
  int freq;
  for (freq = 600; freq < 1200; freq += 5)
  {

    digitalWrite(green_led, HIGH);
    pinMode(vibration_motor, OUTPUT);
    digitalWrite(green_led , LOW);
    tone(buzzer, freq, timeBuzzer);     // Beep pin, freq, time
    delay(5);
  }
  for (freq = 1200; freq > 600; freq -= 5)
  {

    tone(buzzer, freq, timeBuzzer);     // Beep pin, freq, time

    digitalWrite(green_led, LOW);
    pinMode(vibration_motor, INPUT);
    delay(5);
  }
}

void send_json(String data, String MSG) {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["ESP"] = data;
  JSONencoder["MSG"] = MSG;
  JSONencoder["NAME"] = ALIAS;
  JSONencoder["ID"] = Device_id;
  JsonArray& values = JSONencoder.createNestedArray("values"); //JSON array
  values.add(20); //Add value to array
  values.add(21); //Add value to array
  values.add(23); //Add value to array

  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  Serial.println("JSONmessageBuffer"); 
  HTTPClient http;    //Declare object of class HTTPClient
  Serial.println("start http1"); 
  http.begin(host);      //Specify request destination
  Serial.println("http.begin(host)"); 
  http.addHeader("Content-Type", "application/json");  //Specify content-type header

  int httpCode = http.POST(JSONmessageBuffer);   //Send the request
  String payload = http.getString();                                        //Get the response payload

  Serial.println(httpCode);   //Print HTTP return code
  Serial.println("Code"); 
  Serial.println(payload);    //Print request response payload
  Serial.println("Payload"); 
  http.end();  //Close connection
}

void doInt()
{
  timeOut = timer;
  state = 1;
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
  switch(mpu.getClockSource())
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
  switch(mpu.getRange())
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
  switch(mpu.getAccelPowerOnDelay())
  {
    case MPU6050_DELAY_3MS:            Serial.println("3ms"); break;
    case MPU6050_DELAY_2MS:            Serial.println("2ms"); break;
    case MPU6050_DELAY_1MS:            Serial.println("1ms"); break;
    case MPU6050_NO_DELAY:             Serial.println("0ms"); break;
  }  
  
  Serial.println();
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
  while (!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_16G)) {
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
  attachInterrupt(int_1, doInt, RISING);
  //#####initialize input/output#####
  //vibration active Low
  pinMode(vibration_motor, OUTPUT);
  digitalWrite(vibration_motor, HIGH);
  pinMode(confirm_button, INPUT);
  pinMode(green_led, OUTPUT);
  digitalWrite(green_led, LOW);
  pinMode(red_led, OUTPUT);
  digitalWrite(red_led, LOW);
  // pinMode(wifi_led, OUTPUT);
  pinMode(battery_adc, INPUT);
//  pinMode(buzzer , OUTPUT);
//  digitalWrite(buzzer, LOW);
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(buzzer, LEDC_CHANNEL_0);
  //#####End initialize input/output#####

  prepareFile();//SSID PASS File

  beep(50);
  beep(50);
  timeOut = millis();
}


void loop() {
  timer = millis();
  buttonState = digitalRead(confirm_button);
  /*#######################################################
    ทำงานหลังจากเครื่องพร้อมใช้งาน 10 วินาที
    pre_program_mode | เปลี่ยนเมื่อ           | เข้าสู่โหมด
    -----------------+--------------------
    0                | defualt           |    -
    1                | กดปุ่ม4วินาที         | AP mode
    2                | ไม่มีการกดปุ่ม       | Normal mode
    #######################################################*/
  if (pre_program_mode == 0) {
    digitalWrite(green_led, HIGH );
    if (((timer - timeOut) / 1000) < 10) {
      if (buttonState == HIGH) {
        unsigned long timerAck = ((timer - preTime) / 1000);
        if (timerAck >= 3) {
          delay(50);
          pre_program_mode = 1;
        }
      } else {
        digitalWrite(green_led ,HIGH);
        delay(500);
        digitalWrite(green_led ,LOW);
        delay(500);
        preTime = timer;
        
      }
    } else {
      pre_program_mode = 2;
    }


  }
  //buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (pre_program_mode == 1) {
    if (start_ap == 1) {
      setup_apmode();
      start_ap = 0;
      beep(250);
      beep(50);
      beep(50);
    }
    dnsServer.processNextRequest();
    server.handleClient();
    digitalWrite(green_led, HIGH  );
    delay(250);
    digitalWrite(green_led, LOW  );
    delay(250);
  }
  if (pre_program_mode == 2) {
    if (start_ap == 1) {
      start_ap = 0;
      digitalWrite(green_led , HIGH);

      setup_wifi();
      microgear.init(KEY, SECRET, ALIAS);
      microgear.connect(APPID);
      //?
      if (reset_pass == "1") {
        send_json("RESET", "Password has been change");
        reset_pass = "0";
        handle_msg();
        Serial.println("--------------------------- password reset----------------------------");
      }
      //?
    }
    // wifi check disconnect

    // NEW
    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
      if (WiFi.status() == WL_CONNECTED) {
        microgear.init(KEY, SECRET, ALIAS);
        microgear.connect(APPID);
        microgear.loop();
      }
    }

    if ((WiFi.status() == WL_CONNECTED) && (!microgear.connected())) {
      microgear.init(KEY, SECRET, ALIAS);
      microgear.connect(APPID);
      microgear.loop();
    }
    if (microgear.connected())
    {
      microgear.loop();
    }
    //END NEW

    //OlD
    /*if (WiFi.status() != WL_CONNECTED) {
      Serial.println("check point3");
      setup_wifi();
      Serial.println("check point1");
      microgear.init(KEY, SECRET, ALIAS);
      microgear.connect(APPID);
      microgear.loop();
      }
      //microgear connect
      microgear.loop();*/
    //END OLD
    if (WiFi.status() == WL_CONNECTED) {
      if (state == 0) {
        //Serial.println("check point2");
        digitalWrite(green_led, HIGH);
        if (buttonState == HIGH) {
          unsigned long timerAck = ((timer - preTime) / 1000);
          if ( timerAck >= 1.0) {
            state = 3;
          }
        } else {
          preTime = timer;
        }


      } else if (state == 1) {
        //Serial.println((timer - timeOut) / 1000);
        buttonState = digitalRead(confirm_button);

        if (((timer - timeOut) / 1000) <= 8) {
          
          digitalWrite(green_led, HIGH);
          digitalWrite(vibration_motor , LOW);
          delay(500);
          digitalWrite(green_led, LOW);
          pinMode(vibration_motor, OUTPUT);
          digitalWrite(vibration_motor , HIGH);
          delay(500);
          

          if (buttonState == HIGH) {
            unsigned long timerAck = ((timer - preTime) / 1000);
            if ( timerAck >= 1.0) {
              digitalWrite(green_led, LOW);
              digitalWrite(vibration_motor , HIGH);
              delay(50);
              state = 4;
            }
          } else {
            preTime = timer;
          }

        } else {

          DETECT = "FALL";
          ACK = 1;
          state = 2;
        }


      } else if (state == 2) {
        //buzzer , vibration on
        //รอ ack เพื่อเปลี่ยนเสียง
        if (ACK == 1) {
          
          send_notification(30 * 1000);
          siren();
        } else {
          siren2();
        }
        buttonState = digitalRead(confirm_button);

        if (buttonState == HIGH ) {

          unsigned long timerAck = ((timer - preTime) / 1000);
          if ( timerAck >= 1) {
            
            send_json("HELPACK", "HELPACK");
            analogWrite(buzzer, 0);       // 0 turns it off
            state = 4;
          }

        } else {
          preTime = timer;
        }

      } else if (state == 3) {
        buttonState = digitalRead(confirm_button);
        if (buttonState == HIGH) {

        } else {
          DETECT = "PRESS";
          state = 2;
        }
      } else if (state == 4) {
        buttonState = digitalRead(confirm_button);
        if (buttonState == HIGH) {

        } else {
          ACK = 1; // Enable send massage to line every 30 sec
          state = 5;
        }
      } else if (state == 5) {
        
        digitalWrite(green_led, HIGH);
        beep(60);
        digitalWrite(green_led, LOW);
        beep(60);
        digitalWrite(green_led, HIGH);
        state = 0;
      }

      //read_battery_milsec(600000);
    }
  }
}
