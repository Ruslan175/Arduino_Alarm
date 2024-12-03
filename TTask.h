#ifndef T_TASK_H_
#define T_TASK_H_

#define TASK_CAPACITY           4u

typedef void (*tFunc)(void);

// Declaration part
struct tTask
{
    tFunc                     func;     // task function
    unsigned long int         mask;     // Filter for Loop_counter
    unsigned long int         phase;    // when func should be called
	
	tTask(tFunc f, unsigned long int m, unsigned long int p) : func(f), mask(m), phase(p) {};
	tTask() = default;
};


class CTaskManager
{
public:
	static bool addTask(tFunc f, unsigned long int m, unsigned long int p);
	static void run();
	static unsigned long int getLoopCounter();
	
private:
	static byte  Task_cnt;
	static unsigned long int  Loop_cnt; 	// Main loop call counter
	static tTask arTask[TASK_CAPACITY];		// Task container
	
	CTaskManager() = delete;
	~CTaskManager() = delete;
};

// Implementation part
byte  CTaskManager::Task_cnt = 0u;
unsigned long int  CTaskManager::Loop_cnt = 1uL;
tTask CTaskManager::arTask[TASK_CAPACITY];


bool CTaskManager::addTask(tFunc f, unsigned long int m, unsigned long int p)
{
	if (TASK_CAPACITY == Task_cnt) return false;
	arTask[Task_cnt++] = tTask(f, m, p);
	return true;
}

void CTaskManager::run()
{
	tTask *ptr = NULL;
	for (byte i = 0; i < Task_cnt; ++i)
	{
		ptr = &arTask[i];
		if (ptr->phase == (ptr->mask & Loop_cnt))  ptr->func();
	}
	++Loop_cnt;
}

unsigned long int CTaskManager::getLoopCounter()
{
	return Loop_cnt;
}

#endif