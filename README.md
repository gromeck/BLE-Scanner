# BLE-Scanner

ESP32 based Bluetooth Low Energy (BLE) scanner to report presence into an MQTT service.

## Challenge

Nowadays more and more bluetooth devices are around for personal use, like smart phones or smart watches. Why not use these devices to check presence of tenants at home?

There are [already some solutions around](https://github.com/search?q=ble+scan+esp32&type=Repositories). So why add another one?

I was quite astonished how many devices are detected if you scan the air for bluetooth. Only a few of these report their name, so its hard to distinguish these. So I decided to use [macaddress.io](https://macaddres.io) to lookup the vendor by the devices MAC address. I had to realize that only very few vendors can be looked up this way (can't tell so far, if this MAC address register ist the official one).

## Solution

I used an [Espressif ESP32](https://www.espressif.com/en/products/socs/esp32) device which has WiFi and Bluetooth on-board. The bluetooth scan results are published to an MQTT server via WiFi.

The MAC adresses of the scanned devices are looked up via [macaddress.io](https://macaddress.io).

The BLE scanning device is configured via a Web Frontend which is inspired by [Tasmota](https://github.com/arendst/Tasmota).

The BLE-Scanner doesn't need any external circuit -- just flash the software on it and configure the device.

## Initialization Procedure

Whenever the BLE-Scanner starts and is not able to connect to your WiFi (eg. because of a missing configuration), it enters the configuration mode.
In configuration mode the BLE-Scanner opens an WiFi Access Point with the SSID _BLE-Scanner-AP-XX:XX:XX_. Connect to it with your smartphone or notebook and configure at lease the WiFi settings. That restart the BLE-Scanner.

## What is in this respository?

### [BLE-Scanner Sketch](BLE-Scanner/)

This is the sketch for the ESP32 micro controller. Use the [ArduinoIDE](https://www.arduino.cc/en/main/software) to compile and upload into the ESP32 micro controller.

Use the following arduino setup:



### [Helper Script](scripts/)
