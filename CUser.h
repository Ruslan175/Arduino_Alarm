#ifndef C_USER_H_
#define C_USER_H_

#include "TRomBase.h"

#define		USER_DATA_CNT_ADR      	344u	// just after 340: EVENT_DATA_BEG_ADR + EVENT_DATA_REC_LEN * EVENT_DATA_CAPACITY
#define		USER_DATA_BEG_ADR      	345u
#define		USER_DATA_REC_LEN      	5		// bytes: [phone_number, 4][state, 1]
#define		USER_DATA_STATE_OFFSET 	4
#define 	USER_DATA_CAPACITY     	10u		// Max posible user number 
#define 	MIN_VALID_PHONE_NUMBER 	20000000uL
#define 	MAX_VALID_PHONE_NUMBER 	999999999uL

#define 	STATE_BLOCKED			0x01u
#define 	STATE_ADMIN				0x02u

typedef 	unsigned long int	tPhone;


// Declaration part
class CUser : public TRomBase
{
public:
	void init();
	tPhone 	getAdmin() const;
	//byte	getUserCount() const;
	bool updateUser(const tPhone phone, byte state = 0u);

private:
	tPhone 	mAdmin{0uL};
	byte	mDataCnt{0u};
};


// Implementation part
void CUser::init()
{
	mDataCnt = EEPROM.read(USER_DATA_CNT_ADR);
	tPhone phone{0uL};
	byte state{0u};
	unsigned int adr{USER_DATA_BEG_ADR};
	Serial.println("");
	char buf[32]{0};
	for (byte i = 0u; i < mDataCnt; ++i)
	{
		state = EEPROM.read(adr + USER_DATA_STATE_OFFSET);
		readData(adr, sizeof(phone), &phone);
		if (state & STATE_ADMIN)	mAdmin = phone;
		sprintf(buf, "user %lu: %u", phone, state);
		Serial.println(buf);
		adr += USER_DATA_REC_LEN;
	}
}

bool CUser::updateUser(const tPhone phone, byte state /*= 0u*/)
{
	// Validate phone number: expected 9 digitals
	if ((phone < MIN_VALID_PHONE_NUMBER) || (phone > MAX_VALID_PHONE_NUMBER)) return false;
	// Try to update stored user
	unsigned int adr{USER_DATA_BEG_ADR};
	for (byte i = 0u; i < mDataCnt; ++i)
	{
		tPhone t;
		readData(adr, sizeof(t), &t);
		if (phone == t)
		{
			EEPROM.write(adr + USER_DATA_STATE_OFFSET, state);
			return true;
		}
		adr += USER_DATA_REC_LEN;
	}
	// Add new user
	if (USER_DATA_CAPACITY > mDataCnt)
	{
		adr = USER_DATA_BEG_ADR + mDataCnt * USER_DATA_REC_LEN;
		adr = writeData(adr, sizeof(phone), &phone);
		EEPROM.write(adr, state);
		EEPROM.write(USER_DATA_CNT_ADR, ++mDataCnt);
		return true;
	}
	return false;
}

tPhone CUser::getAdmin() const
{
	return mAdmin;
}


/*
byte CUser::getUserCount() const
{
	return mDataCnt;
}
*/
#endif