//***************** Logging data from 433MHz radio receiver ********************


// These can be defined to enable printing status info to serial
// #define DEBUG_PRINT
// #define DEBUG_PRINT_SIGNAL
// #define DEBUG_PRINT_PARTIAL_SIGNALS


class OrigoSmokeDetectorListener {
private:
  uint8_t trig_cnt = 0;        // Count of received signals in the last interval
  uint32_t trig_cnt_start = 0; // When the signal count was restarted
  
  uint8_t pin_radio = 2; // Digital input pin where SRX882 is connected

  // The sequence following the 8 bits of the smoke detector device ID.
  // Note that this sequence is set by the main unit, and will change if it is reset.
  // Do a SCAN operation to find them, then configure them here.
  uint32_t      sequence_highbits = 0x5AAA,
                sequence_lowbits  = 0xA965A600;
  const uint8_t SEQUENCE_ID_MASK  = 0xFF; // The bits containing the smoke detector ID

  // All widths are in microseconds
  const uint8_t  SEQUENCE_LEN = 51;  // Number of bits in a sequeuence. First bit is a high pulse, second is low etc. 32 + 19
  const uint16_t NARROW_PULSE = 450, // A narrow high or low pulse, signalling a logical 1
                 WIDE_PULSE = 1250,  // A wide high or low pulse, signalling a logical 0
                 NARROW_TOLERANCE = 80,
                 WIDE_TOLERANCE = 100;

  uint16_t radio_listen_time = 110; // ms, more than twice the signal length, enough to ensure picking up a signal

  // Buffer and work variables for logging the input pulses
  bool is_high = false;           // If previous measurement was high
  uint32_t last_flank_micros = 0, // Time of previous flank
           last_signal_ms = 0;    // Time of last recognized signal (from detector_id)    
  uint8_t detector_id = 0,        // 8 bit ID of originating smoke detector
          last_detector_id = 0,   // The ID if the last received signal, not cleared
          seq_pos = 0,            // Next bit pos in the sequence to be received
          phase = 0;              // Keeping track of phases of scanning (0=start, 1=got sample but no flank, 2=got flank)
  uint32_t buf[2];                // A 64 bit buffer to collect the signal into (filling lowest bits first)

#ifdef DEBUG_PRINT_SIGNAL
#define DBG_BUFLEN 150
  uint8_t dbg_cnt = 0;
  uint32_t pulse_widths[DBG_BUFLEN];
  void dbg(uint32_t pulse_width) { if (dbg_cnt < DBG_BUFLEN) pulse_widths[dbg_cnt++] = pulse_width; }
#endif


  void debug_print_sequence() {
#ifdef DEBUG_PRINT_SIGNAL
#ifndef DEBUG_PRINT_PARTIAL_SIGNALS
    if (seq_pos != 51) return;
#endif  
    Serial.println();
    Serial.print("Sequence: ");
    for (uint8_t i=0; i<seq_pos; i++) {
      uint8_t byte_index = i > 31 ? 1 : 0, bit_index = i - byte_index*32;
      Serial.print((buf[byte_index] & (((uint32_t) 1)<<bit_index)) != 0 ? '1' : '0');
    }
    Serial.println();
    Serial.print("Widths: ");
    for (uint8_t i=0; i<dbg_cnt; i++) { Serial.print(pulse_widths[i]); Serial.print(' '); }
    Serial.println(); 
    // Print constant if we got a full sequence
    if (seq_pos == 51) {
      Serial.print("HIGHBITS = 0x");
      Serial.println(buf[1], HEX);
      Serial.print("LOWBITS  = 0x");
      Serial.println(buf[0] & ~SEQUENCE_ID_MASK, HEX);
      Serial.print("DEVICEID = 0x");
      Serial.print(buf[0] & SEQUENCE_ID_MASK, HEX);
      Serial.print(" (");
      Serial.print(buf[0] & SEQUENCE_ID_MASK);
      Serial.println(")");
    }
    dbg_cnt = 0;
#endif
  }

  void restart_scanning() {
    if (seq_pos > 30) debug_print_sequence();  
    seq_pos = 0; // Restart scanning
    detector_id = 0;
    buf[0] = buf[1] = 0;
#ifdef DEBUG_PRINT_SIGNAL
    dbg_cnt = 0;
#endif
  }

  void add_to_sequence(uint8_t high_bit) {
    uint8_t byte_index = seq_pos > 31 ? 1 : 0, bit_index = seq_pos - byte_index*32;
    if (high_bit) buf[byte_index] = buf[byte_index] | ((uint32_t) 1) << bit_index;
    seq_pos++;
  }

  bool is_valid_pulse(uint32_t pulse_width, bool high_pulse, uint8_t seq_pos, bool &is_high_bit) {
    // Does the pulse have an expected width (narrow or wide)?
    bool is_narrow = pulse_width > NARROW_PULSE - NARROW_TOLERANCE && pulse_width < NARROW_PULSE + NARROW_TOLERANCE,
         is_wide = pulse_width > WIDE_PULSE - WIDE_TOLERANCE && pulse_width < WIDE_PULSE + WIDE_TOLERANCE;
    if (!is_narrow && !is_wide) return false; // Not an accepted pulse width, probably noise
    if (high_pulse != (0 == (seq_pos % 2))) return false; // Expecting every second bit to be a high pulse independent of width
    is_high_bit = is_wide;
    return true;
  }

  bool recognized_sequence(uint8_t &detector_id) {
    if ((buf[0] & ~SEQUENCE_ID_MASK) == (sequence_lowbits & ~SEQUENCE_ID_MASK) && buf[1] == sequence_highbits) {
      detector_id = (uint8_t) (buf[0] & SEQUENCE_ID_MASK); // Extract 8 lowest bits for the smoke detector ID
      last_signal_ms = millis();
      trig_cnt++;
      return true;
    }
    return false;
  }

  void shift_buffer_one_bit() {
    // The sequence is long enough but does not match. Try to discard the first sample,
    // shifting the window we are looking at.
    buf[0] = buf[0] >> 1; // Drop oldest bit of oldest byte
    buf[0] |= (buf[1] & 1)<<31; // Copy oldest bit of newest byte to newest position of oldest byte
    buf[1] = buf[1] >> 1; // Drop oldest bit of newest byte
    seq_pos--; // We removed the oldest bit, ready for more
  }

  // Listen for more data. Return detector id if complete sequence found, otherwise 0.
  uint8_t listen_radio() {
    bool high = (digitalRead(pin_radio) == HIGH);
    uint32_t now = micros();
    bool found = false;

    // If we have restarted scanning, the first round is just to get the current state
    if (phase == 0) { is_high = high; phase = 1; return 0; } // Got a sample, go to phase 1
  
    if (high != is_high) {
      uint32_t pulse_width = (uint32_t)(now - last_flank_micros);
      last_flank_micros = now;
      if (phase == 1) { phase = 2; is_high = high; return 0; } // Got a flank, go to phase 2 (collecting bits)    
      bool is_high_bit;
      if (is_valid_pulse(pulse_width, is_high, seq_pos, is_high_bit) && seq_pos < SEQUENCE_LEN) {
        add_to_sequence(is_high_bit);
#ifdef DEBUG_PRINT_SIGNAL      
        dbg(pulse_width);
#endif
        if (seq_pos == SEQUENCE_LEN) {
          if (recognized_sequence(detector_id)) {
            last_detector_id = detector_id;
            restart_scanning();
            found = true;
          } else { // Complete but unrecognized sequence, shift it and add one more bit
            shift_buffer_one_bit();
            //restart_scanning();
          }
        }
      } else restart_scanning(); // Invalid pulse length
    } 
    else if (((uint32_t) (now - last_flank_micros)) > WIDE_PULSE + WIDE_TOLERANCE) {
      restart_scanning(); // Too wide pulse, or initial long silence
    }
    is_high = high;
    return found ? last_detector_id : 0;
  }

public:

  OrigoSmokeDetectorListener(uint8_t  pin_radio, 
	                           uint32_t sequence_highbits, 
	                           uint32_t sequence_lowbits, 
	                           uint16_t radio_listen_time) {
    this->pin_radio         = pin_radio, 
    this->sequence_highbits = sequence_highbits;
    this->sequence_lowbits  = sequence_lowbits;
    this->radio_listen_time = radio_listen_time;
  }
	  
  void setup() { pinMode(pin_radio, INPUT); }

  uint8_t listen() {
    // Spend some uninterrupted time listening for radio signals
    restart_scanning();
    phase = 0;
    uint32_t start_listen = millis();
    uint8_t detector_id = 0;
    while ((((uint32_t)(millis()-start_listen)) < radio_listen_time) && !detector_id) {
      detector_id = listen_radio();
    }
#ifdef DEBUG_PRINT
    if ((uint32_t)(millis()-trig_cnt_start) > 5000) { 
      if (trig_cnt > 0) {
        Serial.print("ALARM from ID=");
        Serial.print(last_detector_id);
        Serial.print(", count = ");
        Serial.println(trig_cnt);
        trig_cnt = 0;
      }
      trig_cnt_start = millis();
    }
#endif
    return detector_id;
  }
  
  void clear_alarm_count() { trig_cnt = 0; }
  uint8_t get_alarm_count() { return trig_cnt; }
};