#ifndef QTHREADPOOL_HPP
#define QTHREADPOOL_HPP

#include "../../common/sdktypes.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <stdexcept>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "qthreadpool"

template <typename Qtask>
class qthreadpool {
   public:
	explicit qthreadpool(size_t num_threads);
	~qthreadpool();

	// Enqueue a task with a callback for the main thread notification
	template <typename Func, typename Callback, typename... Args>
	auto enqueue(Func&& func, Callback&& callback, int task_id, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>;

	// Stop the thread pool gracefully
	void stop();
	void process_in_main_thread();	// Method for processing callbacks in the main thread

   private:
	static void* worker_thread(void* arg);

	std::vector<pthread_t> workers;						  // Worker threads
	std::queue<Qtask> tasks;							  // Task queue
	std::queue<std::function<void()>> main_thread_queue;  // Main thread callback queue
	std::mutex queue_mutex;								  // Mutex for task queue
	std::mutex main_thread_mutex;						  // Mutex for main thread queue
	std::condition_variable condition;					  // Condition variable for tasks
														  //    std::condition_variable main_thread_condition; // Condition variable for main thread callbacks
	std::atomic<bool> stop_flag;						  // Flag to stop the thread pool
};

template <typename Qtask>
qthreadpool<Qtask>::qthreadpool(size_t num_threads) : stop_flag(false) {
	for (size_t i = 0; i < num_threads; ++i) {
		workers.emplace_back();
		pthread_create(&workers.back(), nullptr, &worker_thread, this);
	}
}

template <typename Qtask>
template <typename Func, typename Callback, typename... Args>
auto qthreadpool<Qtask>::enqueue(Func&& func, Callback&& callback, int task_id, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type> {
	using return_type = typename std::result_of<Func(Args...)>::type;

	// Create a packaged task to wrap the function and its arguments
	auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
	std::future<return_type> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		if (stop_flag) {
			debug_warn(LOG_LEVEL_0, __LOGTAG__, "enqueue on stopped qthreadpool");
		}

		// Enqueue the task for worker threads
		tasks.emplace([task, this, callback, task_id]() {
			(*task)();	// Execute the original task

			// Enqueue the callback to the main thread queue with task_id
			{
				std::lock_guard<std::mutex> lock(main_thread_mutex);
				main_thread_queue.emplace([callback, task_id]() {
					callback(task_id);	// Call the main thread callback with task_id
				});
			}
			//            main_thread_condition.notify_one();  // Notify main thread about new callback
		});
	}

	condition.notify_one();	 // Notify a worker thread that there's a new task
	return res;
}

// Stop function to gracefully stop the thread pool
template <typename Qtask>
void qthreadpool<Qtask>::stop() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop_flag = true;
	}

	// Notify all worker threads
	condition.notify_all();

	// Wait for all threads to finish
	for (auto& worker : workers) {
		pthread_join(worker, nullptr);
	}

	workers.clear();  // Clean up worker threads
	debug_print(LOG_LEVEL_0, __LOGTAG__, "qthreadpool stopped");
}

template <typename Qtask>
qthreadpool<Qtask>::~qthreadpool() {
	stop();	 // Ensure stop is called on destruction
}

template <typename Qtask>
void* qthreadpool<Qtask>::worker_thread(void* arg) {
	auto* pool = static_cast<qthreadpool*>(arg);
	while (true) {
		Qtask task;
		{
			std::unique_lock<std::mutex> lock(pool->queue_mutex);
			pool->condition.wait(lock, [pool] { return pool->stop_flag || !pool->tasks.empty(); });

			// Break the loop if stopping and no more tasks
			if (pool->stop_flag && pool->tasks.empty()) {
				return nullptr;
			}

			// Get the next task
			task = std::move(pool->tasks.front());
			pool->tasks.pop();
		}

		task();	 // Execute the task
	}
	return nullptr;
}

// Method for processing callbacks in the main thread
template <typename Qtask>
void qthreadpool<Qtask>::process_in_main_thread() {
	while (true) {
		std::function<void()> callback;
		{
			std::unique_lock<std::mutex> lock(main_thread_mutex);
			// Check if there are callbacks to process
			if (main_thread_queue.empty()) {
				break;	// No more callbacks to process
			}

			// Retrieve the callback
			callback = std::move(main_thread_queue.front());
			main_thread_queue.pop();
		}

		// Execute the callback
		if (callback) {
			callback();	 // This is executed in the main thread
		}
	}
}

#endif	// QTHREADPOOL_HPP

// ------------------------ EXAMPLE CODE ------------------------
// ------------------------ EXAMPLE CODE ------------------------
// ------------------------ EXAMPLE CODE ------------------------

/*
// in .H header file
qthreadpool<std::function<void()>> threadpool {4};

// in .CPP source file
 // threadpool.process_in_main_thread() has to be called periodically by any timer on regular inteval so that
 // the second callback get called on main thread. If you dont want the main thread callback you can ignore the second callback
threadpool.enqueue(
	[] {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "worker thread");
	},
	[](int id) {
		debug_print(LOG_LEVEL_0, __LOGTAG__, "Task %d worker finished. called in main thread", id);
	},
	1   // taask id
);
*/
