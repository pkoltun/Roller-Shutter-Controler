/*
  FSWebServer - Example WebServer with SPIFFS backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done

  access the sample web page at http://esp8266fs.local
  edit the page by going to http://esp8266fs.local/edit
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "webServer.h"
#include "configuration.h"

extern "C" {
  #include "user_interface.h"
}

AsyncWebServer server(80);
//holds the current upload
File fsUploadFile;

boolean switchToAP = false;

DNSServer* dnsServer = NULL;

String _ssid;
String _pass;
boolean connect = false;

//format bytes
String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

boolean isValidNumber(String str){
   for(byte i=0;i<str.length();i++)
   {
      if(isDigit(str.charAt(i))) return true;
        }
   return false;
}

void setupWebServer(Configuration &configuration, std::function<void(int)> move){

  server.serveStatic("/fs", SPIFFS, "/");

  server.serveStatic("/", SPIFFS, "/index.html");
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"heap\":"+String(ESP.getFreeHeap());
    json += ", \"milis\":"+String(millis());
    // json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    request->send(200, "text/json", json);
    json = String();
  });

  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "version" + VERSION);
  });

  server.on("/size", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "total mem:" + String(ESP.getFlashChipRealSize()) +" free mem:" + String( ESP.getFreeSketchSpace()) );
  });

  server.on("/size", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "TOTAL mem:" + String( ESP.getFreeSketchSpace()) );
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Rebooting");
    ESP.restart();
  });

  server.on("/apMode", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Switching to AP");
    switchToAP = true;
  });

  server.on("/fileList", HTTP_GET, [](AsyncWebServerRequest *request){
    Dir dir = SPIFFS.openDir("/");
    String fileList;
    while (dir.next()) {
      String fileName = dir.fileName();

      size_t fileSize = dir.fileSize();
fileList = fileList + "\n" + fileName + "  size: " + formatBytes(fileSize);
    }
    request->send(200, "text/plain", fileList);
  });
  server.on("/setWiFi", HTTP_GET, [](AsyncWebServerRequest *request){
    _ssid = request->arg("ssid");
    _pass = request->arg("pass");
    request->send(200, "text/plain", "Connecting using ssid: " + _ssid + "pass: " + _pass);

    connect = true;
    switchToAP = false;
  });

  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){

    request->send(200, "text/plain", getCombinedLog());

  });

  server.on("/config", HTTP_GET, [&configuration](AsyncWebServerRequest *request){
    String data;
    configuration.PrintToString(data);
    request->send(200, "text/json", data);
  });

  server.on("/changePosition", HTTP_GET, [move](AsyncWebServerRequest *request){
    String position = request->arg("pos");
    if(!isValidNumber(position))
    {
      request->send(200, "text/plain", "Not valid number");
    }
    else
    {
      move(position.toInt());
      request->send(200, "text/plain", position);
    }

  });


  server.on("/config", HTTP_POST, [&configuration](AsyncWebServerRequest *request){
    String data;
    if(request->hasParam("data", true, false) )
    {
      data = request->getParam("data", true, false)->value();
      auto configFile =  SPIFFS.open(configuration.GetConfigurationFileName(), "w");

      configFile.print(data);
    }
    request->send(200, "text/plain", "UPLOADED: "+data);
  });

//called when the url is not defined here
//use it to load content from SPIFFS
server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "NotFound");
});

  server.begin();
  Serial.println("HTTP server started");
}

uint8_t waitForConnectResult( int _connectTimeout) {
  if (_connectTimeout == 0) {
    return WiFi.waitForConnectResult();
  } else {

    unsigned long start = millis();
    boolean keepConnecting = true;
    uint8_t status;
    while (keepConnecting) {
      status = WiFi.status();
      if (millis() > start + _connectTimeout) {
        keepConnecting = false;

      }
      if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
        keepConnecting = false;
      }
      delay(100);
    }
    return status;
  }
}

int connectWifi(String ssid, String pass) {

  //fix for auto connect racing issue
  if (WiFi.status() == WL_CONNECTED) {
    return WL_CONNECTED;
  }
  //check if we have ssid and pass and force those, if not, try with last saved values
  if (ssid != "") {
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    if(WiFi.SSID()) {
      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();

      WiFi.begin();
    }
  }

  int connRes = waitForConnectResult(5000);

  return connRes;
}

bool autoConnect() {


  // read eeprom for ssid and pass
  //String ssid = getSSID();
  //String pass = getPassword();

  // attempt to connect; should it fail, fall back to AP
  WiFi.mode(WIFI_STA);

  if (connectWifi("", "") == WL_CONNECTED)   {

    //connected
    return true;
  }
  switchToAP = true;
  return false;
}

bool serverDisposed = false;

void stopWebServer()
{
  if(!serverDisposed){
    server.~AsyncWebServer();
    serverDisposed = true;
  }
}

void handleWebServer(){
  if(serverDisposed) return;
  if(switchToAP){
    if (dnsServer == NULL)
    {
      dnsServer = new DNSServer();

      WiFi.softAP("ESPConfig");
      delay(500);
      dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer->start(53, "*", WiFi.softAPIP());
    }
    dnsServer->processNextRequest();
  }else {
    if (dnsServer != NULL){
        dnsServer->stop();
        delete dnsServer;
        dnsServer = NULL;

      }
    if (connect)
    {
      connect = false;
      if (connectWifi(_ssid.c_str(),_pass.c_str()) != WL_CONNECTED)
      {
        switchToAP =true;
      }
    }
}
}
