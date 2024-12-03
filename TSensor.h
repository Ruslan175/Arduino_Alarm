#ifndef T_SENSOR_H_
#define T_SENSOR_H_

#define 	FRAME_SZ                16u // Should be 2^n
#define 	SENSOR_CAPACITY         3u
#define 	USED_ANALOG_PINS        {tSensor(A3), tSensor(A4), tSensor(A5)};
// Init sound sensors (A0 ... A7)
// !!! Sections 8,9 have the same input A7 (but splitted sensors). I.e. ... tSensor(A6), tSensor(A7), tSensor(A7), tSensor(A8),...

enum eGlobalState
{
	  eNo_Alarm = 0,
	  eAlarm_Detected,
	  eIgnor_Mode,
	  eSensor_Issue
};

// Declaration part
struct tSensor
{
  tSensor() = default;
  tSensor(int pn);
  void handleInput();
  void reset();
  void collect();  
  
  static int		ALARM_THRESHOLD;
  static long int	SAMPLE_THRESHOLD;
  int pin;                  	// analog input of connected sensor
  int data[FRAME_SZ]{0};       	// Sensor data storage
  int sample_cnt{0};           	// Alarm qualification counter
  byte state{eNo_Alarm};		// Actual state of alarm
  byte idx{0};        			// Sensor data index
};


class CSoundSensor
{
public:
	static void collect();
	static void handleInput();
	static void reset();
	static byte getSensorCounter();
	static unsigned long int getAlarmMask();

private:
	static tSensor arSensor[SENSOR_CAPACITY]; // one sensor per one section; 
	
	CSoundSensor() = delete;
	~CSoundSensor() = delete;
};


// Implementation part
int 		tSensor::ALARM_THRESHOLD = 100;
long int	tSensor::SAMPLE_THRESHOLD  = 0L;
tSensor CSoundSensor::arSensor[SENSOR_CAPACITY] = USED_ANALOG_PINS;


tSensor::tSensor(int pn) : 
  pin(pn)
{
	pinMode(pin, INPUT);
}

void tSensor::collect()
{
   data[idx & (FRAME_SZ - 1u)] = analogRead(pin);
   ++idx;
}

void tSensor::handleInput()
{
   // Find boundary singnal values
   int minVal = data[0];
   int maxVal = minVal;
   for (byte i = 1u; i < FRAME_SZ; ++i)
   {
       if (data[i] > maxVal) maxVal = data[i];
       else if (data[i] < minVal) minVal = data[i];
   }
   maxVal -= minVal;

   // Update issue counter and issue state
   if (maxVal > ALARM_THRESHOLD)
   {// too much noise
      if (sample_cnt > -SAMPLE_THRESHOLD) --sample_cnt;
      if (-SAMPLE_THRESHOLD == sample_cnt) state = eAlarm_Detected;
   }
   else
   { // no noise
      if (sample_cnt < SAMPLE_THRESHOLD) ++sample_cnt;
      if (SAMPLE_THRESHOLD == sample_cnt) state = eNo_Alarm;
   }
   //Serial.println(sample_cnt);
}

void tSensor::reset()
{
  memset(&data, 0, sizeof(data));
  sample_cnt = 0;
  state = eNo_Alarm;
  idx = 0u;
}

void CSoundSensor::collect()
{
	for (byte i = 0; i < SENSOR_CAPACITY; ++i) arSensor[i].collect();
}

void CSoundSensor::handleInput()
{
	for (byte i = 0; i < SENSOR_CAPACITY; ++i) arSensor[i].handleInput();
}

void CSoundSensor::reset()
{
	for (byte i = 0; i < SENSOR_CAPACITY; ++i) arSensor[i].reset();
}

byte CSoundSensor::getSensorCounter()
{
	return SENSOR_CAPACITY;
}

unsigned long int CSoundSensor::getAlarmMask()
{
	unsigned long int mask = 0uL;
	for (byte i = 0; i < SENSOR_CAPACITY; ++i)
	{
		if (eAlarm_Detected == arSensor[i].state)
		{
			mask |= (1uL << i);
		}
	}
	return mask;
}

#endif