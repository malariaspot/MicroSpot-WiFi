#ifndef MICROSERVER_H
#define MICROSERVER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#include <string>
#include <tuple>

#include "FileManager.h"

class Mechanical;

class MicroServer {

	public: 
		MicroServer(Mechanical *m);
		void setUp(String hostname);
		void run();
		void update(String msg);
	private: 
		const char* ap_default_psk = "microspot"; ///< Default PSK.
		void handleClient();
		void handleHomeAxis();
		void handleMoveAxis();
		void handleJogAxis();
		void handleStopJog();
		void handleAyyLmao();
		void handleUnlockAxis();
		void handleToggle();
        void handleToggleLight();
        void handleGetPos();
		FileManager fileManager;
		Mechanical *mechanical;
};

#endif //MICROSERVER_H
