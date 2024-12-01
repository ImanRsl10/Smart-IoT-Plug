#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <AsyncHTTPRequest_Generic.h>

Ticker ticker;

AsyncHTTPRequest httpReq;
//AsyncHTTPRequest httpPost;
//ICACHE_RAM_ATTR void touchInterrupt();

#define GET_TIME 1
#define TOUCH_DELAY_TIME 500

const char *WIFI_SSID = "Hajiton";
const char *WIFI_PASSWORD = "amir123123";
const char *postURL = "http://espike.ir/time.php";
const char *getURL = "http://espike.ir/mic1.php";

int relayState, getFlag, timer, timerLastVal, timerFlag;
volatile int lastTouchTime = 0, touchFlag = 0;

WiFiClient client;


void setup()
{
  Serial.begin(115200);
  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);

  attachInterrupt(digitalPinToInterrupt(D2), touchInterrupt, FALLING);

  httpReq.onReadyStateChange(pgReqCB);
  ticker.attach(GET_TIME, getReq);

  //httpPost.onReadyStateChange(postReqCB);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(1000);
  Serial.print("Proccessing");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected");
}

void loop()
{
  /*if (millis() % TIME == 0)
    {
    getFlag = 1;
    }
    if (getFlag)
    {
    getReq();
    }*/

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
  int httpCode;
  Serial.print("[HTTP] begin...\n");
  httpCode = httpReq.open("GET", getURL);
  if (httpCode) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    //Serial.println(httpReq.method());
    httpReq.send();
  }
  else {
    Serial.printf("[HTTP] GET... failed\n");
  }
  getFlag = 1;
}

void pgReqCB(void* optParm, AsyncHTTPRequest* request, int readyState) {
  (void) optParm;

  if (readyState == readyStateDone)
  {
    String command = request->responseText();
    Serial.println(F("\n********GET*************************"));
    Serial.println(command);
    Serial.println(F("**************************************"));

    request->setDebug(false);
    if (getFlag)
    {
      proccessCommand(command);
      getFlag = 0;
    }
  }
}

void postReq(String data)
{
  httpReq.open("POST", postURL);
  httpReq.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = httpReq.send(data);
  Serial.printf("POST .... Code:%d\n", httpCode);
  /*String content = String(httpPost.send());
    Serial.println(content);*/
}

void postReqCB(void* optParm, AsyncHTTPRequest* request, int readyState) {
  (void) optParm;

  if (readyState == readyStateDone)
  {
    String cmd = request->responseText();
    Serial.println(F("\n*********POST*********************"));
    Serial.println(cmd);
    Serial.println(F("**************************************"));

    request->setDebug(false);
    Serial.println(cmd);
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
  if (values[1] != 0)
  {
    timer = values[1] * 1000;
    timerLastVal = millis();
    timerFlag = 1;

    postReq("time='0'");
  }
  digitalWrite(D1, relayState);
}
