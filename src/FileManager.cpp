#include "FileManager.h"
#include <Arduino.h>

#include <FS.h>

FileManager::FileManager()
{
  if (!SPIFFS.begin()) { return; }
  // Load wifi connection information.
  if (! loadConfig(&station_ssid, &station_psk)) {
    station_ssid = "";
    station_psk = "";
  }
}

/**
 * @brief Read WiFi connection information from file system.
 * @param ssid String pointer for storing SSID.
 * @param pass String pointer for storing PSK.
 * @return True or False.
 *
 * The config file have to containt the WiFi SSID in the first line
 * and the WiFi PSK in the second line.
 * Line seperator can be \r\n (CR LF) \r or \n.
 */

bool 
FileManager::loadWifiConfig(String *ssid, String *pass)
{
  // open file for reading.
  File configFile = SPIFFS.open("/cl_conf.txt", "r");
  if (!configFile)
  {
    //Replace for an html message.
    ////Serial.println("Failed to open cl_conf.txt.");
    return false;
  }

  // Read content from config file.
  String content = configFile.readString();
  configFile.close();

  content.trim();

  // Check if ther is a second line available.
  int8_t pos = content.indexOf("\r\n");
  uint8_t le = 2;
  // check for linux and mac line ending.
  if (pos == -1) {
    le = 1;
    pos = content.indexOf("\n");
    if (pos == -1) {
      pos = content.indexOf("\r");
    }
  }

  // If there is no second line: Some information is missing.
  if (pos == -1) {
    //Serial.println("Invalid content.");
    //Serial.println(content);
    return false;
  }

  // Store SSID and PSK into string vars.
  *ssid = content.substring(0, pos);
  *pass = content.substring(pos + le);

  ssid->trim();
  pass->trim();

  return true;
}

/**
 * @brief Save WiFi SSID and PSK to configuration file.
 * @param ssid SSID as string pointer.
 * @param pass PSK as string pointer,
 * @return True or False.
 */

bool
FileManager::saveWifiConfig(String *ssid, String *pass)
{
  // Open config file for writing.
  File configFile = SPIFFS.open("/cl_conf.txt", "w");
  if (!configFile) { return false; }
  // Save SSID and PSK.
  configFile.println(*ssid);
  configFile.println(*pass);
  configFile.close();
  return true;
}