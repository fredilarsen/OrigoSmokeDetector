/*** This sketch will pick up alarms from Housegard Origo smoke detectors, and will
*    publish these on a MQTT broker for other systems to pick up.
*/

#include "UserValues.h"
#include <ESP8266WiFi.h>
#include <OrigoSmokeDetector.h>
#include <PJONEthernetTCP.h>
#include <ReconnectingMqttClient.h>


//************************* MQTT *****************************

ReconnectingMqttClient mqtt(BROKER_IP, BROKER_PORT, ROOT_TOPIC);

void publish(uint8_t detector_id) {
    String topic = String(ROOT_TOPIC) + "/" + String(detector_id),
           payload = String(millis());
    mqtt.publish(topic.c_str(), (uint8_t*) payload.c_str(), payload.length(), false, 1);  
}

void setup_mqtt() { publish(0); } // Send a hello signal as unused detector id 0

//***************** Logging data from 433MHz radio receiver ********************

const uint16_t RADIO_LISTEN_TIME = 110; // ms, more than twice the sequence length from the Origo detectors

uint8_t detector_id = 0; // Set to non-zero when an alarm was received by the last listening

OrigoSmokeDetectorListener listener(PIN_RADIORECEIVER, SEQUENCE_HIGHBITS, SEQUENCE_LOWBITS, RADIO_LISTEN_TIME);

void loop_origo() {  
  uint8_t detector_id = listener.listen();
  if (detector_id != 0) publish(detector_id);
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
  Serial.printf("\nNow listening at IP %s\n", WiFi.localIP().toString().c_str());
#endif 
}


//**************************** LED ********************************
// On-board LED will flash quickly when an alarm has been received

const uint16_t FAST_BLINK = 200, SLOW_BLINK = 1000;
bool led_on = false;
uint16_t interval = SLOW_BLINK;
uint32_t changed = 0;

void setup_led() { pinMode(LED_BUILTIN, OUTPUT); }	

void loop_led() {
  interval = detector_id == 0 ? SLOW_BLINK : FAST_BLINK;
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
  setup_led();
  listener.setup();
  setup_mqtt();
}

void loop() {
  loop_origo();
  mqtt.update();
  loop_led();
}
