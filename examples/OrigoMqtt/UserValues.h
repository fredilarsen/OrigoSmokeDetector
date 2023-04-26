// This file contains user-defined constants for the origo-mqtt example.

// Whether to print debug information to Serial. Typical use is to define the two on top, run it to scan and find
// the SEQUENCE_HIGHBITS and SEQUENCE_LOWBITS to insert fiurther down, then undefine the debug prints when all working.
#define DEBUG_PRINT                  // Print basic debug information (must be defined to define the other 2 below)
#define DEBUG_PRINT_SIGNAL           // Whether to print complete signals and sequence bits (SCAN for bits)
//#define DEBUG_PRINT_PARTIAL_SIGNAL // Whether to also print incomplete signals over a certain length.

// Note that this sequence is set by the main unit, and will change if it is reset.
// Do a SCAN operation to find them, then configure them here.
const uint32_t SEQUENCE_HIGHBITS = 0x5AAA,
               SEQUENCE_LOWBITS  = 0xA965A600;

// MQTT broker information (IP address, port number and top topic)
uint8_t BROKER_IP[] = { 127,0,0,1 };
uint16_t BROKER_PORT = 1883;
const char *ROOT_TOPIC = "origo";
               
// WiFi network to connect to
const char* WIFI_SSID     = "MyNetworkSSID";
const char* WIFI_PASSWORD = "MyNetworkPassword";

// Digital input pin where SRX882 is connected
const uint8_t PIN_RADIORECEIVER  = D5;