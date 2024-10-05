#ifndef QTHREADPOOL_H
#define QTHREADPOOL_H

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <pthread.h>
#include <queue>
#include <unordered_map>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "qthreadpool"

template <typename user_context_t>
class qthreadpool {
   public:
	using task_t = std::function<void(user_context_t&, int)>;
	using task_completion_callback_t = std::optional<std::function<void(int)>>;

	struct qthread_context {
		user_context_t context;	 // The user-defined context for each thread
		bool initialized;		 // Flag indicating if the thread was initialized successfully
	};

	qthreadpool(uint8_t num_threads, std::function<bool(user_context_t&, const void*)> init_callback, std::function<void(user_context_t&)> shutdown_callback)
		: num_threads(num_threads), init_callback(init_callback), shutdown_callback(shutdown_callback), stop_flag(false), initialized_threads(0), failed_threads(0) {}

	~qthreadpool() { stop(); }

	// Initialize the thread pool (create threads) synchronously with an additional argument for init_callback
	int16_t init(const void* init_arg) {
		pthread_mutex_lock(&init_mutex);
		if (initialized_threads > 0 || failed_threads > 0) {
			debug_print_warn(__LOGTAG__, "threadpool already inited, ignoring.");
			pthread_mutex_unlock(&init_mutex);
			return initialized_threads;
		}

		threads.resize(num_threads);
		initialized_threads = 0;  // Reset the initialized count
		failed_threads = 0;		  // Reset the failed threads count

		// Pass the same init_arg to each thread during creation
		this->init_arg = init_arg;	// Store a pointer to the initialization argument

		for (uint8_t i = 0; i < num_threads; ++i) {
			if (pthread_create(&threads[i], nullptr, &qthreadpool::thread_func, this) != 0) {
				debug_print_error(__LOGTAG__, "Failed to create threadpool thread");

				// Thread creation failed, return false
				pthread_mutex_unlock(&init_mutex);
				return -1;
			}
		}

		// Wait until all threads have completed their initialization
		while (initialized_threads + failed_threads < num_threads) {
			pthread_cond_wait(&all_threads_initialized_cond, &init_mutex);
		}

		pthread_mutex_unlock(&init_mutex);
		debug_print_important(__LOGTAG__, "inited with %d thread (failed %d). total %d", initialized_threads, failed_threads, num_threads);
		// Return false if any thread failed to initialize
		return initialized_threads;
	}

	// Enqueue a task with an optional completion callback
	void enqueue(task_t task, int user_id, task_completion_callback_t callback = std::nullopt) {
		pthread_mutex_lock(&queue_mutex);
		if (initialized_threads == 0) {
			pthread_mutex_unlock(&queue_mutex);
			return;	 // If no threads initialized, return
		}
		tasks.push({task, user_id, callback});		// Store both task, user ID, and optional callback
		pthread_cond_signal(&task_available_cond);	// Signal that a new task is available
		pthread_mutex_unlock(&queue_mutex);
	}

	// Function to stop the thread pool gracefully
	void stop() {
		pthread_mutex_lock(&queue_mutex);
		stop_flag = true;
		pthread_cond_broadcast(&task_available_cond);  // Wake up all worker threads
		pthread_mutex_unlock(&queue_mutex);

		for (auto& thread : threads) {
			pthread_join(thread, nullptr);	// Join each worker thread
		}

		// Clean up mutexes and condition variables
		pthread_mutex_destroy(&queue_mutex);
		pthread_mutex_destroy(&init_mutex);
		pthread_cond_destroy(&task_available_cond);
		pthread_cond_destroy(&all_threads_initialized_cond);
	}

	// Get the number of pending tasks in the queue
	size_t get_pending_task_count() const {
		pthread_mutex_lock(&queue_mutex);
		size_t task_count = tasks.size();  // Get the size of the task queue
		pthread_mutex_unlock(&queue_mutex);
		return task_count;
	}

	// Getter for initialized_threads
	uint16_t get_initialized_threads() const {
		pthread_mutex_lock(&init_mutex);
		uint16_t count = initialized_threads;  // Get the count of initialized threads
		pthread_mutex_unlock(&init_mutex);
		return count;
	}

	// Process completed tasks in the main thread and call any registered callbacks
	void process_in_main_thread() {
		pthread_mutex_lock(&completed_mutex);
		for (auto& completed_task : completed_tasks) {
			if (completed_task.callback) {
				// Call the callback with the task ID if it's set
				(*completed_task.callback)(completed_task.user_id);
			}
		}
		// Clear completed tasks after processing
		completed_tasks.clear();
		pthread_mutex_unlock(&completed_mutex);
	}

   private:
	struct task_data_t {
		task_t task;
		int user_id;						  // Renamed from task_index to user_id
		task_completion_callback_t callback;  // Optional callback
	};

	uint8_t num_threads;
	std::vector<pthread_t> threads;
	std::queue<task_data_t> tasks;
	std::vector<task_data_t> completed_tasks;				  // Store completed tasks for main thread processing
	std::unordered_map<pthread_t, qthread_context> contexts;  // Map thread_id to context

	pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t task_available_cond = PTHREAD_COND_INITIALIZER;

	std::function<bool(user_context_t&, const void*)> init_callback;  // Now returns bool
	std::function<void(user_context_t&)> shutdown_callback;
	bool stop_flag;

	// Synchronization for thread initialization
	pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t all_threads_initialized_cond = PTHREAD_COND_INITIALIZER;
	uint16_t initialized_threads;
	uint16_t failed_threads;

	// Add a new mutex for handling the completed_tasks list
	pthread_mutex_t completed_mutex = PTHREAD_MUTEX_INITIALIZER;

	const void* init_arg;  // Store a pointer to the initialization argument

	static void* thread_func(void* arg) {
		PTHREAD_NAME("qthreadpool");
		qthreadpool* pool = static_cast<qthreadpool*>(arg);
		pthread_t thread_id = pthread_self();				   // Get the current thread ID
		qthread_context& context = pool->contexts[thread_id];  // Use specific thread context

		// Initialize context in worker thread with the common init_arg
		context.initialized = pool->init_callback(context.context, pool->init_arg);

		// Signal that the thread has finished its initialization
		pthread_mutex_lock(&pool->init_mutex);
		if (context.initialized) {
			pool->initialized_threads++;
			debug_print(LOG_LEVEL_0, __LOGTAG__, "initialised threadpool context. count %d", pool->initialized_threads);
		} else {
			pool->failed_threads++;
			debug_print_error(__LOGTAG__, "failed to initialise threadpool context");
		}

		if (pool->initialized_threads + pool->failed_threads == pool->num_threads) {
			pthread_cond_signal(&pool->all_threads_initialized_cond);
		}
		pthread_mutex_unlock(&pool->init_mutex);

		// Exit if initialization failed
		if (!context.initialized) {
			// Shutdown context in worker thread
			pool->shutdown_callback(context.context);  // Call shutdown_callback with the user_context
			debug_print_important(__LOGTAG__, "cleanup threadpool context due to failed initialisation");
			return nullptr;
		}

		while (true) {
			task_data_t task_data;

			pthread_mutex_lock(&pool->queue_mutex);
			while (!pool->stop_flag && pool->tasks.empty()) {
				pthread_cond_wait(&pool->task_available_cond, &pool->queue_mutex);
			}

			if (pool->stop_flag && pool->tasks.empty()) {
				pthread_mutex_unlock(&pool->queue_mutex);
				break;
			}

			task_data = std::move(pool->tasks.front());
			pool->tasks.pop();
			pthread_mutex_unlock(&pool->queue_mutex);

			// Execute the task with thread-specific context and user ID
			task_data.task(context.context, task_data.user_id);

			// Add completed task to completed_tasks list for main thread processing
			pthread_mutex_lock(&pool->completed_mutex);
			pool->completed_tasks.push_back(std::move(task_data));
			pthread_mutex_unlock(&pool->completed_mutex);
		}

		// Shutdown context in worker thread
		pool->shutdown_callback(context.context);  // Call shutdown_callback with the user_context
		debug_print(LOG_LEVEL_0, __LOGTAG__, "cleanup threadpool context");
		return nullptr;
	}
};

#endif	// QTHREADPOOL_H

// Example Usage
/*
 threadpool.enqueue(
	 [](thread_pool_context& ctx, int user_id) {    // <-- Callback from worker thread
		 pthread_t thread_id = pthread_self();  // Get the current thread ID
		 debug_print(LOG_LEVEL_0, __LOGTAG__, "worker thread for task %d using context data - thread:%d", user_id, thread_id);
	 },
	 1, <-- user defined id
	 [](int user_id) {  <-- Callback in mainthread to notify task completion. (Optional)
		 pthread_t thread_id = pthread_self();  // Get the current thread ID
		 debug_print(LOG_LEVEL_0, __LOGTAG__, "task %d finished - thread:%d", user_id, thread_id);
	 });
 */
