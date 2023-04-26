/* SmokeDetectorOrigoMI sketch. 
   
   This program listens to incoming 433.92MHz signals from Origo smoke detectors.
   It has a fixed sequence of pulses where most are fixed but some contain the ID
   of the originating detector sending the signal.
   
   The sequence (reverse, lowest bits are start of transmission) is: 
   Hex (starting to receive lowest bits first): 0x05AAA55665600FFC
   Hex mask for the 8 ID bits: 0x000000FF
   Pulse length is about 55ms including leading autoadjust pulse and long silence.
   The main sequence is 43ms long. A listening interval of 55+43 = 98 ms should make sure
   to catch a sequence.
   
   When a signal is received, a ModuleInterface message is sent with the time and ID, as an event.
*/

#include "UserValues.h"

// Allow extra time to respond to MI messages, because we spend uninterrupted time scanning for radio messags
#define SWBB_PREAMBLE 2
#define SWBB_MAX_PREAMBLE 105

#include <MIModule.h>
#include <PJONSoftwareBitBang.h>
#include <OrigoSmokeDetector.h>


//***************** Logging data from 433MHz radio receiver ********************

const uint16_t RADIO_LISTEN_TIME = 110; // ms, more than twice the signal length, enough to ensure picking up a signal

uint8_t detector_id = 0;    // Set to non-zero when an alarm was received by the last listening
uint32_t detector_time = 0; // The time of the last alarm received, in ms. Set to 0 after some seconds.

OrigoSmokeDetectorListener listener(PIN_RADIORECEIVER, SEQUENCE_HIGHBITS, SEQUENCE_LOWBITS, RADIO_LISTEN_TIME);

void loop_origo() {  
  uint8_t detector_id = listener.listen();
  if (detector_id != 0) { send_event(detector_id); detector_time = millis(); }
  if ((uint32_t)(millis() - detector_time) > 10000) detector_time = 0;
}

//***************** PJON+ModuleInterface communication ********************

const uint8_t IX_SETTING_MODE = 0;     // 0=Disabled, 1=Enabled

const uint8_t IX_OUTPUT_SMOKEID = 0,
              IX_OUTPUT_ALARM_TIME = 1,
              IX_OUTPUT_ALARM_COUNT = 2;

bool isEnabled = true;

PJONLink<SoftwareBitBang> bus(PJON_ID);
PJONModuleInterface interface("Smoke", bus, "Mode:u1", NULL, "SmokeId:u1 AlarmTm:u4 AlarmCnt:u1");

void setup_bus() {
  bus.bus.strategy.set_pin(PJON_PIN); // use pin for MI/PJON/SWBB communication
  bus.bus.begin();
}

void loop_bus() { interface.update(); }

void send_event(uint8_t smokeId) {
  // Set values to be transferred
  interface.outputs.set_value(IX_OUTPUT_SMOKEID, smokeId);
  interface.outputs.set_value(IX_OUTPUT_ALARM_TIME, interface.get_time_utc_s());
  interface.outputs.set_event(IX_OUTPUT_ALARM_COUNT, listener.get_alarm_count());
  
  // Flag for immediate transfer as events, then send
  interface.outputs.set_event(IX_OUTPUT_SMOKEID, true);
  interface.outputs.set_event(IX_OUTPUT_ALARM_TIME, true);
  interface.outputs.set_event(IX_OUTPUT_ALARM_COUNT, true);
  interface.outputs.set_updated();
  interface.update();
}

void notification_function(NotificationType notification_type, const ModuleInterface *module_interface) { 
  if (notification_type == ntSampleOutputs) listener.clear_alarm_count(); // We are counting signals between each sampling
}


//************************* LED *****************************

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
  Serial.println("OrigoModuleInterface starting");
#endif
  setup_led();
  listener.setup();
  setup_bus();
}

void loop() {
  loop_bus();
  loop_led();
  loop_origo();

  // Enable outputs to be sent when sensors have been read
  if (interface.settings.is_updated()) isEnabled = interface.settings.get_uint8(IX_SETTING_MODE);
}