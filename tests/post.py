import requests

#If you can't import the request module,
#use the following command.
#sudo pip install requests

url='http://192.168.4.1/connect'
headers ={'User-Agent': 'MicroSpotApp'}
data = {'ssid': 'your_ssid', 'pass': 'your_pass'}
r = requests.post(url, headers=headers, data=data)
print(r.text)
