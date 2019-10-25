// Bit-LED Project

// This Program hosts a configuration web page on an ESP32's WiFi AP
// where the user can set the WiFi SSID and password of a network
// connected to the Internet.
// Also on this config page the user can set the Bitrise Access Token
// and the App Slug of a Bitrise App.

// When the configuration is done, the ESP32 turns into Station mode:
// Turning off it's WiFi AP and connecting to the userdefined WiFi network
// Then it regularly checks the state of the configured Bitrise App
// and sets the LED strip's color accordingly:
// In Progress - Purple blinking
// Success     - Green
// Error       - Red
// Aborted     - Yellow

// Libraries used
#include <Arduino.h> // Aurdino core
#include <Adafruit_NeoPixel.h> // NeoPixel
#include <WiFi.h> // WiFi
#include <HTTPClient.h> // HTTP Client
#include "ESPAsyncWebServer.h" // WebServer
#include "ArduinoJson.h" // JSON Parser

// WiFi AP Settings
const char* ssid = "BIT-LED";  // SSID

// IP Address details
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Station state
bool Change_to_Station = false;
bool Is_Station        = false;

// WiFi Station variables
// These will be set by the user during runtime via the configuration page
const char* wifi_ssid;
const char* wifi_pass;
const char* app_slug;
const char* api_token;
char wifi_ssid_c[32]  = "\0";
char wifi_pass_c[32]  = "\0";
char app_slug_c[32]   = "\0";
char api_token_c[128] = "\0";

// Poll Interval and state
const long Poll_Interval       = 2000;
unsigned long Next_Poll_Millis = 0;

// A tiny web server to host the configuration page
AsyncWebServer server(80);

// The configuration page compressed into one line
const char* Web_Page = "<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\"> <title>Bit LED</title> <link href=\"data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAADRVQAAAAAALU/kgDPZ7AACltuAN6VyADrltEAiiRrAPW14gCFKmoAbhlUAEADNQBXDUEAOb7bABqCmQAAAAAAEREdERHRERERER4REeERERERFBERQREREREQEREBEREREbu7u7sRERERvMzMyxERERG6qqzLEREREbmZqssRERERt3eayxERERG3d5rLEREREbJ3mssRERERsyeayxERERG1MnrLEREREbhjJ8sRERERG4YysRERERERu7sRERH73wAA+98AAPvfAAD73wAA8A8AAPAPAADwDwAA8A8AAPAPAADwDwAA8A8AAPAPAADwDwAA8A8AAPgfAAD8PwAA\" rel=\"icon\" type=\"image/x-icon\" /> <style> html, body { display: inline-block; text-align: center; height: 100%; border: 0px; padding: 0%; margin: 0%; background-color: rgb(118,15,195); } .form_elem { margin-bottom: 0.75rem; } #submit_button { background-color:aquamarine; color: rgb(8, 23, 40); font-size: 1.2rem; } </style></head><body> <div id=\"main_wrapper\"> <div id=\"main_header\"> <h1>Bit LED Configuration</h1> </div> <div id=\"main_body\"> <form> <div class=\"form_elem\"> WiFi SSID:<br> <input id=\"input_wifi_ssid\" type=\"text\" name=\"wifi_ssid\"><br> </div> <div class=\"form_elem\"> Wifi Password<br> <input id=\"input_wifi_pass\" type=\"text\" name=\"wifi_pass\"><br> </div> <div class=\"form_elem\"> Bitrise App Slug:<br> <input id=\"input_app_slug\" type=\"text\" name=\"app_slug\"><br> </div> <div class=\"form_elem\"> Bitrise API Token<br> <input id=\"input_api_token\" type=\"text\" name=\"api_token\"><br> </div> <br> <div class=\"form_elem\"> <input id=\"submit_button\" type=\"button\" value=\"Submit\"> </div> </form> <br> <div id=\"result_box\">Waiting Configs</div> </div> <br> <div id=\"main_foot\"> <p>2019 Bitrise ltd</p> </div> </div> <script> 'use strict'; function displayResult(result) { document.getElementById(\"result_box\").innerHTML = result; }; function POST(payload) { let xmlHttp = new XMLHttpRequest(); xmlHttp.onreadystatechange = () => { if (xmlHttp.readyState == 4 && xmlHttp.status == 200) displayResult(xmlHttp.responseText); }; xmlHttp.open(\"POST\", window.location.origin, true); xmlHttp.send(JSON.stringify(payload)); }; function submitValues() { let wifi_ssid = document.getElementById(\"input_wifi_ssid\").value; let wifi_pass = document.getElementById(\"input_wifi_pass\").value; let app_slug = document.getElementById(\"input_app_slug\").value; let api_token = document.getElementById(\"input_api_token\").value; let payload = { wifi_ssid: wifi_ssid, wifi_pass: wifi_pass, app_slug: app_slug, api_token: api_token }; POST(payload); }; window.addEventListener('load', function() { document.getElementById(\"submit_button\").addEventListener(\"click\", submitValues); }); </script></body></html>";

// DST Root Certificate x3 for https://api.bitrise.io
const char* API_Cert= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";

// NeoPixel LED Strip's details
#define NUMBER_OF_LEDS 1
#define LED_PIN 16
// Set up the NeoPixel controller with the predefined values
Adafruit_NeoPixel pixels(NUMBER_OF_LEDS, LED_PIN);

// Predefined colors
const uint32_t C_Black   = pixels.Color(0,0,0);
const uint32_t C_White   = pixels.Color(255,255,255);
const uint32_t C_Red     = pixels.Color(255,0,0);
const uint32_t C_Green   = pixels.Color(0,255,0);
const uint32_t C_Blue    = pixels.Color(0,0,255);
const uint32_t C_Yellow  = pixels.Color(255,255,0);
const uint32_t C_Magenta = pixels.Color(255,0,255);
const uint32_t C_Cyan    = pixels.Color(0,255,255);
const uint32_t C_Purple  = pixels.Color(118,15,195);

// The current color of the LED
uint32_t LED_Color = C_Black;

// LED blinking related variables
const long Delay_Interval     = 500;
unsigned long Previous_Millis = 0;
bool Is_Blinking              = false;
bool Blink_State              = false;

// Set the color of the LED and the blinking state
void Set_LED(uint32_t color, bool blinking) {
  LED_Color = color;
  Is_Blinking = blinking;
  Serial.print("LED has been set to color: ");
  Serial.print(color);
  Serial.print(" blinking: ");
  Serial.println(blinking);
}

// Depending on the blink state enlight or turn off the LED
void Show_LED() {
  // Flip the blinking state in every interval
  unsigned long Current_Millis = millis();
  if (Current_Millis - Previous_Millis >= Delay_Interval) {
    Previous_Millis = Current_Millis;
    Blink_State = !Blink_State;
  }
  // Set the LEDs color depending on the blink state
  for(int i=0; i < pixels.numPixels(); i++) {
    if (Is_Blinking && !Blink_State) {
       pixels.setPixelColor(i, C_Black);
    } else {
      pixels.setPixelColor(i, LED_Color);
    }
  }
  // Enlight the LEDs
  pixels.show();
}

// Show the LED for n millisedonds
void Show_LED(unsigned long until) {
  unsigned long Started_at = millis();
  while (millis() < Started_at + until) {
    Show_LED();
  }
}

// Change the LED's color according to the Bitrise App's status
void Set_LED_By_Status(int Status) {
  switch (Status) {
    case 0: // In progress
      Set_LED(C_Purple, true);
      break;
    case 1: // Success
      Set_LED(C_Green, false);
      break;
    case 2: // Fail
      Set_LED(C_Red, false);
      break;
    case 3: // Aborted
      Set_LED(C_Yellow, false);
      break;
    default: // Unknown status
      Set_LED(C_Cyan, true);
      break;
  }
  Show_LED();
}

// Issue a GET request agains the Bitrise API
// Change the LED's state according to the App's status
void PollBitrise() {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String url = String("https://api.bitrise.io/v0.1/apps/") + String(app_slug) + String("/builds?limit=1");
    Serial.print("GET: ");
    Serial.println(url);
    http.begin(url, API_Cert); //Specify the URL and certificate
    if (api_token) {
      http.addHeader("Authorization", api_token);
    }
    int httpCode = http.GET();
    if (httpCode > 0) { //Check for the returning code
      Serial.println("Build data received from Bitrise.io");
      Serial.print("Status Code: ");
      Serial.println(httpCode);
      String payload = http.getString();
      Serial.println(payload);
      StaticJsonDocument<2000> obj;
      DeserializationError err = deserializeJson(obj, payload);
      if (err) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(err.c_str());
      } else {
        int temp_status = obj["data"][0]["status"].as<int>();
        Serial.printf("status: %d\n", temp_status);
        Set_LED_By_Status(temp_status);
      }
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end(); //Free the resources
  } else {
    Serial.println("Error: Not connected to the WiFi");
  }
}

// Stop the WiFi AP and set up Station Mode
void ChangeToStationMode() {
  Set_LED(C_Purple, true);
  Show_LED();
  Serial.println("Turning off WebServer...");
  Show_LED(1000);
  server.end();
  Show_LED(1000);
  Serial.println("Disconnecting WiFi AP...");
  WiFi.disconnect();
  Show_LED(1000);
  Serial.printf("Connecting to WiFi: %s with Pass: %s\n", wifi_ssid, wifi_pass);
  WiFi.mode(WIFI_STA);
  Show_LED(1000);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) {
      Show_LED(500);
      Serial.print(".");
  }
  Set_LED(C_Green, true);
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Show_LED(3000);
}

// ---------------- Setup Functions ---------------------------------
// Print the Bit-LED title on the serial
void PrintTitle() {
  Serial.println();
  Serial.println("+---------------------+");
  Serial.println("| . o O Bit-LED O o . |");
  Serial.println("+---------------------+");
  Serial.println();
}

// Iinitialize the NeoPixel strip object
void SetupLED() {
  pixels.begin();
  pixels.clear();
  Set_LED(C_Purple, false);
  Serial.println("NeoPixel has been set up");
  Serial.println();
}

// Setup WiFi AP
void SetupAP() {
  WiFi.persistent(false);
  WiFi.softAP(ssid, NULL);
  delay(100);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  Serial.println("Wifi AP has been set up");
  Serial.println("SSID: BIT-LED");
  Serial.println("IP  : 192.168.1.1");
  Serial.println();
}

// Set up the async web server which will host the configuration page
// and parse incoming configuration POST requests
void SetupServer() {
  // The config page sitting on the root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", Web_Page);
  });

  // On POST request parse the configuration JSON
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    Serial.println("Incoming configuration");
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, data);
    switch (err.code()) { // Report any error
    case DeserializationError::Ok:
        Serial.println(F("Deserialization succeeded"));
        break;
    case DeserializationError::InvalidInput:
        Serial.println(F("Invalid input!"));
        break;
    case DeserializationError::NoMemory:
        Serial.println(F("Not enough memory"));
        break;
    default:
        Serial.println(F("Deserialization failed"));
        break;
    }
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      request->send(200, "text/plain", "Invalid Configuration");
    } else {
      // Copy the JSON fields into the char arrays and set their pointers
      strcpy(wifi_ssid_c, doc["wifi_ssid"]);
      wifi_ssid = wifi_ssid_c;
      strcpy(wifi_pass_c, doc["wifi_pass"]);
      wifi_pass = wifi_pass_c;
      strcpy(app_slug_c, doc["app_slug"]);
      app_slug = app_slug_c;
      strcpy(api_token_c, doc["api_token"]);
      api_token = api_token_c;
      // TODO: config validation
      Serial.printf("Bit-LED Configs:\nSSID : %s\nPass : %s\nSlug : %s\nToken: %s\n", wifi_ssid, wifi_pass, app_slug, api_token);
      Change_to_Station = true;
      request->send(200, "text/plain", "Configured successfully");
    }
  });

  // In any other case fall back to 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404 - Not Found");
  });

  // Listen and serve
  server.begin();
  Serial.println("Bit-LED HTTP server listening on PORT 80");
  Serial.println();
}

// ---------------- SETUP -------------------------------------------
void setup() {
  // Open the serial port
  Serial.begin(115200);
  PrintTitle();
  SetupAP();
  SetupServer();
  SetupLED();
}

// ---------------- MAIN LÖÖP ---------------------------------------
void loop() {
  if (Change_to_Station) {
    ChangeToStationMode();
    Change_to_Station = false;
    Is_Station = true;
  }
  unsigned long Now = millis();
  if (Is_Station && Next_Poll_Millis <= Now) {
    Next_Poll_Millis = Now + Poll_Interval;
    PollBitrise();
  }
  Show_LED();
}
