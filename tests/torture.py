import urllib.request as urllib2
import time
import math
import sys
import re

frequency = 0.15
count = 0

class CircleIterator:
    def __init__(self):
        self.angle = 0
    def __iter__(self):
        return self
    def next(self):
        self.angle = self.angle + 5
        return [0.2*math.cos(self.angle*math.pi/180),0.2*math.sin(self.angle*math.pi/180)]


pattern = re.compile("[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}")

try:
    ip = str(sys.argv[1])
except:
    print("Please provide an IP")
    exit()
if pattern.match(ip) is None:
    print("Please provide a proper IP")
    exit()

c = CircleIterator()
timestamp = time.time()
speed = 6/frequency
f = '%.3f' % speed

try:
    request = urllib2.Request("http://" + ip + "/ayy/lmao")
    request.add_header('User-Agent','MicroSpotApp/1.0 +http://malariaspot.org')
    print(urllib2.urlopen(request).read())
    request = urllib2.Request("http://" + ip + "/home")
    request.add_header('User-Agent','MicroSpotApp/1.0 +http://malariaspot.org')
    print(urllib2.urlopen(request).read())
    request = urllib2.Request("http://" + ip + "/move?x=25&y=7.5&f=500")
    request.add_header('User-Agent','MicroSpotApp/1.0 +http://malariaspot.org')
    print(urllib2.urlopen(request).read())
except:
    print("MicroSpot is dead, or disconnected")
    exit()

while True:
    if time.time() - timestamp > frequency:
        timestamp = time.time()
        count = count + 1
        list = c.next()
        x = '%.3f' % list[0]
        y = '%.3f' % list[1]
        try:
            request = urllib2.Request("http://" + ip + "/pan?x=" + x + "&y=" + y + "&f=" + f)
            request.add_header('User-Agent','MicroSpotApp/1.0 +http://malariaspot.org')
            before = time.time()
            print(urllib2.urlopen(request).read())
            after = time.time()
        except Exception as inst:
            print(inst)
            break;
        print(str(count) + " delay: " + str(after - before))

print("MicroSpot died after " + str(count) + " requests, at a rate of " + str(frequency) + " seconds per petition")
