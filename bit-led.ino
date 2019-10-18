// control one NeoPixel LED

// Import Adafruit's NeoPixel library
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// Define the data PIN we will use for the LED
#define PIN 15

// Declare object of class WiFiClient
WiFiClientSecure httpsClient;

// Set up the NeoPixel LED on the predefined PIN
Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);

const String Host = "api.bitrise.io";
const String Wifi_Name = "Your-Wifi-Name";
const String Wifi_Pass = "Your-Wifi-Pass";
const String App_URL = "/app/ae82b6a9ec246f90/status.json?token=AHkq9V2RubZyORmdYxLGZw&branch=master";

const String CONFIG_HOST = "gist.githubusercontent.com";
const String CONFIG_URL = "/bitrisekristof/b9414fe2dbbea82745ffe8abc14707d6/raw/config.conf";
const String BITRISE_HOST = "app.bitrise.io";

const int PORT = 443;  //HTTPS= 443 and HTTP = 80
String Slug_From_Gist;


//SHA1 finger print of certificate use web browser to view and copy
const char fingerprint[] PROGMEM = "32 5B 1E 37 8B 82 46 1A 01 5B 18 FA 1F 0C 76 C0 A3 90 C4 7F";
// Constant colors
const uint32_t C_Black = pixels.Color(0,0,0);
const uint32_t C_Red = pixels.Color(255,0,0);
const uint32_t C_Green = pixels.Color(0,255,0);
const uint32_t C_Blue = pixels.Color(0,0,255);
const uint32_t C_Purple = pixels.Color(118,15,195);
const uint32_t C_Yellow = pixels.Color(255,200,0);
const uint32_t C_Magenta = pixels.Color(244,244,30);
const uint32_t C_Cyan = pixels.Color(245,0,245);

// The current color of the LED
uint32_t LED_Color = C_Black;

// Is the LED blinking
int Is_Blink = 0;
int Blink_State = 0;

unsigned long Previous_Millis = 0;

const long Delay_Interval = 100;

void setup() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  Set_LED(C_Purple, 1);
  Show_LED();
  connectToWifi(Wifi_Name, Wifi_Pass);
  Slug_From_Gist = GetSlug(CONFIG_HOST, CONFIG_URL);
}

void loop() {
  char StatusCh = getStatusOfLastBuild(Slug_From_Gist, "");
  switch (StatusCh) {
    case '0':
      Set_LED(C_Purple, 0);
      break;
    case '1':
      Set_LED(C_Green, 0);
      break;
    case '2':
      Set_LED(C_Red, 0);
      break;
    case '3':
      Set_LED(C_Yellow, 0);
      break;
    default:
      Set_LED(C_Cyan, 1);
      break;
  }
  unsigned long Before_Loop = millis();
  while (Before_Loop + 500 > millis()) {
    Show_LED();
  }
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

void connectToWifi(String ssid, String password) {
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

char getStatusOfLastBuild(String slug, String token)
{
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000); // 15 Seconds
  int r=0; //retry counter
  while((!httpsClient.connect(Host, PORT)) && (r < 30)){
    delay(100);
    r++;
  }
  if(r==30) {
    Serial.println("Failed to connect");
    return 'x';
  }
  String ADCData, getData, Link;
  int adcvalue=analogRead(A0);  //Read Analog value of LDR
  ADCData = String(adcvalue);   //String to interger conversion

  httpsClient.print(String("GET ") + "/v0.1/apps/" + slug + "/builds?limit=1"+ " HTTP/1.1\r\n" +
               "Host: " + Host + "\r\n" +
               "Connection: close\r\n\r\n");
  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line;
  line = httpsClient.readString();  //Read Line by Line
  Serial.println(line);

  int idx = line.indexOf("status");

  Serial.print("Status :");
  Serial.println(line[idx + 8]);

  return line[idx + 8];
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

  return line;

}

String GetSlug(String host, String url) {
  String result = GetHttpContent(host, url);
  Serial.println("APP Slug");
  Serial.println(result);
  return result;
}
