import requests
import sys
import re

#If you can't import the request module,
#use the following command.
#sudo pip install requests

pattern = re.compile("[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}")



try:
    ip = str(sys.argv[1])
except:
    print("Please provide an IP")
    exit()
if pattern.match(ip) is None:
    print("Please provide a proper IP")
    exit()

try:
    ssid = str(sys.argv[2])
except:
    print("Please provide an ssid")
    exit()


try:
    password = str(sys.argv[3])
except:
    password = ''



url='http://' + ip +'/connect'
headers ={'User-Agent': 'MicroSpotApp'}
data = {'ssid': ssid, 'pass': password }
r = requests.post(url, headers=headers, data=data)
print(r.request.headers)
print(r.request.body)
print(r.text)
