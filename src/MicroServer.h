#ifndef MICROSERVER_H
#define MICROSERVER_H

#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include "WiFiServer.h"
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h> 

#include <string>
#include <tuple>

#include "FileManager.h"

class Mechanical;

class MicroServer {

	public: 
		MicroServer(Mechanical *m);

		void run();
		void setup(String hostname);
		void update(String msg);
		void send(String msg, WiFiClient * client);
	private: 
		const char* ap_default_psk = "microspot"; ///< Default PSK.

		WiFiClient currentClient;
		WiFiClient newClient;
		String url;
		String method;

		FileManager fileManager;
		Mechanical *mechanical;

		void parseRequest();
		void handleClient();
};

#endif //MICROSERVER_H
