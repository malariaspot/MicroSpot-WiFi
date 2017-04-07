/**
 * Project Untitled
 */


#ifndef _SERVER_H
#define _SERVER_H

class Server {
public: 
	Server();

	String prepareHtmlPage(String response);

	void setUp();

    void run();
    
    void success();
    
    void error();
private: 
  //  Mechanical mechanical;
	WiFiServer serverWifi;
};

#endif //_SERVER_H