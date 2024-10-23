// This file contains user-defined constants for the origo-mqtt example.

// Whether to print debug information to Serial. Typical use is to define the two on top, run it to scan and find
// the SEQUENCE_HIGHBITS and SEQUENCE_LOWBITS to insert fiurther down, then undefine the debug prints when all working.
#define DEBUG_PRINT                   // Print basic debug information (must be defined to define the other 2 below)
//#define DEBUG_PRINT_SIGNAL          // Whether to print complete signals and sequence bits (SCAN for bits)
//#define DEBUG_PRINT_PARTIAL_SIGNALS // Whether to also print incomplete signals over a certain length.

// Note that this sequence is set by the main unit, and will change if it is reset.
// Do a SCAN operation to find them, then configure them here.
const uint32_t SEQUENCE_HIGHBITS = 0x5AAA,
               SEQUENCE_LOWBITS  = 0xA965A600;

// Digital input pin where SRX882 is connected
const uint8_t PIN_RADIORECEIVER  = 4;

// ModuleInterface/PJON
const uint8_t PJON_ID  = 20; // Device ID
const uint8_t PJON_PIN = 7;  // Signal pin for the bus
