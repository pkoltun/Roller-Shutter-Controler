
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "configuration.h"
#include <functional>

void handleWebServer();
void stopWebServer();
void setupWebServer(Configuration &configuration, std::function<void(int)> move);
bool autoConnect();
extern const String VERSION;
#endif /* end of include guard:  */
