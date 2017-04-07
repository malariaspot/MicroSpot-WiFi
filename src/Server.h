/**
 * Project Untitled
 */

#ifndef _SERVER_H
#define _SERVER_H

class Server {
public: 
	Server();
	void setUp(String hostname);
    void run();
    void success();
    void error();
private: 
  //  Mechanical mechanical;
	WiFiServer serverWifi(80);
	const char* ap_default_psk = "microspot"; ///< Default PSK.
	String prepareHtmlPage(String response);
};

#endif //_SERVER_H