#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <iostream>
#include <mutex>


using namespace std;
typedef unsigned int uint;
using Task = function<void()>;


class thread_pool
{
private:
	bool mStopping = false;
	vector<thread> vThreads;
	mutex M;
	condition_variable cond;
	queue<function<void()>> qTasks;


	void start(uint t_number);
	void stop();
public:
	thread_pool(uint t_number = thread::hardware_concurrency());
	~thread_pool();
	void enqueue(Task task);
};
