#ifndef T_ROM_H_
#define T_ROM_H_

#include "TRomBase.h"

// EEPROM addresses of data
#define RESET_COUNTER_ADR       0   // Arduino reset/power on counter
#define DEBOUNCE_TIME_ADR       4   // DEBOUNCE_TIME value in msec (default: 180s)
#define ALARM_THRESHOLD_ADR     8   // ALARM_THRESHOLD value (default: 100)
#define IGNORE_MODE_TIME_ADR    10  // IGNORE_MODE_TIME value in msec (default: 4h)
#define EVENT_DATA_CNT_ADR      19  // Total number of detected events
#define EVENT_DATA_BEG_ADR      20  // Event data beginning
#define EVENT_DATA_REC_LEN      10  // bytes: [reset_cnt, 1][timesatamp, 4][state, 1][Section_mask, 3][reserved, 1]
#define EVENT_DATA_CAPACITY     32  // Max possible event number to be stored to EEPROM


// Declaration part
struct TRom: public TRomBase
{
	static byte Event_detected;
	static byte Reset_cnt;
	static long int DEBOUNCE_TIME;
	static long int IGNORE_MODE_TIME;
	static int		ALARM_THRESHOLD;
	
	static void storeEvent(byte state, unsigned long int mask, unsigned long int secTm);
	static void printEventMem();
	static void getStoredParams();
};

// Implementation part
byte TRom::Event_detected = 0u;
byte TRom::Reset_cnt = 0u;
long int TRom::DEBOUNCE_TIME = 10000L;  // Should be 180s
long int TRom::IGNORE_MODE_TIME = 2 * 60 * 1000L; // Should be 4h
int		 TRom::ALARM_THRESHOLD = 100;	// max signal diff


void TRom::storeEvent(byte state, unsigned long int mask, unsigned long int secTm)
{
   int adr = EVENT_DATA_BEG_ADR + (TRom::Event_detected % EVENT_DATA_CAPACITY) * EVENT_DATA_REC_LEN;
   EEPROM.write(adr++, TRom::Reset_cnt);
   adr = writeData(adr, sizeof(secTm), &secTm);
   EEPROM.write(adr++, state);
   adr = writeData(adr, sizeof(mask) - 1, &mask);
   EEPROM.write(adr++, 0x00); // reserved
   ++TRom::Event_detected;
   EEPROM.write(EVENT_DATA_CNT_ADR, TRom::Event_detected);
}


void TRom::printEventMem()
{
  byte value = EEPROM.read(EVENT_DATA_CNT_ADR);
  Serial.print("Total event number: ");
  Serial.println(value);
  for (int i = 0; i < EVENT_DATA_CAPACITY; ++i)
  {
     int idx = EVENT_DATA_BEG_ADR + i * EVENT_DATA_REC_LEN;
     Serial.print("--------- Event ");
     Serial.println(i + 1);
     Serial.print("reset: ");
     value = EEPROM.read(idx);
     Serial.println(value);

     unsigned long int seconds = 0;
     byte *ptr = (byte*)(&seconds);
     for (byte j = 0; j < sizeof(seconds); ++j)
     {
        *ptr = EEPROM.read(idx + j + 1);
        ptr++;
     }
     Serial.print("seconds: ");
     Serial.println(seconds);
   
     value = EEPROM.read(idx + 5);
     Serial.print("Global_state: ");
     Serial.println(value);
     
     value = EEPROM.read(idx + 6);
     Serial.print("Section_mask (low): 0x");
     Serial.println(value, HEX);
     value = EEPROM.read(idx + 7);
     Serial.print("Section_mask (middle): 0x");
     Serial.println(value, HEX);
     value = EEPROM.read(idx + 8);
     Serial.print("Section_mask (high): 0x");
     Serial.println(value, HEX);
     
     value = EEPROM.read(idx + 9);
     Serial.print("Reserved: 0x");
     Serial.print(value, HEX);
     Serial.println();
  } 
}

void TRom::getStoredParams()
{
  // Update reset counter
  TRom::Reset_cnt = EEPROM.read(RESET_COUNTER_ADR);
  EEPROM.write(RESET_COUNTER_ADR, ++TRom::Reset_cnt);
  Serial.print("Reset counter = ");
  Serial.println(TRom::Reset_cnt);

  // Get event detected number
  TRom::Event_detected = EEPROM.read(EVENT_DATA_CNT_ADR);
  Serial.print("Event_detected = ");
  Serial.println(TRom::Event_detected);

  // Get another data
  readData(DEBOUNCE_TIME_ADR, sizeof(TRom::DEBOUNCE_TIME), &TRom::DEBOUNCE_TIME);
  Serial.print("DEBOUNCE_TIME = ");
  Serial.println(TRom::DEBOUNCE_TIME);
  
  readData(ALARM_THRESHOLD_ADR, sizeof(TRom::ALARM_THRESHOLD), &TRom::ALARM_THRESHOLD);
  Serial.print("ALARM_THRESHOLD = ");
  Serial.println(TRom::ALARM_THRESHOLD);
  
  readData(IGNORE_MODE_TIME_ADR, sizeof(TRom::IGNORE_MODE_TIME), &TRom::IGNORE_MODE_TIME);
  Serial.print("IGNORE_MODE_TIME = ");
  Serial.println(TRom::IGNORE_MODE_TIME);
}

#endif
