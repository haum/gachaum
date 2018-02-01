# GacHAUM

## Hardware

* WeMos D1 mini
* PN532-based reader (ie. Adafruit PN532)

## Wiring

* D5 -> SCLK
* D6 -> MISO
* D7 -> MOSI
* D8 -> SSS

##  Arduino required libraries

* PubSubClient

This library can be installed using Arduino IDE.

* Seeed-Studio PN532

https://github.com/Seeed-Studio/PN532

```
git submodule init
git submodule update
export ARDUINO_LIBS="$HOME/Arduino/libraries"
ln -s $PWD/PN532/PN532 $ARDUINO_LIBS/
ln -s $PWD/PN532/PN532_SPI $ARDUINO_LIBS/
```
