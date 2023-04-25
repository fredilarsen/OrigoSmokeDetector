# OrigoSmokeDetector
A header-only C++ library, plus gateways, for Housegard Origo smoke detectors

## Introduction
The Housegard Origo smoke detectors are interconnected using 433MHz radio communication. If one of them starts an alarm, all the others will sound as well.
These smoke detectors are interconnected but do not have an app or any connectivity to other systems.

These detectors have a more solid interconnection than many others because they are not relying on the presence of a hub/gateway to trigger each other.

This repo implements a library for creating a gateway for these smoke detectors, so that other systems can be notified if an alarm goes off, and also see which detector was triggered.

It contains a few gateways for different protocols, most notably MQTT, but can also be used as a simple libary so you can make yur own program to interface with other systems.

## How
The gateway will pick up the radio transmissions from the smoke detectors, extract the device ID, and send a notification to other systems.

## Hardware
The sketch should run on any device that can be programmed from the Arduino sketches in this repo.

It needs a 433 MHz receiver like the cheap SRX882 (search on eBay).

It is tested in two contexts:

1. On an ESP8266 with a SRX882, using a direct connection to a MQTT broker via WiFi.

2. On an Arduino Nano with a SRX882, communicating on a single-wire bus using no extra hardware. This is based on the ModuleInterface repo that is utilizing PJON on the SWBB strategy.

### MQTT gateway
This gateway sketch can be found in examples/OrigoMqtt, and has been tested on a ESP8266. It is probably able to run on most other WiFi capable Arduino compatible boards as well.

When an alarm is detected, the gateway will publish a message to the MQTT topic origo/<numeric device ID> with a payload of the current millisecond from the ESP, so that a change can be detected every time. This is a quick and dirty alternative to publishing the current epoch time as the ModuleInterface gateway does (because it is time synchronized).
  
Systems like Home Assistant can pick up the MQTT notifications and trigger automations like flashing lights or voice information about which detector has triggered, plus notifications to mobile devices. The imagination is the limit.

### ModuleInterface gateway
This gateway sketch can be found in examples/OrigoModuleInterface and has been tested on an Arduino Nano. It is meant to be part of a ModuleInterface wired bus network (daisy-chained/star/any topology) using only a digital Arduino pin (no communication shields required on the device itself).
The ModuleInterface Master will pick up alarm events and will forward them to other modules that are listening, plus to its own web pages+database and potentially also to a MQTT broker.

## Dependencies
* The library itself does not depend on any other repositories. This means you can easily use it to create gateways to other protocols than the ones that are available here.
* The MQTT gateway depends on the [ReconnectingMqttClient](https://github.com/fredilarsen/ReconnectingMqttClient) repository and the [PJON](https://github.com/gioblu/PJON) repository.
* The ModuleInterface gateway depends on the [ModuleInterface](https://github.com/fredilarsen/ModuleInterface) repository and the [PJON](https://github.com/gioblu/PJON) repository.
