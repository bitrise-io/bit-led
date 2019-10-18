// control one NeoPixel LED

#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <IotWebConf.h>

// Define the data PIN we will use for the LED
#define PIN D4
// Set up the NeoPixel LED on the predefined PIN
Adafruit_NeoPixel pixels(1, PIN, NEO_GRB + NEO_KHZ800);

const String host = "app.bitrise.io";
const String App_URL = "/app/ae82b6a9ec246f90/status.json?token=AHkq9V2RubZyORmdYxLGZw&branch=master";
#define APP_SLUG_LEN 16
#define TOKEN_LEN 22

/// Captive Portal stuff

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "rgb_status_led_01";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "rgb_status_led_01";
DNSServer dnsServer;
WebServer server(80);
char appSlug[APP_SLUG_LEN];
char token[TOKEN_LEN];
//IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, "ver1");
IotWebConfParameter appSlugParam = IotWebConfParameter("App Slug", "appSlug", appSlug, APP_SLUG_LEN);
IotWebConfParameter tokenParam = IotWebConfParameter("Token", "token", token, TOKEN_LEN);
void wifiConnected();

const int PORT = 443;  // HTTPS 443

//SHA1 finger print of certificate use web browser to view and copy
const char fingerprint[] PROGMEM = "1E E5 3F 59 83 35 42 53 49 69 FB 0A 3A 49 8D 66 5E 58 05 15";

// Colors
const uint32_t C_Black = pixels.Color(0,0,0);
const uint32_t C_Red = pixels.Color(255,0,0);
const uint32_t C_Green = pixels.Color(0,255,0);
const uint32_t C_Blue = pixels.Color(0,0,255);
const uint32_t C_Purple = pixels.Color(118,15,195);
const uint32_t C_Yellow = pixels.Color(244,244,30);
const uint32_t C_Magenta = pixels.Color(244,30, 244);
const uint32_t C_Cyan = pixels.Color(30,244,240);

// The current color of the LED
uint32_t LED_Color = C_Black;

// Is the LED blinking
int Is_Blink = 0;
int Blink_State = 0;

unsigned long Previous_Millis = 0;

const long Delay_Interval = 500;

void setup() {
  Serial.begin(115200);
  
  // -- Initializing the configuration.
  iotWebConf.setStatusPin(LED_BUILTIN);
  iotWebConf.addParameter(&appSlugParam);
  // iotWebConf.addParameter(&tokenParam);
  // iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear();
  Set_LED(C_Purple, 1);
  Show_LED(); 
}

bool connected = false;

void loop() {
  if(connected) {
    if (GetStatusOfLastBuild(App_URL) == 1) {
      Set_LED(C_Green, 0);
    } else {
      Set_LED(C_Red, 0);
    }
  }
  iotWebConf.doLoop();

  Show_LED();
  delay(500);
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 01 Minimal</title></head><body style=\"text-align: center;\"><h3>Bit LED</h3><br>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void Set_LED(uint32_t color, int blinking) {
  LED_Color = color;
  Is_Blink = blinking;
}

void wifiConnected()
{
  Serial.println("WiFi was connected.");
  connected = true;
  Set_LED(C_Magenta, 0);
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

int GetStatusOfLastBuild(String url) {
   WiFiClientSecure httpsClient;    //Declare object of class WiFiClient

  Serial.println(host);

  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000); // 15 Seconds

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
  Link = url;

  Serial.print("requesting URL: ");
  Serial.println(host+Link);

  httpsClient.print(String("GET ") + Link + " HTTP/1.1\r\n" +
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
  line = httpsClient.readString();  //Read Line by Line
  if(line.indexOf("success") > 0) {
    return 1;
  }
  else {
    return 2;
  }
}