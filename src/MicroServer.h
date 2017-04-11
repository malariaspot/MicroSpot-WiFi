/**
 * Project Untitled
 */

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
		void success();
		void error();
	private: 
		const char* ap_default_psk = "microspot"; ///< Default PSK.
		void handleHomeAxis();
		void handleMoveAxis();
		void handleJogAxis();
		void handleStopJog();
		void handleAyyLmao();
		void handleUnlockAxis();
		void handleToggle();
		FileManager fileManager;
		Mechanical *mechanical;
		std::tuple<float, float, float> strongToFloat(String xs, String ys, String fs){
			std::string::size_type sz; 
    		float x = atof(xs.c_str());
    		float y = atof(ys.c_str());
    		float f = atof(fs.c_str());
		    return std::make_tuple(x, y, f);
		}
};

#endif //MICROSERVER_H