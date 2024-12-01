#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <AsyncHTTPRequest_Generic.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

Ticker ticker;
AsyncHTTPRequest httpReq;
AsyncHTTPRequest eventReq;
ESP8266WebServer server(80);
//AsyncHTTPRequest httpPost;
//ICACHE_RAM_ATTR void touchInterrupt();
WiFiClient client;

#define GET_TIME 1
#define TOUCH_DELAY_TIME 500
#define AP_SSID "Piriiiiiz"
#define AP_PASS "parto1234"
#define AP 0
#define STATION 1
#define SERVER 2
#define DISCONNECTIONTIME 15000
#define DISCONNECTTIME 10000

const char* postURL = "http://espike.ir/esp.php";

int relayState = 0, getFlag = 0, lastTimeConnection = 0, TryToConnect = 0;
bool eventFlag = false;
int timer, timerLastVal, timerFlag = 0;         //Timer Part
volatile int lastTouchTime = 0, touchFlag = 0;  //Touch Part
int ps = AP;
String apiKey = "parto";

void setup() {
  Serial.begin(115200);
  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);
  EEPROM.begin(512);
  delay(500);
  Serial.println("Dalam");
  //  attachInterrupt(digitalPinToInterrupt(D2), touchInterrupt, FALLING);

  //load a local server
  server.on("/", handleUrl);
  server.on("/esp", localServerCmd);
  server.begin();
  transitionToSTATION();

  httpReq.onReadyStateChange(postReqCB);
  //  eventReq.onReadyStateChange(eventCB);
}
void loop() {
  if (touchFlag)
    RelayOnTouch();

  StateDiagram();

  if (timerFlag) 
  {
    if (millis() - timerLastVal > timer) 
    {
      // when the timer finished
      relayState = 0;
      timer = 0;
      timerFlag = 0;
      // digitalWrite(D1, relayState);
      //postReq("relay='0'");
    }
  }
}

/*ICACHE_RAM_ATTR void touchInterrupt()
{
  if (millis() - lastTouchTime > TOUCH_DELAY_TIME)
  {
    touchFlag = 1;
    Serial.printf("I'm giong to touch flag 1\n");
    lastTouchTime = millis();
  }
}*/

void RelayOnTouch() 
{
  relayState = !relayState;
  Serial.printf("touch Status is:%d\n", relayState);
  digitalWrite(D1, relayState);
  //  postReq("relay='" + String(relayState) + "'");
  touchFlag = 0;
  timerFlag = 0;
}

void postReq(String data) 
{
  static bool eventRequestOpenResult;
  if (eventReq.readyState() == readyStateUnsent || eventReq.readyState() == readyStateDone) 
  {
    eventRequestOpenResult = eventReq.open("POST", "http://espike.ir/esp.php");
    eventReq.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
    if (eventRequestOpenResult)
      eventReq.send(data);
  }
}

void getEspData() 
{
  static bool requestOpenResult;
  if (httpReq.readyState() == readyStateUnsent || httpReq.readyState() == readyStateDone) 
  {
    requestOpenResult = httpReq.open("POST", "http://espike.ir/esp.php");
    httpReq.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
    if (requestOpenResult)
      httpReq.send("apiKey=" + apiKey + "&command=getEspData");
  }
}

void postReqCB(void* optParm, AsyncHTTPRequest* request, int readyState) 
{
  (void)optParm;

  if (readyState == readyStateDone) 
  {
    if (request->responseHTTPcode() == 200) 
    {
      String cmd = request->responseText();
      Serial.println(F("\n******RECIVE******"));
      Serial.println(cmd);
      Serial.println(F("******************"));
      proccessCommand(cmd);
      lastTimeConnection = millis();
    }
  }
}

void eventCB(void* optParm, AsyncHTTPRequest* request, int readyState) 
{
  (void)optParm;

  if (readyState == readyStateDone)
  {
    if (request->responseHTTPcode() == 200) 
    {
      String cmd = request->responseText();
      Serial.println(F("\n*******POST*********"));
      Serial.println(cmd);
      Serial.println(F("*************"));
    }
  }
}

void proccessCommand(String payload) 
{
  int values[2];
  for (int i = 0; i < 2; i++) {
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
  if (values[1] != 0) {
    timer = values[1] * 1000;
    timerLastVal = millis();
    timerFlag = 1;
    //    postReq("time='0'");
  }
  digitalWrite(D1, relayState);
  //  Serial.println("Dalaaaaaam");
}

void handleUrl() 
{
  String str = "Dalam";
  server.send(200, "text/html", str);
}

String scanNetwork() 
{
  int numberOfNetworks = WiFi.scanNetworks();
  String networks = "networks=";
  for (int i = 0; i < numberOfNetworks; i++) 
  {
    if (i == numberOfNetworks - 1) 
      networks += WiFi.SSID(i);
    else
      networks += WiFi.SSID(i) + ",";
  }
  return networks;
}

void localServerCmd() 
{
  String APIKey = server.arg(0);
  if (APIKey == apiKey) 
  {
    String cmd = server.arg(1);
    if (cmd == "scan") 
    {
      Serial.println("scaning");
      server.send(200, "text/html", scanNetwork());
    }
    else if (cmd == "checkConnection") 
    {
      server.send(200, "text/html", "Local OK");
    } 
    else if (cmd == "relay") 
    {
      Serial.println("switching");
      relayState = server.arg(2).toInt();
      digitalWrite(D1, relayState);
      server.send(200, "text/html", "OK");
      String relayCmd = "apiKey=parto&command=switchRelay&relayState=" + String(relayState);
      eventFlag = 1;
      postReq(relayCmd);
    }
    else if (cmd == "connect") 
    {
      TryToConnect = 1;
      Serial.println("connecting");
      String WIFI_SSID = server.arg(2);
      String WIFI_PASS = server.arg(3);
      //clear
      for (int i = 0; i < 96; i++) 
        EEPROM.write(i, 0);
      //write
      for (int j = 0; j < WIFI_SSID.length(); j++)
        EEPROM.write(j, WIFI_SSID[j]);
        
      for (int k = 32; k < WIFI_PASS.length() + 32; k++)
        EEPROM.write(k, WIFI_PASS[k - 32]);

      EEPROM.commit();
      server.send(200, "text/html", "Connecting to WiFi");
      transitionToSTATION();
    } 
    else if (cmd == "LocalUse") 
    {
      TryToConnect = 0;
      server.send(200, "text/html", "Local is available");
    } 
    else if (cmd == "time") 
    {
      timer = server.arg(2).toInt() * 1000;
      timerFlag = 1;
      relayState = 1;
      digitalWrite(D1, relayState);
      server.send(200, "text/html", "Timer has been set");
      timerLastVal = millis();
    }
    else if (cmd == "getAppData") 
      server.send(200, "text/html", "relay=" + String(relayState) + "&time=" + String(timer) + "&status=ok");
    else 
      server.send(200, "text/html", "Undefined command");
  } 
  else
    server.send(200, "text/html", "Wrong API-Key");
}

String loadEEPROM(int startAddr, int endAddr) 
{
  String EEPROMdata = "";
  for (int i = startAddr; i < endAddr; i++) 
  {
    char data = char(EEPROM.read(i));
    if (data == '\0') 
      break;
    else 
      EEPROMdata = EEPROMdata + data;
  }
  return EEPROMdata;
}

void StateDiagram() 
{
  if (ps == AP)
    handleAP();
  else if (ps == STATION)
    handleSTATION();
  else if (ps == SERVER)
    handleSERVER();
}

void handleAP()  //30sec connect
{
  server.handleClient();
  if (TryToConnect)
    if (millis() - lastTimeConnection > DISCONNECTTIME)
      transitionToSTATION();
}

void handleSTATION() 
{
  if (WiFi.status() == WL_CONNECTED) 
  {
    transitionToSERVER();
    // lastTimeConnection = millis();
  }
  else if (millis() - lastTimeConnection > DISCONNECTIONTIME)
    transitionToAP();
}

void handleSERVER() {
  if (millis() - lastTimeConnection > DISCONNECTIONTIME)
    transitionToAP();
}

void transitionToAP() {
  ps = AP;
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("Transition to AP mode");
  ticker.detach();
}

void transitionToSTATION() {
  ps = STATION;
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  String WIFI_SSID = loadEEPROM(0, 32);
  String WIFI_PASS = loadEEPROM(32, 96);
  Serial.printf("\nSSID:%s\n", WIFI_SSID);
  Serial.printf("PASS:%s\n", WIFI_PASS);
  Serial.println(WIFI_PASS);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Transition to Station mode");
  lastTimeConnection = millis();
}

void transitionToSERVER() 
{
  ps = SERVER;
  ticker.attach(GET_TIME, getEspData);
  Serial.println("Server");
}