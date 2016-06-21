#include "webClient.h"

void onData(void *data, size_t len){
    char* buffer = new char[len + 1];
    buffer[len] = 0;
    for(int i=0; i < len;i++)
    {
      buffer[i] = ((char *)data)[i];
    }

    addLogMessage("Data: "+ String( buffer));
    delete [] buffer;
}

void WebClient::Initialize(Configuration &configuration)
{
  _ip.fromString(configuration.Host);
  _port = configuration.Port;
  String s;
  addLogMessage(_ip.toString() + ":" + String(_port));
  _client.onData([](void *obj, AsyncClient* c, void *data, size_t len){onData(data,len);});
  _client.onConnect([&configuration](void* obj,AsyncClient *c){
    auto positionPercent = (configuration.CurrentPosition*100)/configuration.MaxPosition;
    if (positionPercent < 0) positionPercent = -positionPercent;
    String url(configuration.Url);
    url.replace("#LEVEL", String(positionPercent));

    String request="GET " + url + " HTTP/1.1\r\nHost: "+configuration.Host+"\r\n\r\n";
    addLogMessage(request);
    addLogMessage(String(c->canSend()));
    addLogMessage(String(c->space()) + " " + String(request.length()));
    ((WebClient*)obj)->_client.write(request.c_str(), request.length());
  }, this);
}

void WebClient::SendUpdate()
{
  if(_client.connected())
  {
      _client.close(true);
  }
  _client.connect(_ip, _port);
}
