#ifndef C_GSM_DEVICE_H_
#define C_GSM_DEVICE_H_

#include <SoftwareSerial.h>
#include "CMarkerGsm.h"

#define 	RX_PIN				2
#define		TX_PIN				3
#define		BAUD_RATE			9600	// ~1200 chars / sec
#define 	SMS_SEND_TIMEOUT	30000L	// ms
#define 	SMS_TIMEOUT_IDLE	0L


typedef void (*tCbkIncomCall) (const char* number);
typedef void (*tCbkIncomSms) (const char* number, const char* text);

// Declaration part
class CGsmDevice
{
public:
	CGsmDevice(tCbkIncomCall cbkCall, tCbkIncomSms cbkSm, unsigned long int task_delay);
	void init();
	void update(); // should be calling every 30-40 ms
	void initVoiceCall(unsigned long int number);					//	outgoing call
	void stopVoiceCall();											//	This can be run in 15s-20s after initVoiceCall()
	void sendSMS(unsigned long int number, const char* text);		//	outgoing SMS
	void sendSMS(const char* number, const char* text);				//	outgoing SMS   ? Can be deleted ?
	void getStrNumber(unsigned long int number, char *str);
	
private:
	enum eMarker {eIncomCall = 0, eNewSmsNotify, eNewSmsRead, eMarkerSize};
	struct tSmsData
	{
		char phone[14]{0};
		char text[65]{0};
		
		void set(const char* number, const char* t)
		{
			memset(phone, 0, sizeof(phone));
			memset(text, 0, sizeof(text));
			strcpy(phone, number);
			int len = (strlen(t) < sizeof(text) ? strlen(t) : sizeof(text) - 1);
			strncpy(text, t, len);
		}
	};
	
	static SoftwareSerial gsmSerial;
	static const char Last_Stored_SMS;
	static const char* Phone_Prefix;
	CMarkerGsm Markers[eMarkerSize];
	tCbkIncomCall	mCbkIncomCall;
	tCbkIncomSms	mCbkIncomSms;
	const long int mTaskDelay;
	long int	mSmsSendTimer{SMS_TIMEOUT_IDLE};	// we should wait before sending next SMS
	tSmsData*	mPtrPostSms{NULL}; 					// postponed SMS
	
	static void getNewSms();
	static void readSms(char c);
	static void deleteAllSms(char c);
	void pushSms(const char *number, const char* text);
};

// Implementation part
SoftwareSerial CGsmDevice::gsmSerial(RX_PIN, TX_PIN);
const char  CGsmDevice::Last_Stored_SMS = '5';
const char* CGsmDevice::Phone_Prefix = "380";


CGsmDevice::CGsmDevice(tCbkIncomCall cbkCall, tCbkIncomSms cbkSms,  unsigned long int task_delay)
	:	Markers({CMarkerGsm("+CLIP: ", "\"+", '"',  false), // eIncomCall
				 CMarkerGsm("+CMTI: ", "\",", '\n', false), // eNewSmsNotify
				 CMarkerGsm("+CMGR: ", "\"+", '"',  true)}) // eNewSmsRead
	, 	mCbkIncomCall(cbkCall)
	,	mCbkIncomSms(cbkSms)
	, 	mTaskDelay((long int)task_delay)
{
}

void CGsmDevice::init()
{
  gsmSerial.begin(BAUD_RATE);
  delay(8000);
  gsmSerial.println("AT");
  // The calling line identity (CLI) when receiving a call
  gsmSerial.println("AT+CLIP=1"); // +CLIP: "+380981070259",145, ...  (145 - International number type)
  // SMS system into text mode
  gsmSerial.println("AT+CMGF=1");
  // Perform a Net Survey to Show All the Cells’ Information
  //gsmSerial.println("AT+CNETSCAN");	
}

void CGsmDevice::update()
{
	// Read GSM output data
	char ch;
    while(gsmSerial.available() > 0)
    {
        ch = gsmSerial.read();
        for (byte i = 0u; i < CGsmDevice::eMarkerSize; ++i)
        {
            Markers[i].check(ch);
        }
    }

	// Handle GSM output data
    for (byte i = 0u; i < CGsmDevice::eMarkerSize; ++i)
    {
        if (true == Markers[i].isReady())
        {
			switch(i)
			{
				case CGsmDevice::eIncomCall:
					mCbkIncomCall(CMarkerGsm::Buf);
					break;
					
				case CGsmDevice::eNewSmsNotify:
					getNewSms();
					break;

				case CGsmDevice::eNewSmsRead:
					mCbkIncomSms(CMarkerGsm::Buf, CMarkerGsm::Buf2);
					break;					
			}
            Markers[i].reset();
			break;
        }
    }
	
	// Handle SMS timer and postponed SMS
	if (mSmsSendTimer > 0L)	mSmsSendTimer -= mTaskDelay;
	else if (mSmsSendTimer < 0L)
	{
		mSmsSendTimer = SMS_TIMEOUT_IDLE;
		if (NULL != mPtrPostSms) pushSms(mPtrPostSms->phone, mPtrPostSms->text);
	}
}

void CGsmDevice::sendSMS(const char* number, const char* text)
{
	pushSms(number, text);
}

void CGsmDevice::sendSMS(unsigned long int number, const char* text)
{
	char num_str[15]{0};
	getStrNumber(number, num_str);
	pushSms(num_str, text);
}

void CGsmDevice::pushSms(const char *number, const char* text)
{
	if (SMS_TIMEOUT_IDLE == mSmsSendTimer)
	{ // Send SMS now
		char cmd[32]{0};
		gsmSerial.println("AT+CSCS=\"GSM\"");
		sprintf(cmd, "AT+CMGS=\"+%s\"\n", number);
		gsmSerial.println(cmd);
		gsmSerial.print(text);
		gsmSerial.write("\x1A"); // <Ctrl+Z>
		mSmsSendTimer = SMS_SEND_TIMEOUT;
		if (NULL != mPtrPostSms)
		{
			delete mPtrPostSms;
			mPtrPostSms = NULL;
		}
	}
	else 
	{// Postpone sending
		if (NULL == mPtrPostSms)  mPtrPostSms = new tSmsData();
		if (NULL != mPtrPostSms)  mPtrPostSms->set(number, text);
	}	
}

void CGsmDevice::initVoiceCall(unsigned long int number)
{
    char cmd[32]{0};
	char num_str[15]{0};
	getStrNumber(number, num_str);
    sprintf(cmd, "ATD+%s;", num_str);
    gsmSerial.println(cmd);
}

void CGsmDevice::stopVoiceCall()
{
    gsmSerial.println("ATH");
}

void CGsmDevice::getStrNumber(unsigned long int number, char *str)
{
	sprintf(str, "%s%lu", Phone_Prefix, number);
}

void  CGsmDevice::getNewSms()
{// Read the new SMS & delete old ones
    char n = CMarkerGsm::Buf[0];
    readSms(n);
    if (Last_Stored_SMS == n)  deleteAllSms(n);
}

void CGsmDevice::readSms(char c)
{ // Like AT+CMGR=9
    char bf[12]{0};
    sprintf(bf, "AT+CMGR=%c", c);
    gsmSerial.println(bf);
}

void CGsmDevice::deleteAllSms(char c)
{// AT+CMGD=1
    char bf[12]{0};
    for (byte i = 0; i < 9; ++i)
    {
		const char ch = (char)('1' + i);
        memset(bf, 0, sizeof(bf));
        sprintf(bf, "AT+CMGD=%c", ch);
        gsmSerial.println(bf);
        if (c == ch) break;
    }
}

#endif