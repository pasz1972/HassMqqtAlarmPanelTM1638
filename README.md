# HassMqqtAlarmPanelTM1638
ESP8266 and TM1638 based Alarm Panel

## Introduction

With the introduction of Home Assistant's MQTT Alarm Panel, it is very easy to create a WiFi enabled alarmpanel to arm and disarm your alarm.

## libraries

### TM1638
The great tm1638 library can be found here and should be installed manually
https://github.com/rjbatista/tm1638-library/wiki

### PubSub
Can be found in the standard Arduino library
https://github.com/knolleary/pubsubclient

## Hardware
NodeMcu or Wemos D1 boards are cheap and already have a usefull powersupply. Any other ESP8266 (except for the ESP-01) can be used.
The TM1638 LED panel can be found at the usual eBay or Chinese providers.

## Wiring
|Panel|ESP |color|
|-----|----|-----|
|vcc  |3.3V|white|
|gnd  | GND|blue|   
|stb  |  D2|orange|      
|clk  |  D1|yellow|
|dio  |  D0|green|

## Home Assistant
Just add the following lines to your configuration.yaml and you are good to go
```
alarm_control_panel:
  - platform: manual_mqtt
    state_topic: home/alarm/state
    command_topic: home/alarm/set
```
