/**
 * Project Untitled
 */

#ifndef _MICROSERVER_H
#define _MICROSERVER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h> 
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#include <string>
#include <tuple>
#include <cstdlib>

#include <Arduino.h>

#include "FileManager.h"
#include "Mechanical.h"

class MicroServer {
	Mechanical *mechanical;

	public: 
		MicroServer(Mechanical *m);
		void setUp(String hostname);
		void run();
		void success();
		void error();
	private: 
		const char* ap_default_psk = "microspot"; ///< Default PSK.
		String prepareHtmlPage(String response);
		void handleHomeAxis();
		void handleMoveAxis();
		void handleJogAxis();
		void handleStopJog();
		FileManager fileManager;
		WiFiClient client;
		/*std::tuple<int, int, int> strongToFloat(String xs, String ys, String fs){
			std::string::size_type sz; 
    		float x = std::stof (xs ,&sz);
    		float y = std::stof (ys ,&sz);
    		float f = std::stof (fs ,&sz);
		    return std::make_tuple(x, y, f);
		}*/ //FAILS - stof is only c++11 function
};

#endif //_SERVER_H