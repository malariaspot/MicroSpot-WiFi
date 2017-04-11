#include <Arduino.h>

class FileManager
{
	public:
		FileManager();
		bool loadWifiConfig(String *ssid, String *pass);
		bool saveWifiConfig(String *ssid, String *pass);

};