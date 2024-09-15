#ifndef qtimer_uv_hpp
#define qtimer_uv_hpp

#include "../../common/sdktypes.hpp"

#include <functional>
#include <uv.h>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "qtimer_uv"

class qtimer_uv;
typedef std::function<void(qtimer_uv& qtimer_uv)> type_qtimer_uv_cb;

class qtimer_uv {
   private:
	qtimer_uv() {}

   public:
	static void uvtimer_cb(uv_timer_t* handle) {
		qtimer_uv* timer_uv = (qtimer_uv*) handle->data;

		// Repeat timer case
		if (timer_uv->count == -1) {
			timer_uv->TIMEOUT_CALLBACK(*timer_uv);
			return;
		}

		// Other timers
		timer_uv->count--;
		if (timer_uv->count <= 0) {
			uv_timer_stop(handle);
			timer_uv->finished = true;
		}
		timer_uv->TIMEOUT_CALLBACK(*timer_uv);
	}

	qtimer_uv(uv_loop_t* loop, type_qtimer_uv_cb timeout_callback, void* data, float delay = 1.0f, float count = 1) : loop(loop), TIMEOUT_CALLBACK(timeout_callback), count(count), delay(delay), data(data) {
		uv_timer_init(loop, &timer);
		timer.data = this;
		uv_timer_start(&timer, uvtimer_cb, delay * 1000, delay * 1000);	 // Delay in milliseconds
	}

	virtual ~qtimer_uv() {
		if (!uv_is_closing((uv_handle_t*) &timer)) {
			uv_timer_stop(&timer);	// in-case if its not closed.
			uv_close((uv_handle_t*) &timer, nullptr);
			// Run the loop again to process the cleanup
			uv_run(loop, UV_RUN_ONCE);
		}
		debug_print(LOG_LEVEL_4, __LOGTAG__, "qtimer_uv destructor");
	}

	void update_delay(float new_delay) {
		uv_timer_stop(&timer);
		uv_timer_start(&timer, uvtimer_cb, new_delay * 1000, new_delay * 1000);
		delay = new_delay;
	}

	uv_timer_t timer;
	uv_loop_t* loop;
	const type_qtimer_uv_cb TIMEOUT_CALLBACK;
	int count;
	bool finished = false;
	float delay = 1.0f;
	void* data = nullptr;
};

typedef std::function<void(bool timer_deleted)> type_async_qtimer_uv_cancel_finalize_cb;

class qtimer_uv_scheduler {
   public:
	qtimer_uv_scheduler();
	virtual ~qtimer_uv_scheduler();

	void set_loop(uv_loop_t* loop) { this->loop = loop; }
	qtimer_uv* schedule_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data = nullptr);
	qtimer_uv* schedule_count_timer(type_qtimer_uv_cb timeout_callback, float delay, int count, void* data = nullptr);
	qtimer_uv* schedule_repeat_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data = nullptr);
	void cancel_timer(qtimer_uv* timer_uv);
	bool cancel_and_destroy_timer(qtimer_uv* timer_uv);
	void async_cancel_and_destroy_timer(qtimer_uv* timer_uv, const type_async_qtimer_uv_cancel_finalize_cb cb);
	bool is_timer_present_in_list(qtimer_uv* timer_uv);

   private:
	struct async_qtimer_uv_cancel_tuple {
		async_qtimer_uv_cancel_tuple(qtimer_uv_scheduler* scheduler, qtimer_uv* timer, const type_async_qtimer_uv_cancel_finalize_cb cb) : scheduler(scheduler), timer(timer), cb(cb) {}
		qtimer_uv_scheduler* scheduler = nullptr;
		qtimer_uv* timer = nullptr;
		const type_async_qtimer_uv_cancel_finalize_cb cb = nullptr;
	};
	struct qtimer_uv_scheduler_data {
		qtimer_uv_scheduler_data(void* data, void* scheduler, const type_qtimer_uv_cb TIMEOUT_CALLBACK) : data(data), scheduler(scheduler), TIMEOUT_CALLBACK(TIMEOUT_CALLBACK) {}
		qtimer_uv_scheduler_data(const qtimer_uv_scheduler_data& qtimer_uvschedulerdata) : TIMEOUT_CALLBACK(qtimer_uvschedulerdata.TIMEOUT_CALLBACK) {
			data = qtimer_uvschedulerdata.data;
			scheduler = qtimer_uvschedulerdata.scheduler;
		}
		void* data = nullptr;
		void* scheduler = nullptr;
		const type_qtimer_uv_cb TIMEOUT_CALLBACK;
	};
	bool destroy_timer(qtimer_uv* timer_uv);
	static void uvtimer_scheduler_cb(qtimer_uv& timer_uv);
	void destroy_all();
	static void stop_timer_async_cb(uv_async_t* handle);

	uv_loop_t* loop;
	std::vector<qtimer_uv*> timers;
};

#endif /* qtimer_uv_hpp */
