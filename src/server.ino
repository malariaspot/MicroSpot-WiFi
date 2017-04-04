#include <ESP8266WiFi.h>

const char* ssid = "********";
const char* password = "********";

WiFiServer server(80);

void setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  server.begin();
}

// prepare a web page to be send to a client (web browser)
String prepareHtmlPage(String response) {
  String htmlPage = String("HTTP/1.1 200 OK\r\n") +
            "Content-Type: text/html\r\n" +
            "Connection: close\r\n" +  // the connection will be closed after completion of the response
            "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
            "\r\n" +
            "<!DOCTYPE HTML>" +
            "<html>" +
            response +
            "</html>" +
            "\r\n";

  return htmlPage;
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) { return; }
  // Wait until the client sends some data
  while(!client.available()){ delay(1); }
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  client.flush();
  // Match the request
  String val;
  if (req.indexOf("/ayy/lmao") != -1) val = req;
  else { 
    client.stop();
    return;
  }
  client.flush();
  // Prepare the response
  client.println(prepareHtmlPage(val));
  // Send the response to the client
  delay(1);
}