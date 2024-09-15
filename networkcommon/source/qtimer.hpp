//
//  Copyright 2024 homenet25
//  evtimer.hpp
//  networkcommon
//
//  Created by Arun A on 27/10/23.
//

#ifndef qtimer_hpp
#define qtimer_hpp

#include "../../common/sdktypes.hpp"

#include <algorithm>
#include <ev.h>
#include <functional>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "qtimer"

// // USAGE
// struct ev_loop* loop = ev_default_loop(0);
// qtimer_scheduler scheduler;
// scheduler.set_loop(loop);
// scheduler.schedule_timer([&scheduler](qtimer& timer){
//     debug_print(LOG_LEVEL_0, __LOGTAG__, "first timer - timeout");
//     qtimer* repeat_timer = scheduler.schedule_repeat_timer([](qtimer& timer){
//         debug_print(LOG_LEVEL_0, __LOGTAG__, "repeat timer - callback");
//     }, 5);
//
//     scheduler.schedule_repeat_timer([&scheduler, repeat_timer](qtimer& timer){
//         debug_print(LOG_LEVEL_0, __LOGTAG__, "third timer - timeout");
//     }, 25);
// }, 5);

class qtimer;
typedef std::function<void(qtimer& timer)> type_qtimer_cb;

class qtimer {
   private:
	qtimer() {}

   public:
	static void evtimer_cb(EV_P_ ev_timer* w, int revents) {
		UNUSED(revents);
		qtimer* timer = (qtimer*) w->data;
		// repeat timer case
		if (timer->count == -1) {
			w->repeat = timer->delay;
			ev_timer_again(loop, w);
			timer->TIMEOUT_CALLBACK(*timer);
			return;
		}
		//

		// other timers
		timer->count--;
		if (timer->count <= 0) {
			ev_timer_stop(loop, &timer->timer);
			timer->finished = true;
		} else {
			w->repeat = timer->delay;
			ev_timer_again(loop, w);
		}
		timer->TIMEOUT_CALLBACK(*timer);
	}

	qtimer(struct ev_loop* loop, type_qtimer_cb timeout_callback, void* data, float delay = 1.0f, float count = 1) : loop(loop), TIMEOUT_CALLBACK(timeout_callback), count(count), delay(delay), data(data) {
		ev_timer_init(&timer, evtimer_cb, delay, 0);
		timer.data = this;
		ev_timer_start(loop, &timer);
	}
	virtual ~qtimer() { debug_print(LOG_LEVEL_4, __LOGTAG__, "qtimer destructor"); }

	void update_delay(float new_delay) {
		// Stop the timer
		ev_timer_stop(loop, &timer);

		// Modify the timer's delay and restart it
		timer.repeat = new_delay;
		ev_timer_again(loop, &timer);
		delay = new_delay;
	}

	ev_timer timer;
	struct ev_loop* loop;
	const type_qtimer_cb TIMEOUT_CALLBACK;
	int count;
	bool finished = false;
	float delay = 1.0f;
	void* data = nullptr;
};

class qtimer_scheduler {
   public:
	qtimer_scheduler();
	virtual ~qtimer_scheduler();

	void set_loop(struct ev_loop* loop) { this->loop = loop; }
	qtimer* schedule_timer(type_qtimer_cb timeout_callback, float delay, void* data = nullptr);
	qtimer* schedule_count_timer(type_qtimer_cb timeout_callback, float delay, int count, void* data = nullptr);
	qtimer* schedule_repeat_timer(type_qtimer_cb timeout_callback, float delay, void* data = nullptr);
	void cancel_timer(qtimer* timer);
	bool cancel_and_destroy_timer(qtimer* timer);
	bool is_timer_present_in_list(qtimer* timer);

   private:
	struct qtimer_sceduler_data {
		qtimer_sceduler_data(void* data, void* scheduler, const type_qtimer_cb TIMEOUT_CALLBACK) : data(data), scheduler(scheduler), TIMEOUT_CALLBACK(TIMEOUT_CALLBACK) {}
		qtimer_sceduler_data(const qtimer_sceduler_data& qtimerschedulerdata) : TIMEOUT_CALLBACK(qtimerschedulerdata.TIMEOUT_CALLBACK) {
			data = qtimerschedulerdata.data;
			scheduler = qtimerschedulerdata.scheduler;
		}
		void* data = nullptr;
		void* scheduler = nullptr;
		const type_qtimer_cb TIMEOUT_CALLBACK;
	};
	bool destroy_timer(qtimer* qtimer);
	static void evtimer_scheduler_cb(qtimer& timer);

	void destroy_all();

	struct ev_loop* loop;
	std::vector<qtimer*> timers;
};

#endif /* qtimer_hpp */
