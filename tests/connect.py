import requests
import sys
import re

#If you can't import the request module,
#use the following command.
#sudo pip install requests

pattern = re.compile("[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}")

def usage():
    print("Usage: python tests/connect.py <ip> <ssid> <pass>")
    print("If ssid or pass contains spaces, use quotes")

try:
    ip = str(sys.argv[1])
except:
    print("Please provide an IP")
    usage()
    exit()
if pattern.match(ip) is None:
    print("Please provide a proper IP")
    usage()
    exit()

try:
    ssid = str(sys.argv[2])
except:
    print("Please provide an ssid")
    usage()
    exit()


try:
    password = str(sys.argv[3])
except:
    password = ''

print("Trying to connect to network: " + ssid)
print("with password: " + password)

url='http://' + ip +'/connect'
headers ={'User-Agent': 'MicroSpotApp'}
data = {'ssid': ssid, 'pass': password }
r = requests.post(url, headers=headers, data=data)

print("====SENT REQUEST====")
print(r.request.headers)
print(r.request.body)

print("====RECEIVED RESPONSE====")
print(r.text)
