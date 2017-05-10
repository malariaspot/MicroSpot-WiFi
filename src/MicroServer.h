#ifndef MICROSERVER_H
#define MICROSERVER_H

#include <WiFiUdp.h>
#include "WiFiServer.h"
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <ESP8266mDNS.h>

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
		void handleWhomst();
		void handleAyyLmao();

		void handleHomeAxis();
		void handleMoveAxis();
		void handleJogAxis();
		void handleStopJog();
		void handleUnlockAxis();
		void handleToggle();
        void handleToggleLight();
        void handleGetPos();
		FileManager fileManager;
		Mechanical *mechanical;
};

#endif //MICROSERVER_H
