#ifndef C_ROM_BASE_H_
#define C_ROM_BASE_H_

#include <EEPROM.h>

// Declaration part
struct TRomBase
{
protected:
	static void readData(const unsigned int adr, const byte sz, void* data);
	static unsigned int writeData(const unsigned int adr, const byte sz, const void* data);
};


// Implementation part
void TRomBase::readData(const unsigned int adr, const byte sz, void* data)
{
  byte* ptr = (byte*)data;
  for (unsigned int i = adr; i < (adr + sz); ++i)
  {
      *ptr = EEPROM.read(i);
      ++ptr;
  }
}

unsigned int TRomBase::writeData(const unsigned int adr, const byte sz, const void* data)
{
	  const byte* ptr = (const byte*)data;
	  for (unsigned int i = adr; i < (adr + sz); ++i)
	  {
		  EEPROM.write(i, *ptr);
		  ++ptr;
	  }
	  return adr + sz;
}

#endif