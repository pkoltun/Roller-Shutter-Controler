// #include <ESP8266WiFi.h>
// #include <ESP8266mDNS.h>
#include <Arduino.h>
#include <Hash.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "ota.h"
#include "webServer.h"
//#include <WiFiManager.h>
#include "switches.h"
#include "configuration.h"
#include "webClient.h"

#undef min
#undef max
#include <vector>

#define POWER_PIN 14
#define MOVE_PIN 13

WebClient webClient;
const String VERSION = "0.12";
const String CONFIGURATION_FILE="/configFile.txt";

std::vector<Switch*> _switches;
Configuration _configuration(CONFIGURATION_FILE);
int lastButtonCheck = 0;
int longClicks = 0;
int lastState = HIGH;

std::deque<String> logQueue;

int currentMoveDirection = 0;
int expectedMoveDirection = 0;
int expectedPosition = -1;

int lastStopTime;
int lastMoveStart = 0;
int lastPositionUpdate = 0;

void stop()
{
  currentMoveDirection = 0;
  expectedMoveDirection = 0;
  digitalWrite(POWER_PIN,LOW);
  digitalWrite(MOVE_PIN,LOW);

  addLogMessage("Stopping");
}

void moveUp()
{
  currentMoveDirection = 1;
  digitalWrite(MOVE_PIN,LOW);
  addLogMessage("Moving up");
}

void moveDown()
{
  currentMoveDirection = -1;
  digitalWrite(MOVE_PIN,HIGH);
  addLogMessage("Moving down");
}

void powerOn()
{
    lastMoveStart = millis();
    digitalWrite(POWER_PIN,HIGH);
    addLogMessage("Power up");
}

void handleMove()
{
  if (currentMoveDirection != 0)
  {
    auto moveTime = millis() - lastMoveStart;
    if ( moveTime > _configuration.MaxMoveTime)
    {
      stop();
      addLogMessage("Max move time exceeded");
      return;
    }

    if (moveTime > _configuration.MaxPositionUpdateDelay && millis() - lastPositionUpdate > _configuration.MaxPositionUpdateDelay )
    {
      stop();
      addLogMessage("No position update in time");
      return;
    }

    auto distanceFromPosition = _configuration.CurrentPosition - expectedPosition;
    if (expectedPosition != -1)
    {
        if (expectedMoveDirection == -1 && distanceFromPosition <= 0)
        {
          stop();
          return;
        }

        if (expectedMoveDirection == 1 && distanceFromPosition >= 0)
        {
          stop();
          return;
        }
    }
  }

  if(expectedMoveDirection == currentMoveDirection)
  {
    return;
  }

  if (expectedMoveDirection == 0)
  {
    stop();
    return;
  }

  if ( millis() - lastStopTime < _configuration.StopDelay)
  {
          return;
  }

  if (expectedMoveDirection == 1)
  {
      moveUp();
      powerOn();
  }
  if (expectedMoveDirection == -1)
  {
      moveDown();
      powerOn();
  }
}


void handleButtonUp( int holdMS )
{
  addLogMessage("Up hold for " + String(holdMS) );

  if (holdMS > 2000)
  {
    longClicks++;
  }
  else
  {
    longClicks = 0;
    if (currentMoveDirection != 0) expectedMoveDirection =0;
    else expectedMoveDirection = 1;
    expectedPosition = -1;
  }
}

void handleButtonDown( int holdMS )
{
  addLogMessage("Down hold for " + String(holdMS) );
  if (currentMoveDirection != 0) expectedMoveDirection =0;
  else expectedMoveDirection = -1;
  expectedPosition = -1;
}

void handleReed( int holdMS )
{
  if (currentMoveDirection !=0)
  {
    _configuration.CurrentPosition += currentMoveDirection;
    _configuration.SaveToFS();
    webClient.SendUpdate();
  }

  addLogMessage("Reed hold for " + String(holdMS) + " postion:" + String(_configuration.CurrentPosition) );
  lastPositionUpdate = millis();
}

void setup() {
  pinMode(POWER_PIN, OUTPUT);
  pinMode(MOVE_PIN, OUTPUT);
    Serial.begin(115200);
  Serial.println("Booting");
  SPIFFS.begin();
  _configuration.SetupFromFS();
  _configuration.SaveToFS();
  Switch* analogUpButton =
          new AnalogSwitch(
            _configuration.ButtonUpADC,
            _configuration.ADCTollerance,
            _configuration.ADCDebounceTime);

  analogUpButton->AddReleasedHandler(handleButtonUp);

  Switch* analogDownButton =
          new AnalogSwitch(
            _configuration.ButtonDownADC,
            _configuration.ADCTollerance,
            _configuration.ADCDebounceTime);

  analogDownButton->AddReleasedHandler(handleButtonDown);

  Switch* reedSwitch = new GPIOSwitch(12,10);
  reedSwitch->AddReleasedHandler(handleReed);
 _switches.push_back(analogUpButton);
 _switches.push_back(analogDownButton);
 _switches.push_back(reedSwitch);

  autoConnect();
  setupOTA();
  setupWebServer(_configuration,[](int position){
    addLogMessage("Setting position: " + String(position) );
    expectedPosition = position;
    if (_configuration.CurrentPosition > position){
      expectedMoveDirection = -1;
    }else
    {
      expectedMoveDirection = 1;
    }
  });
  webClient.Initialize(_configuration);
  Serial.println("Ready");
}

void loop() {
  if ( millis() - lastButtonCheck > 10)
  {
    for(auto button:_switches){
        button->checkState();
    }
    lastButtonCheck = millis();
  }
  handleOTA();
  handleWebServer();
  if (longClicks > 3){
    ESP.reset();
  }

  if (currentMoveDirection == 0 && expectedMoveDirection ==0 && millis() > _configuration.ResetTime){
    ESP.reset();
  }
  handleMove();

}
