#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <ESPAsyncTCP.h>
#include "configuration.h"

class WebClient
{
   AsyncClient _client;
   IPAddress _ip;
   uint16_t _port;
public:
    void Initialize(Configuration &configuration);
    void SendUpdate();
};


#endif /* end of include guard:
 */
