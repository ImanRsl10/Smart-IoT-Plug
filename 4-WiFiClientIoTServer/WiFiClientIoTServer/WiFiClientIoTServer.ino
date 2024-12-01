#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <AsyncHTTPRequest_Generic.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

Ticker ticker;
AsyncHTTPRequest httpReq;
ESP8266WebServer server(80);
//AsyncHTTPRequest httpPost;
//ICACHE_RAM_ATTR void touchInterrupt();
WiFiClient client;

#define GET_TIME 1
#define TOUCH_DELAY_TIME 500
#define AP_SSID "Piriiiiiz"
#define AP_PASS "parto1234"

String WIFI_SSID, WIFI_PASS;
const char *postURL = "http://espike.ir/time.php";
const char *getURL = "http://espike.ir/mic1.php";

int relayState, getFlag, lastTimeConnection;
int timer, timerLastVal, timerFlag; //Timer Part
volatile int lastTouchTime = 0, touchFlag = 0; //Touch Part

void setup()
{
  Serial.begin(115200);
  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);
  EEPROM.begin(512);
  //load a local server
  server.on("/", handleUrl);
  server.on("/esp", localServerCmd);

  //load
  WIFI_SSID = loadWiFi(WIFI_SSID, 0, 32);
  WIFI_PASS = loadWiFi(WIFI_PASS, 32, 96);
  Serial.printf("SSID:%s", WIFI_SSID);
  Serial.printf("PASS:%s", WIFI_PASS);

  attachInterrupt(digitalPinToInterrupt(D2), touchInterrupt, FALLING);

  httpReq.onReadyStateChange(pgReqCB);
  ticker.attach(GET_TIME, getReq);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(1000);
  Serial.print("Proccessing");
  server.begin();
}

void loop()
{
  server.handleClient();

  if (touchFlag)
    RelayOnTouch();

  if (timerFlag)
  {
    if (millis() - timerLastVal > timer)
    {
      //when the timer finished
      relayState = 0;
      timer = 0;
      timerFlag = 0;
      digitalWrite(D1, relayState);
      postReq("relay='0'");
    }
  }
}

ICACHE_RAM_ATTR void touchInterrupt()
{
  if (millis() - lastTouchTime > TOUCH_DELAY_TIME)
  {
    touchFlag = 1;
    Serial.printf("I'm giong to touch flag 1\n");
    lastTouchTime = millis();
  }
}

void RelayOnTouch()
{
  relayState = !relayState;
  Serial.printf("touch Status is:%d\n", relayState);
  digitalWrite(D1, relayState);
  postReq("relay='" + String(relayState) + "'");
  touchFlag = 0;
  timerFlag = 0;
}

void getReq()
{
  Serial.print("[HTTP] begin...\n");
  int httpCode = httpReq.open("GET", getURL);
  if (httpCode)
  {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (millis() > lastTimeConnection)
    {
      httpReq.send();
    }
    else
    {
      httpReq.send();
    }
  }
  getFlag = 1;
}

void pgReqCB(void* optParm, AsyncHTTPRequest* request, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    if (request->responseHTTPcode() == 200)
    {
      String command = request->responseText();
      Serial.println(F("\n********GET************"));
      Serial.println(command);
      Serial.println(F("*********************"));
      lastTimeConnection = millis();
      request->setDebug(false);
      if (getFlag)
      {
        proccessCommand(command);
        getFlag = 0;
      }
    }
  }
}

void postReq(String data)
{
  httpReq.open("POST", postURL);
  httpReq.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = httpReq.send(data);
  Serial.printf("POST .... Code:%d\n", httpCode);
}

void postReqCB(void* optParm, AsyncHTTPRequest* request, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    String cmd = request->responseText();
    Serial.println(F("\n*******POST*********"));
    Serial.println(cmd);
    Serial.println(F("****************"));

    request->setDebug(false);
    Serial.println(cmd);
  }
}

void proccessCommand(String payload)
{
  int values[2];
  for (int i = 0; i < 2; i++)
  {
    int andLoc = payload.indexOf("&");
    String current = payload.substring(0, andLoc);
    int eqLoc = current.indexOf("=");
    current = current.substring(eqLoc + 1);
    values[i] = current.toInt();
    payload = payload.substring(andLoc + 1);
  }
  Serial.printf("relay Status:%d\n", values[0]);
  Serial.printf("time span:%d\n", values[1]);
  relayState = values[0];
  if (relayState == 0 && timerFlag == 1)
    timerFlag = 0;
  if (values[1] != 0)
  {
    timer = values[1] * 1000;
    timerLastVal = millis();
    timerFlag = 1;
    postReq("time='0'");
  }
  digitalWrite(D1, relayState);
}

void handleUrl()
{
  String str = "Dalam";
  server.send(200, "text/html", str);
}

String scanNetwork()
{
  int numberOfNetworks = WiFi.scanNetworks();
  String networks;
  for (int i = 0; i < numberOfNetworks; i++)
  {
    if (i == numberOfNetworks - 1)
    {
      networks += WiFi.SSID(i);
    }
    else
    {
      networks += WiFi.SSID(i) + ",";
    }
  }
  return networks;
}

void localServerCmd()
{
  String cmd = server.arg(0);
  if (cmd == "scan")
  {
    server.send(200, "text/html", scanNetwork());
  }
  else if (cmd == "relaySwitch")
  {
    relayState = server.arg(1).toInt();
    digitalWrite(D1, relayState);
    server.send(200, "text/html", "OK");
  }
  else if (cmd == "connect")
  {
    WIFI_SSID = server.arg(1);
    WIFI_PASS = server.arg(2);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    //clear
    for (int i = 0; i < 96; i++)
    {
      EEPROM.write(i, 0);
    }
    //write
    for (int j = 0; j < WIFI_SSID.length(); j++)
    {
      EEPROM.write(j, WIFI_SSID[j]);
    }
    for (int k = 32; k < WIFI_PASS.length() + 32; k++)
    {
      EEPROM.write(k, WIFI_PASS[k - 32]);
    }
    EEPROM.commit();
    server.send(200, "text/html", "Connecting to Hajitoon");
  }
  else if (cmd == "setTimer")
  {
    timer = server.arg(1).toInt() * 1000;
    timerFlag = 1;
    relayState = 1;
    digitalWrite(D1, relayState);
    server.send(200, "text/html", "Timer has been set");
    timerLastVal = millis();
  }
}

String loadWiFi(String WiFiInf, int startAddr, int endAddr)
{
  WiFiInf = "";
  for (int i = startAddr; i < endAddr; i++)
  {
    char data = char(EEPROM.read(i));
    if (data == NULL)
    {
      break;
    }
    else
    {
      WiFiInf = WiFiInf + data;
    }
  }
  return WiFiInf;
}
