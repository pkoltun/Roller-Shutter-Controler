#ifndef CONFIGURATION_H
#define CONFIGURATION_H


#include <ArduinoJson.h>
#include <deque>

extern std::deque<String> logQueue;

void addLogMessage(String log);

String getCombinedLog();

class Configuration
{
private:
  void SaveConfigToJsonObject( JsonObject& json);
  String _configurationFile;
public:
  int ButtonUpADC = 580; // R1 10k, R2 2.2k
  int ButtonDownADC = 150; // R1 47K R2 2.2k
  int ADCTollerance = 100;
  int ADCDebounceTime = 100;

  int MaxMoveTime = 40*1000;
  int StopDelay = 5*1000;
  int CurrentPosition = 0;
  int MaxPositionUpdateDelay = 3*1000;
  int ResetTime = 24*3600*1000;
  int MaxPosition = 100;
  String Host="192.168.1.12";
  int Port=8080;
  String Url="/json.htm?type=command&param=udevice&idx=31&nvalue=0&svalue=#LEVEL";

  Configuration(String configurationFile);
  void SetupFromFS();

  void SaveToFS();

  void PrintToString( String& stringObj );

  String GetConfigurationFileName();
};
#endif
