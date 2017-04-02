# MicroSpot WiFi module firmware

## Installation

This firmware is built using PlatformIO. Please go to their website and download the CLI tools, and the IDE.
To set up the firmware just simply clone the repo on your preferred location.

## IMPORTANT NOTE

Because the Arduino UNO WiFi's particular oscillator setting, we have to use a tampered version of the Arduino framework to make this work.
Just open up a terminal and type:

```
pio platform install https://github.com/malariaspot/platform-espressif826640
```

This will be used as a solution until the original frameworks supports 40MHz crystals in a seamless way.
