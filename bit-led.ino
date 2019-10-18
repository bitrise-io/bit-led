// control one NeoPixel LED

// Import Adafruit's NeoPixel library
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// Define the data PIN we will use for the LED
#define PIN 15

// Set up the NeoPixel LED on the predefined PIN
Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);

const String CONFIG_HOST = "gist.githubusercontent.com";
const String CONFIG_URL = "/bitrisekristof/b9414fe2dbbea82745ffe8abc14707d6/raw/81e0a430d36cbfb0782db8099414dbe123631492/config.conf";
const String BITRISE_HOST = "app.bitrise.io";
const String Wifi_Name = "Your-wifi-name";
const String Wifi_Pass = "Your-wifi-passw";


const int PORT = 443;  // HTTPS 443

//SHA1 finger print of certificate use web browser to view and copy
//const char BITRISE_FINGERPRINT[] PROGMEM = "1E E5 3F 59 83 35 42 53 49 69 FB 0A 3A 49 8D 66 5E 58 05 15";
//const char CONFIG_FINGERPRINT[]  PROGMEM = "59 74 61 88 13 CA 12 34 15 4D 11 0A C1 7F E6 67 07 69 42 F5";

// Colors
const uint32_t C_Black = pixels.Color(0,0,0);
const uint32_t C_Red = pixels.Color(255,0,0);
const uint32_t C_Green = pixels.Color(0,255,0);
const uint32_t C_Blue = pixels.Color(0,0,255);
const uint32_t C_Purple = pixels.Color(118,15,195);
const uint32_t C_Yellow = pixels.Color(244,244,30);
const uint32_t C_Magenta = pixels.Color(244,244,30);
const uint32_t C_Cyan = pixels.Color(244,244,30);

// The current color of the LED
uint32_t LED_Color = C_Black;

// Is the LED blinking
int Is_Blink = 0;
int Blink_State = 0;

unsigned long Previous_Millis = 0;

const long Delay_Interval = 500;

String Url_From_Gist;

void setup() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  Set_LED(C_Purple, 1);
  Show_LED();
  ConnectToWifi(Wifi_Name, Wifi_Pass);
  Url_From_Gist = GetUrl(CONFIG_HOST,CONFIG_URL);
}

void loop() {

  if (GetStatusOfLastBuild(Url_From_Gist) == 1) {
    Set_LED(C_Green, 0);
  } else {
    Set_LED(C_Red, 0);
  }
  Show_LED();
  delay(500);
}

void Set_LED(uint32_t color, int blinking) {
  LED_Color = color;
  Is_Blink = blinking;
}

void Show_LED() {
  unsigned long Current_Millis = millis();

  if (Current_Millis - Previous_Millis >= Delay_Interval) {
    Previous_Millis = Current_Millis;
    // if the LED is off turn it on and vice-versa:
    if (Blink_State == 0) {
      Blink_State = 1;
    } else {
      Blink_State = 0;
    }
  }

  if ((Is_Blink == 1) && (Blink_State == 0)) {
     pixels.setPixelColor(0, C_Black);
  } else {
    pixels.setPixelColor(0, LED_Color);
  }

  pixels.show();
}

void ConnectToWifi(String ssid, String password) {
  delay(1000);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //Only Station No AP, This line hides the viewing of ESP as wifi hotspot

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}

int GetStatusOfLastBuild(String url) {
  String payload = GetHttpContent(BITRISE_HOST, url);

  
  if(payload.indexOf("success") > 0) {
    return 1;
  }
  else {
    return 2;
  }
}

String GetHttpContent(String host, String url) {

     WiFiClientSecure httpsClient;    //Declare object of class WiFiClient

  Serial.println(host);

  //httpsClient.setFingerprint(fingerPrint);
  httpsClient.setInsecure();
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!httpsClient.connect(host, PORT)) && (r < 30)){
      delay(100);
      Serial.print(".");
      r++;
  }
  if(r==30) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }

  String ADCData, getData, Link;
  int adcvalue=analogRead(A0);  //Read Analog value of LDR
  ADCData = String(adcvalue);   //String to interger conversion

  //GET Data
 
  Serial.print("requesting URL: ");
  Serial.println(host+url);

  httpsClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");

  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line;
  line = httpsClient.readString();
    Serial.print("Payload: ");
  Serial.println(line);
  
  //httpsClient.end();
  
  return line;

}

String GetUrl(String host, String url) {

  String result = GetHttpContent(host, url);
  Serial.println("APP URL");
  Serial.println(result);
  return result;
  
}
