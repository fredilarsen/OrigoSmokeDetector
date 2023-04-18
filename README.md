# origo-smokedetector
A hub/gateway for Housegard Origo smoke detectors

*NOTE: Contents will be available soon -- it has been tested but missing some polish*

## Introduction
The Housegard Origo smoke detectors are interconnected using 433MHz radio communication. If one of them starts an alarm, all the others will sound as well.
These smoke detectors are interconnected but do not have an app or any connectivity to other systems.

These detectors have a more solid interconnection than many others because they are not relying on the presence of a hub to trigger each other.

This repo implements a hub/gateway for these smoke detectors, so that other systems can be notified if an alarm goes off.
It is also possible to see which smoke detector has triggered the alarm.

## How
The hub will pick up the radio transmissions from the smoke detectors, extract the device ID, and send a notification to other systems using MQTT.

## Hardware
The sketch should run on any device that can be programmed from the Arduino sketch in this repo.

It needs a 433 MHz receiver like the cheap SRX882 (search on eBay).

It is tested in two contexts:
#. On an ESP8266 that connects to the MQTT broker directly via WiFi.
#. On an Arduino Nano with a SRX882, using the ModuleInterface repo for connectivity to other systems

## Notifications
Notifications will be published to the MQTT topic origo-smokedetector/<numeric device ID>.
