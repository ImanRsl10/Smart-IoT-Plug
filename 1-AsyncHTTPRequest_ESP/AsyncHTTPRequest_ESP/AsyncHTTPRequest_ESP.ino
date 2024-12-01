#define ASYNC_HTTP_DEBUG_PORT     Serial
#define _ASYNC_HTTP_LOGLEVEL_     1
#define TOUCH_DELAY_TIME 1000

// 300s = 5 minutes to not flooding
#define HTTP_REQUEST_INTERVAL     1  //300

// 10s
#define HEARTBEAT_INTERVAL        5

int status;     // the Wifi radio's status

const char* ssid        = "Hajiton";
const char* password    = "amir123123";
int relayState = 0;
volatile int lastTouchTime = 0, touchFlag = 0;

#if (ESP8266)
#include <ESP8266WiFi.h>
#elif (ESP32)
#include <WiFi.h>
#endif

#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN_TARGET      "AsyncHTTPRequest_Generic v1.7.0"
#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN             1007000

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <AsyncHTTPRequest_Generic.h>             // https://github.com/khoih-prog/AsyncHTTPRequest_Generic

#include <Ticker.h>

AsyncHTTPRequest request;
Ticker ticker;
Ticker ticker1;

/*void heartBeatPrint()
  {
  relayState = !relayState;
  digitalWrite(D1,relayState);
  }*/

void sendRequest()
{
  static bool requestOpenResult;

  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone)
  {
    //requestOpenResult = request.open("GET", "http://worldtimeapi.org/api/timezone/Europe/London.txt");
    requestOpenResult = request.open("POST", "http://espike.ir/time.php");
    Serial.println(requestOpenResult);

    if (requestOpenResult)
    {
      // Only send() if open() returns true, or crash
      request.setReqHeader("Content-Type", "application/x-www-form-urlencoded");
      request.send("relay=0");
    }
    else
    {
      Serial.println(F("Can't send bad request"));
    }
  }
  else
  {
    Serial.println(F("Can't send request"));
  }
}

void requestCB(void* optParm, AsyncHTTPRequest* request, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    Serial.println(F("\n**************************************"));
    Serial.println(request->responseText());
    Serial.println(F("**************************************"));

    request->setDebug(false);
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);
  attachInterrupt(digitalPinToInterrupt(D2), ISR, RISING);

  while (!Serial);

  delay(200);

  Serial.print(F("\nStarting AsyncHTTPRequest_ESP using ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ASYNC_HTTP_REQUEST_GENERIC_VERSION);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print(F("Connecting to WiFi SSID: ")); Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }

  Serial.print(F("\nAsyncHTTPRequest @ IP : "));
  Serial.println(WiFi.localIP());

  request.setDebug(false);

  request.onReadyStateChange(requestCB);
  ticker.attach(HTTP_REQUEST_INTERVAL, sendRequest);

  //ticker1.attach(HEARTBEAT_INTERVAL, heartBeatPrint);

  // Send first request now
  sendRequest();
}

void loop()
{
  if (touchFlag)
  {
    RelayOnTouch();
  }
}

ICACHE_RAM_ATTR void ISR()
{
  if (millis() - lastTouchTime > TOUCH_DELAY_TIME)
  {
    touchFlag = 1;
    lastTouchTime = millis();
  }
}
void RelayOnTouch()
{
  relayState = !relayState;
  Serial.printf("touch Status is:%d\n", relayState);
  digitalWrite(D1, relayState);
  //postReq("relay='" + String(relayState) + "'");
  touchFlag = 0;
  //timerFlag = 0;
}
