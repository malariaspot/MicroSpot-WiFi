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


def testCommand(command):
    try:
        request = urllib2.Request("http://" + ip + command)
        request.add_header('User-Agent','MicroSpotApp/1.0 +http://malariaspot.org')
        print("Testing... " + command)
        print(urllib2.urlopen(request).read())
    except Exception as inst:
        print("MicroSpot is dead, or disconnected")
        print(request)
        print(inst)
        exit()

# Toggle on and off the serial


testCommand("/toggle?o=0")

time.sleep(3.0)

testCommand("/toggle?o=1")

time.sleep(3.0)

# Check ping command
# testCommand("/ayy/lmao")

# Check home command
testCommand("/home")

# Check move command
testCommand("/move?x=25&y=7.5&f=500")

# Check uniJog command and stopJog commands
# +X
testCommand("/uniJog?c=+X&f=300")

time.sleep(1)

testCommand("/stopJog")

# +Y
testCommand("/uniJog?c=+Y&f=300")

time.sleep(1)

testCommand("/stopJog")

# -X
testCommand("/uniJog?c=-X&f=300")

time.sleep(1)

testCommand("/stopJog")

# -Y
testCommand("/uniJog?c=-Y&f=300")

time.sleep(1)

testCommand("/stopJog")

time.sleep(3)

# Check generic jog command
testCommand("/jog?x=-20&y=-5&f=200&r=true&s=false")

time.sleep(1)

testCommand("/position")

time.sleep(1)

testCommand("/jog?x=25&y=7.5&f=200&r=false&s=true")

time.sleep(5.0)

# Check the lights
for i in range(3):
    testCommand("/light?l=255")
    time.sleep(0.5)

    testCommand("/light?l=0")
    time.sleep(0.5)


# Back to the center
testCommand("/move?x=25&y=7.5&f=500")

# Pan in circles until the server dies, or until forever.
while True:
    if time.time() - timestamp > frequency:
        timestamp = time.time()
        count = count + 1
        list = c.next()
        x = '%.3f' % list[0]
        y = '%.3f' % list[1]
        try:
            before = time.time()
            testCommand("/pan?x=" + x + "&y=" + y + "&f=" + f)
            after = time.time()
        except Exception as inst:
            print(inst)
            break;
        print(str(count) + " delay: " + str(after - before))

print("MicroSpot died after " + str(count) + " requests, at a rate of " + str(frequency) + " seconds per petition")
