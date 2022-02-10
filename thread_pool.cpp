#include <thread_pool.h>

thread_pool::thread_pool(uint t_number)
{
	start(t_number);
}

thread_pool::~thread_pool()
{
	stop();
}

void thread_pool::start(uint t_number)
{
	for (uint i = 1; i <= t_number; i++)
	{
		
		vThreads.emplace_back(thread([=] {

			while (true)
			{
				Task task;

				
				{
					unique_lock<mutex> mlock{ M };		
					cond.wait(mlock, [=] { return mStopping || !qTasks.empty(); });		

					if (mStopping && qTasks.empty()) break; 
					task = move(qTasks.front());			
					qTasks.pop();							
				}
				
				task();
			}
			}));
	}
}


void thread_pool::stop()
{
	{
		unique_lock<mutex> mlock{ M };
		mStopping = true; 
	}
	cond.notify_all();

	for (auto& t : vThreads)
	{
		t.join();
	}
}

void thread_pool::enqueue(Task task)
{
	unique_lock<mutex> lock{ M };
	qTasks.push(move(task));
}