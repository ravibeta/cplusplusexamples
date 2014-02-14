#ifndef _WIN_T_H_
#define _WIN_T_H_ 1
#include <iostream>
#include <memory>
#include <cassert>
#include <windows.h>
#include <process.h>

class Runnable {
public:
	virtual void* run() = 0;
	virtual ~Runnable() = 0;
};

class Thread {
public:
	Thread(std::auto_ptr<Runnable> run);
	Thread();
	virtual ~Thread();
	void start();
	void* join();
private:
	HANDLE hThread;
	unsigned wThreadID;
	// runnable object will be deleted automatically
	std::auto_ptr<Runnable> runnable;
	Thread(const Thread&);
	const Thread& operator=(const Thread&);
	// called when run() completes
	void setCompleted();
	// stores return value from run()
	void* result;
	virtual void* run() {return 0;}
	static unsigned WINAPI startThreadRunnable(LPVOID pVoid);
	static unsigned WINAPI startThread(LPVOID pVoid);
	void printError(LPSTR lpszFunction, LPSTR fileName, int lineNumber);
};

class simpleRunnable: public Runnable {
public:
	simpleRunnable(int ID) : myID(ID) {}
	virtual void* run() {
		std::cout << "Thread " << myID << " is running" << std::endl;
		return reinterpret_cast<void*>(myID);
	}
private:
	int myID;
};

class simpleThread: public Thread {
public:
	simpleThread(int ID) : myID(ID) {}
	virtual void* run() {
		std::cout << "Thread " << myID << " is running" << std::endl;
		return reinterpret_cast<void*>(myID);
	}
private:
	int myID;
};
#endif
