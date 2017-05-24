#ifndef MICROSERVER_H
#define MICROSERVER_H

#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include "WiFiServer.h"
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include "FileManager.h"

class Mechanical; //Defined and imported in MicroServer.cpp

class MicroServer {

	public: 

		MicroServer(Mechanical * m); //Constructor
		void run(); //Loop
		void setup(String hostname); //init
		void update(String msg); //Event handler
		void send(int code ,String msg, WiFiClient * client); //Sends a responce message to a specific client

	private: 

		const char* ap_default_psk = "microspot"; ///< Default PSK.
		Mechanical *mechanical; //Mechanical object
		FileManager fileManager; //Config file manager
		WiFiClient currentClient; //client for mechanical
		String request; //Request sent
		String url; //URL segment of the request
		String arg(String arg); //Returns requested argument
		bool hasArg(String arg); //Checks if requested argument exists
		
};

#endif //MICROSERVER_H
