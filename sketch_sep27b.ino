#include "TSensor.h"
#include "TRom.h"
#include "TTask.h"
#include "CGsmDevice.h"
#include "CUser.h"

#define SERIAL_BAUD_RATE        9600
#define LOOP_DELAY              1u      // ms
#define SENSOR_DELAY            32uL    // ms => should be LOOP_DELAY * 2^n
#define IGNOR_CHECK_DELAY       8uL     // ms => should be LOOP_DELAY * 2^n
#define GSM_MODULE_DELAY        32uL    // ms => should be LOOP_DELAY * 2^n
#define BTN_THRESHOLD           3
#define NO_SENSOR               0uL
#define STORE_EVENT(_A, _B)     TRom::storeEvent(_A, _B, ((CTaskManager::getLoopCounter() * LOOP_DELAY) / 1000uL));

//// Functions
void handleIgnoreMode(void);
void handleInOut(void);
void collectInData(void);
void handleGsmModule();
void checkIgnoreMode(bool bButton, bool bOn);
void checkDoorsState();


// GSM items
void cbkIncomCall(const char* number);
void cbkIncomSms(const char* number, const char* text);
void cbkEcho(const char* text);
void cbkSetIgnor(const char* text);
void cbkAddUser(const char* text);
void cbkBlockUser(const char* text);
void cbkUnblockUser(const char* text);
unsigned long int AdminPhone = 0uL;
const char *EnterNormMode = "Enter normal mode";
typedef void (*tCommandFunc)(const char* text);
struct tCommand
{
  const char* name;
  tCommandFunc func;
  tCommand(const char* n, tCommandFunc f) : name(n), func(f) {};
};
const tCommand smsCommand[] = {
                                tCommand("echo", cbkEcho),
                                tCommand("setIgnor", cbkSetIgnor),
                                tCommand("addUser", cbkAddUser),
                                tCommand("blockUser", cbkBlockUser),
                                tCommand("unblockUser", cbkUnblockUser)
                              };

///// Variables
CGsmDevice  gsmDevice(cbkIncomCall, cbkIncomSms, GSM_MODULE_DELAY);
CUser   gUser;
int LED_alarm = 4;  // DGT
int LED_ignor = 5;  // DGT
int BTN_ignor = 7;  // DGT
int GRK_door_1 = 8; // DGT
int GRK_door_2 = 9; // DGT
byte Global_state = eNo_Alarm;


//// Init function
void setup() 
{
  pinMode (LED_alarm, OUTPUT);
  pinMode (LED_ignor, OUTPUT);
  pinMode (BTN_ignor, INPUT);
  pinMode (GRK_door_1, INPUT);
  pinMode (GRK_door_2, INPUT);

  // Setup communication
  Serial.begin( SERIAL_BAUD_RATE );
  gsmDevice.init();

  // Setup stored data
  TRom::getStoredParams();
  TRom::printEventMem();
  tSensor::SAMPLE_THRESHOLD = 80; //((4 * (int)TRom::DEBOUNCE_TIME) / (5 * (int)SENSOR_DELAY)); // sould be > 80%
  tSensor::ALARM_THRESHOLD = TRom::ALARM_THRESHOLD;
  gUser.init();
  AdminPhone = gUser.getAdmin();

  // Init tasks
  CTaskManager::addTask( collectInData, 1uL, 0uL );
  CTaskManager::addTask( handleIgnoreMode, IGNOR_CHECK_DELAY - 1uL, 3uL );
  CTaskManager::addTask( handleInOut, SENSOR_DELAY - 1uL, 1uL );
  CTaskManager::addTask( handleGsmModule, GSM_MODULE_DELAY - 1uL, 9uL );
  //CTaskManager::addTask( checkDoorsState, (1uL << 12) - 1uL, 43L );

  // Notify about rebooting
  gsmDevice.sendSMS(AdminPhone, "Enter normal mode after rebooting");
}

//// Main function
void loop()
{
  CTaskManager::run();
  delay(LOOP_DELAY); // or delayMicroseconds(us), but us should be < 16383
}

/////////////////// Task handlers ////////////////
void collectInData(void)
{
   if (eIgnor_Mode == Global_state) return;
   CSoundSensor::collect();
}

void handleInOut(void)
{
   if (eIgnor_Mode == Global_state) return;
   CSoundSensor::handleInput();
   updateOutputs();
}

void handleGsmModule()
{
   gsmDevice.update();
}

void handleIgnoreMode(void)
{
   checkIgnoreMode(true, false);
}

void checkDoorsState()
{
  static int last_state[2] = {LOW, LOW};
  int state[2] = {digitalRead(GRK_door_1), digitalRead(GRK_door_2)};

  if (last_state[0] != state[0])
  {
    if (LOW == state[0])  gsmDevice.sendSMS(AdminPhone, "Door_1 is closed");
    else gsmDevice.sendSMS(AdminPhone, "Door_1 is opened");
  }

  if (last_state[1] != state[1])
  {
    if (LOW == state[1])  gsmDevice.sendSMS(AdminPhone, "Door_2 is closed");
    else gsmDevice.sendSMS(AdminPhone, "Door_2 is opened");
  }
  
  last_state[0] = state[0];
  last_state[1] = state[1];
}

/////////////////// Other functions ////////////////
void updateOutputs()
{
  static unsigned long int last_section_mask = 0uL;
  unsigned long int section_mask = CSoundSensor::getAlarmMask();

  if ((eNo_Alarm == Global_state) && (0uL != section_mask))
  {// Keep LED on until silent happens
     Global_state = eAlarm_Detected;
     digitalWrite(LED_alarm, HIGH);
     enterAlarmMode(section_mask);
  }
  else if ((eAlarm_Detected == Global_state) && (0uL == section_mask))
  {// No noise is detected by any sensor => LED off
     Global_state = eNo_Alarm;
     digitalWrite(LED_alarm, LOW);
     STORE_EVENT(eNo_Alarm, NO_SENSOR)
     gsmDevice.sendSMS(AdminPhone, EnterNormMode);
  }
  else if ((eAlarm_Detected == Global_state) && (section_mask > last_section_mask))
  {// Alarm was detected on new sections
      enterAlarmMode(section_mask);
  }
  last_section_mask = section_mask;
}

void enterAlarmMode(unsigned long int section_mask)
{
  STORE_EVENT(eAlarm_Detected, section_mask)
  char sms[64] = {0};
  sprintf(sms, "Alarm: section ");
  int len = strlen(sms);
  for (byte i = 0u; i < CSoundSensor::getSensorCounter(); ++i)
  {
     if (section_mask & (1uL << i))
     {
        sprintf(&sms[len], "%d ", i + 1);
        len = strlen(sms);
     }
  }
  gsmDevice.sendSMS(AdminPhone, sms);
}

void checkIgnoreMode(bool bButton, bool bOn)
{
    static long int ignore_entered = 0;
    static int btn_state = 0;
    bool btn_pressed = false;

    if (true == bButton)
    {// Handle HW button state
      int now = digitalRead(BTN_ignor);
      int prev = btn_state;
      if ((LOW == now) && (btn_state != BTN_THRESHOLD)) ++btn_state;
      else if ((HIGH == now) && (btn_state != -BTN_THRESHOLD)) --btn_state;
      btn_pressed = (BTN_THRESHOLD == btn_state) && (BTN_THRESHOLD != prev);
    }
    else
    { // Handle SMS command
      if ((true == bOn) && (eIgnor_Mode != Global_state)) btn_pressed = true;
      else if ((false == bOn) && (eIgnor_Mode == Global_state)) btn_pressed = true;
      btn_state = 0;
    }

    // Handle ignore timeout
    if (eIgnor_Mode == Global_state) ignore_entered -= IGNOR_CHECK_DELAY;
    
    // Change mode
    if ((eIgnor_Mode == Global_state) && ((true == btn_pressed) || (0 > ignore_entered)))
    {// enter monitor/normal mode
        Global_state = eNo_Alarm;
        digitalWrite(LED_ignor, LOW);
        digitalWrite(LED_alarm, LOW);
        ignore_entered = 0;
        STORE_EVENT(eNo_Alarm, NO_SENSOR)
        gsmDevice.sendSMS(AdminPhone, EnterNormMode);
    }
    else if ((true == btn_pressed) && (eIgnor_Mode != Global_state))
    { // enter ignore mode
        Global_state = eIgnor_Mode;
        digitalWrite(LED_ignor, HIGH);
        digitalWrite(LED_alarm, LOW);
        CSoundSensor::reset();
        ignore_entered = (TRom::IGNORE_MODE_TIME / LOOP_DELAY);
        STORE_EVENT(eIgnor_Mode, NO_SENSOR)
        gsmDevice.sendSMS(AdminPhone, "Enter ignor mode");
    }
}

/////////////////// GSM related functions ////////////////
void cbkIncomCall(const char* number)
{
    Serial.print("Incomming call from ");
    Serial.println(number);
}

void cbkIncomSms(const char* number, const char* text)
{
    Serial.print("Incomming SMS ");
    Serial.println(number);
    Serial.println(text);
    do
    {
        // number: "380xxxxxxxxx"
        if (gUser.getAdmin() != static_cast<tPhone>(atol(&number[3])))   break;
        
        byte idx = 0u;
        for (; idx < sizeof(smsCommand)/sizeof(smsCommand[0]); ++idx)
        {
            if (0 == strncmp(text, smsCommand[idx].name, strlen(smsCommand[idx].name)))   break;
        }
        if (sizeof(smsCommand)/sizeof(smsCommand[0]) == idx)  break;

        // text: "command param1 param2"
        smsCommand[idx].func(&text[strlen(smsCommand[idx].name) + 1]);
    }
    while(0);
}

void cbkEcho(const char* text)
{
  Serial.println("echo");
  gsmDevice.sendSMS(gUser.getAdmin(), text);  
}

void cbkSetIgnor(const char* text)
{
  Serial.println("setIgnor");
  if ('0' == text[0])  checkIgnoreMode(false, false);
  else if ('1' == text[0]) checkIgnoreMode(false, true);
}

void cbkAddUser(const char* text)
{
  Serial.println("addUser");
  gUser.updateUser(atol(text));
}

void cbkBlockUser(const char* text)
{
  Serial.println("blockUser");
  gUser.updateUser(atol(text), STATE_BLOCKED);
}

void cbkUnblockUser(const char* text)
{
  Serial.println("unblockUser");
  gUser.updateUser(atol(text));
}
