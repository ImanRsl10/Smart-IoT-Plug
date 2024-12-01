#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "ESPape"
#define APPSK  "thereisnospoon"
#endif

/ Tri - LED /
#define DELAY_TIME 200
int mag = 0;
volatile int f = 0, lastTime = 0;
int totalTime = 2000, offTime = 1000, onTime = 1000;
int flag = 0;
int count = 0;

/ Set these to your desired credentials. /
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

void handleUrl() {
  String str = String(f);
  server.send(200, "text/html", str);
}
void ledOff() {
  flag = 0;
  digitalWrite(D1, LOW);
  server.send(200, "text/html", "<h1>turning off</h1>");
  count++;
}
void ledOn() {
  flag = 0;
  digitalWrite(D1, HIGH);
  server.send(200, "text/html", "<h1>turning on</h1>");
  count++;
}

void ledBlink() {
  flag = 1;
  if (millis() % totalTime < onTime) {
    digitalWrite(D1, HIGH);
  }
  else {
    digitalWrite(D1, LOW);
  }
  server.send(200, "text/html", "<h1>Blink</h1>");
  count++;
}

void espOnTime() {
  flag = 0;
  int Time = millis();
  int Sec = Time / 1000;
  int Min = Sec / 60;
  int Hour = Min / 60;
  String second = String(Sec);
  String minute = String(Min);
  String hour = String(Hour);
  server.send(200, "text/html", second);
  count++;
}

void simpleCounter() {
  count++;
  String cnt = String(count);
  server.send(200, "text/html", cnt);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  / You can remove the password parameter if you want the AP to be open. /
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  pinMode(D1, OUTPUT);
  //pinMode(D2, INPUT);
  //attachInterrupt(digitalPinToInterrupt(D2), ISR, RISING);
  //digitalWrite(D1, HIGH);

  server.on("/", handleUrl);
  server.on("/ledOff", ledOff);
  server.on("/ledOn", ledOn);
  server.on("/ledBlink", ledBlink);
  server.on("/espOnTime", espOnTime);
  server.on("/simpleCounter", simpleCounter);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (flag == 1) {
    ledBlink();
  }
}

ICACHE_RAM_ATTR void ISR()
{
  if (millis() - lastTime > DELAY_TIME) {
    f++;
    lastTime = millis();
  }
}

/ Create a WiFi access point and provide a web server on it. /

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "ESPape"
#define APPSK  "thereisnospoon"
#endif

/ Tri - LED /
#define DELAY_TIME 200
int mag = 0;
volatile int f = 0, lastTime = 0;
int totalTime = 1000, offTime = 500, onTime = 500;
int OffFlag = 0;

/ Set these to your desired credentials. /
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleUrl() {
  String str = String(f);
  server.send(200, "text/html", str);
}
int ledOff() {
  int flag = 0;
  server.send(200, "text/html", "<h1>turning off</h1>");
  flag = 1;
  return flag;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  / You can remove the password parameter if you want the AP to be open. /
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);
  attachInterrupt(digitalPinToInterrupt(D2), ISR, RISING);

  server.on("/", handleUrl);
  server.on("/ledOff", ledOff);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  Serial.println(f);
  switch (f % 3) {
    case 1:
      digitalWrite(D1, HIGH);
      break;
    case 2:
      digitalWrite(D1, LOW);
      break;
    case 0:
      if (f != 0) {
        if (millis() % totalTime
            < onTime) {
          digitalWrite(D1, HIGH);
        }
        else {
          digitalWrite(D1, LOW);
        }
        break;
      }
  }
  //OffFlag = ledOff();
  if (ledOff() == 1) {
    digitalWrite(D1, LOW);
  }
}

ICACHE_RAM_ATTR void ISR()
{
  if (millis() - lastTime > DELAY_TIME) {
    f++;
    lastTime = millis();
  }
}
