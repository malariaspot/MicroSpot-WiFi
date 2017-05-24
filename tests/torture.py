import urllib.request as urllib2
import time
import math

frequency = 0.2
count = 0

class CircleIterator:
    def __init__(self):
        self.angle = 0
    def __iter__(self):
        return self
    def next(self):
        self.angle = self.angle + 5
        return [0.1*math.cos(self.angle*math.pi/180),0.1*math.sin(self.angle*math.pi/180)]

c = CircleIterator()
timestamp = time.time()
speed = 6/frequency
f = '%.3f' % speed

while True:
    if time.time() - timestamp > frequency:
        timestamp = time.time()
        count = count + 1
        list = c.next()
        x = '%.3f' % list[0]
        y = '%.3f' % list[1]
        print(urllib2.urlopen("http://192.168.4.1/pan?x=" + x + "&y=" + y + "&f=" + f).read())
        print(count)
