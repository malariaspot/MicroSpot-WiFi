#ifndef FILEMANAGER_H
#define FILEMANAGER_H

class FileManager
{
	public:
		FileManager();
		bool loadWifiConf(String *ssid, String *pass);
		bool saveWifiConf(String *ssid, String *pass);

}