# GacHAUM

## Hardware

* WeMos D1 mini
* PN532-based reader (ie. Adafruit PN532)
* Relay shield (e.g. Relay Shield V2 for WEMOS D1 mini ; 75mA measured)

## Wiring

Powered by WeMos through its USB port

* 3V3 (WeMos) -> 3V3 (PN532)
* D1 (WeMos) -> SCL (PN532)
* D2 (WeMos) -> SDA (PN532)
* GND (WeMos) -> GND (PN532)
* GND (WeMos) -> GND (Relay)
* +5V (WeMos) -> +5V (Relay)
* D1 (WeMos) -> D3 (Relay)

##  Arduino required libraries

* PubSubClient

This library can be installed using Arduino IDE.

* Seeed-Studio PN532 (forked)

https://github.com/haum/PN532

```
git submodule init
git submodule update
export ARDUINO_LIBS="$HOME/Arduino/libraries"
ln -s $PWD/PN532/PN532 $ARDUINO_LIBS/
ln -s $PWD/PN532/PN532_I2C $ARDUINO_LIBS/
```
