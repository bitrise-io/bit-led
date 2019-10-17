#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
 
/* Set these to your desired credentials. */
const char *ssid = "Bitrise";  //ENTER YOUR WIFI SETTINGS
const char *password = "NemErtek31";
 
//Link to read data from https://jsonplaceholder.typicode.com/comments?postId=7
//Web/Server address to read/write from 
const char *host = "app.bitrise.io";
const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80
 
//SHA1 finger print of certificate use web browser to view and copy
const char fingerprint[] PROGMEM = "1E E5 3F 59 83 35 42 53 49 69 FB 0A 3A 49 8D 66 5E 58 05 15";
//=======================================================================
//                    Power on setup
//=======================================================================
 
void setup() {
  connectWifi(ssid, password)
}
 
//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {

  int buildStatus = getStatusOfLastBuild("/app/ae82b6a9ec246f90/status.json?token=AHkq9V2RubZyORmdYxLGZw&branch=master");
  if (buildStatus == 1) {
    Serial.println("SUCCEESSSS!!!");
  }
  else {
    Serial.println("FAAAIIIIL");
  }
  delay(2000);  //GET Data at every 2 seconds
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

int getStatusOfLastBuild(String url)
{
   WiFiClientSecure httpsClient;    //Declare object of class WiFiClient
 
  Serial.println(host);
 
  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);
  
  Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!httpsClient.connect(host, httpsPort)) && (r < 30)){
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
