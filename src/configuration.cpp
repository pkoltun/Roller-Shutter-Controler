#include "configuration.h"
#include <FS.h>

#define LOAD_CONFIG(propertyName) if (json.containsKey( #propertyName )){ propertyName =  json[ #propertyName ]; }
#define LOADSTRING_CONFIG(propertyName) if (json.containsKey( #propertyName )){ propertyName =  json[ #propertyName ].asString(); }

#define SAVE_CONFIG(propertyName) json[#propertyName]=propertyName;
#define SAVESTRING_CONFIG(propertyName) json[#propertyName]=propertyName.c_str();

void addLogMessage(String log)
{
  if(logQueue.size()>100){
    logQueue.pop_back();
  }

  logQueue.push_front("Ms:" + String(millis()) + " : " + log);
}

String getCombinedLog()
{
  String result;

  for(auto msg: logQueue){
    result.concat("\n" + msg );
  }

  return result;
}

Configuration::Configuration(String configurationFile)
{
  _configurationFile = configurationFile;
}
  void Configuration::SaveConfigToJsonObject( JsonObject& json)
  {

    SAVE_CONFIG(ButtonUpADC)
    SAVE_CONFIG(ButtonDownADC)
    SAVE_CONFIG(ADCTollerance)
    SAVE_CONFIG(ADCDebounceTime)
    SAVE_CONFIG(MaxMoveTime)
    SAVE_CONFIG(StopDelay)
    SAVE_CONFIG(CurrentPosition)
    SAVE_CONFIG(MaxPositionUpdateDelay)
    SAVE_CONFIG(ResetTime)
    SAVE_CONFIG(MaxPosition)
    SAVESTRING_CONFIG(Host)
    SAVE_CONFIG(Port)
    SAVESTRING_CONFIG(Url)
  }


  void Configuration::SetupFromFS()
  {
    auto fileName =_configurationFile;
    if (SPIFFS.exists(fileName))
    {
      auto configFile =  SPIFFS.open(fileName, "r");
      size_t size = configFile.size();
       // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());

      LOAD_CONFIG(ButtonUpADC)
      LOAD_CONFIG(ButtonDownADC)
      LOAD_CONFIG(ADCTollerance)
      LOAD_CONFIG(ADCDebounceTime)
      LOAD_CONFIG(MaxMoveTime)
      LOAD_CONFIG(StopDelay)
      LOAD_CONFIG(CurrentPosition)
      LOAD_CONFIG(MaxPositionUpdateDelay)
      LOAD_CONFIG(ResetTime)
      LOAD_CONFIG(MaxPosition)
      LOADSTRING_CONFIG(Host)
      LOAD_CONFIG(Port)
      LOADSTRING_CONFIG(Url)
    }
  }

  void Configuration::SaveToFS()
  {
    auto fileName =_configurationFile;
    auto configFile =  SPIFFS.open(fileName, "w");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    SaveConfigToJsonObject(json);
    json.printTo(configFile);
  }

  void Configuration::PrintToString( String& stringObj )
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    SaveConfigToJsonObject(json);
    json.printTo(stringObj);
  }

  String Configuration::GetConfigurationFileName()
  {
    return _configurationFile;
  }
