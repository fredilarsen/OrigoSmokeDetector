/*** This sketch will pick up alarms from Housegard Origo smoke detectors, and will
*    publish these on a MQTT broker for other systems to pick up.
*    Onboard LED will flash quickly for 10s after an alarm has been received.
*/

#include "UserValues.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OrigoSmokeDetector.h>
#include <PJONEthernetTCP.h>
#include <ReconnectingMqttClient.h>


//************************** Time sync ******************************

WiFiUDP ntp_udp;
NTPClient time_client(ntp_udp, TIMESYNC_SERVER, 0, 600000);

void setup_timesync() { 
  time_client.begin();
  time_client.forceUpdate();
}


//************************* MQTT *****************************

ReconnectingMqttClient mqtt(BROKER_IP, BROKER_PORT, ROOT_TOPIC);

void publish_alarm(uint8_t detector_id) {
    String topic = String(ROOT_TOPIC) + "/alarm/" + String(detector_id),
           payload = String(time_client.getEpochTime());
    mqtt.publish(topic.c_str(), (uint8_t*) payload.c_str(), payload.length(), false, 1);  
}

void publish_heartbeat() {
    String topic = String(ROOT_TOPIC) + "/heartbeat", payload = String(time_client.getEpochTime());
    mqtt.publish(topic.c_str(), (uint8_t*) payload.c_str(), payload.length(), false, 1);  
}

void publish_startup() {
    String topic = String(ROOT_TOPIC) + "/startup", payload = String(time_client.getEpochTime());
    mqtt.publish(topic.c_str(), (uint8_t*) payload.c_str(), payload.length(), false, 1);  
}

void setup_mqtt() { publish_startup(); }

void loop_mqtt() {
  mqtt.update();
  // Send heartbeat
  static uint32_t last_heartbeat = 0;
  if ((uint32_t)(millis() - last_heartbeat) >= 10000) {
    last_heartbeat = millis();
    publish_heartbeat();
  }
}


//***************** Logging data from 433MHz radio receiver ********************

const uint16_t RADIO_LISTEN_TIME = 110; // ms, more than twice the sequence length from the Origo detectors

uint8_t detector_id = 0;    // Set to non-zero when an alarm was received by the last listening
uint32_t detector_time = 0; // The time of the last alarm received, in ms. Set to 0 after some seconds.

OrigoSmokeDetectorListener listener(PIN_RADIORECEIVER, SEQUENCE_HIGHBITS, SEQUENCE_LOWBITS, RADIO_LISTEN_TIME);

void loop_origo() {  
  detector_id = listener.listen();
  if (detector_id != 0) { publish_alarm(detector_id); detector_time = millis(); }
  if ((uint32_t)(millis() - detector_time) > 10000) detector_time = 0;
}


//**************************** WiFi *******************************

void setup_wifi() {
  WiFi.begin((char*) WIFI_SSID, (char*) WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG_PRINT	
    Serial.print(".");
#endif	
  }
#ifdef DEBUG_PRINT  
  Serial.printf("\nDHCP assigned IP is %s\n", WiFi.localIP().toString().c_str());
#endif 
}


//**************************** LED ********************************
// On-board LED will flash quickly when an alarm has been received

const uint16_t FAST_BLINK = 100, SLOW_BLINK = 1000;
bool led_on = false;
uint16_t interval = SLOW_BLINK;
uint32_t changed = 0;

void setup_led() { pinMode(LED_BUILTIN, OUTPUT); }	

void loop_led() {  
  interval = detector_time == 0 ? SLOW_BLINK : FAST_BLINK;
  if ((uint32_t)(millis() - changed) > interval) {
	  changed = millis();
	  led_on = !led_on; // Invert state
	  digitalWrite(LED_BUILTIN, led_on ? HIGH : LOW);
  }
}


//********************** Main program **********************

void setup() {
#ifdef DEBUG_PRINT  
  Serial.begin(115200);
  Serial.println("OrigoMqtt starting");
#endif
  setup_wifi();
  setup_timesync();
  setup_led();
  listener.setup();
  setup_mqtt();
}

void loop() {
  time_client.update();
  loop_origo();
  loop_mqtt();
  loop_led();
}
