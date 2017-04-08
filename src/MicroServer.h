/**
 * Project Untitled
 */

#ifndef _MICROSERVER_H
#define _MICROSERVER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#include "FileManager.h"

class MicroServer {
	public: 
		MicroServer();
		void setUp(String hostname);
		void run();
		void success();
		void error();
	private: 
	//  Mechanical mechanical;
		const char* ap_default_psk = "microspot"; ///< Default PSK.
		String prepareHtmlPage(String response);
		FileManager fileManager;
		WiFiClient client;
};

#endif //_SERVER_H
