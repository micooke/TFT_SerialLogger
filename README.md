# TFT based TTL serial viewer and SD logger

Usage:
* short press - change BAUD rate (defaults to 9600)
*  long press - start/stop logging data to SD

* Author/s: [Mark Cooke](https://www.github.com/micooke)

This is a simple project that piggy backs on some fantastic libraries that do all the dirty work.
Head over to http://www.RinkyDinkElectronics.com/ for all the required libraries (listed below)

I am not affiliated with RinkyDinkElectronics in any way

## Hardware Requirements
* Arduino nano (recommended)
* TFT with digitiser supported by the UTFT library
* SD card breakout
* SD card < 2GB total capacity (this requirement may change)

## Software Requirements
* UTFT library : [website](http://www.rinkydinkelectronics.com/library.php?id=51), [library direct link](http://www.rinkydinkelectronics.com/download.php?f=UTFT.zip)
* (this requirement may change) tinyFAT library : [website](http://www.rinkydinkelectronics.com/library.php?id=37), [library direct link](http://www.rinkydinkelectronics.com/download.php?f=tinyFAT.zip)
* (this requirement may change) UTFT_tinyFAT library : [website](http://www.rinkydinkelectronics.com/library.php?id=53), [library direct link](http://www.rinkydinkelectronics.com/download.php?f=UTFT_tinyFAT.zip)

## Arduino Nano hookup
             +----+=====+----+
             |    | USB |    |
   SD SCK -- | D13+-----+D12 | -- SD MISO
             |3V3        D11~| -- SD MOSI
             |Vref       D10~| -- SD SS
             | A0         D9~| == Serial Rx
             | A1         D8 | -- Touchscreen IRQ
  TFT RST -- | A2         D7 | -- TFT DB7
   TFT CS -- | A3         D6~| -- TFT DB6
   TFT WR -- | A4         D5~| -- TFT DB5
   TFT RS -- | A5         D4 | -- TFT DB4
             | A6         D3~| -- TFT DB3
             | A7         D2 | -- TFT DB2
TFT VCCIN -- | 5V        GND | -- TFT GND
             | RST       RST |
             | GND       TX1 | -- TFT DB0
      +9V -- | Vin       RX1 | -- TFT DB1
             |  5V MOSI GND  |
             |   [] [] []    |
             |   [] [] []    |
             | MISO SCK RST  |
             +---------------+

Other (optional) connections:
* RD -- 3.3V
* LEDA -- 5V
* VCCIN -- 5V
Note : I haven't connected any of these optional connections
