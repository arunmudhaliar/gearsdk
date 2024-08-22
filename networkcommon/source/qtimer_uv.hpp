#ifndef qtimer_uv_hpp
#define qtimer_uv_hpp

#include "../../common/sdktypes.hpp"
#include <uv.h>
#include <functional>
#include <vector>

#undef __LOGTAG__
#define __LOGTAG__ "qtimer_uv"

class qtimer_uv;
typedef std::function<void(qtimer_uv& qtimer_uv_)> type_qtimer_uv_cb;

class qtimer_uv {
private:
    qtimer_uv() {}

public:
    static void evtimer_cb(uv_timer_t* handle) {
        qtimer_uv* qtimer_uv_ = (qtimer_uv*) handle->data;

        // Repeat timer case
        if (qtimer_uv_->count == -1) {
           qtimer_uv_->timeout_callback(*qtimer_uv_);
            return;
        }

        // Other timers
        qtimer_uv_->count--;
        if (qtimer_uv_->count <= 0) {
            uv_timer_stop(handle);
            qtimer_uv_->finished = true;
        }
        qtimer_uv_->timeout_callback(*qtimer_uv_);
    }

    qtimer_uv(uv_loop_t* loop, type_qtimer_uv_cb timeout_callback, void* data, float delay = 1.0f, float count = 1)
        : loop(loop), timeout_callback(timeout_callback), count(count), delay(delay), data(data) {
        uv_timer_init(loop, &timer);
        timer.data = this;
        uv_timer_start(&timer, evtimer_cb, delay * 1000, delay * 1000);  // Delay in milliseconds
    }

    virtual ~qtimer_uv() {
        DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "qtimer_uv destructor");
    }

    void update_delay(float new_delay) {
        uv_timer_stop(&timer);
        uv_timer_start(&timer, evtimer_cb, new_delay * 1000, new_delay * 1000);
        delay = new_delay;
    }

    uv_timer_t timer;
    uv_loop_t* loop;
    const type_qtimer_uv_cb timeout_callback;
    int count;
    bool finished = false;
    float delay = 1.0f;
    void* data = nullptr;
};

class qtimer_uv_scheduler {
public:
    qtimer_uv_scheduler();
    virtual ~qtimer_uv_scheduler();

    void set_uv_loop(uv_loop_t* loop) { this->loop = loop; }
    qtimer_uv* schedule_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data = nullptr);
    qtimer_uv* schedule_count_timer(type_qtimer_uv_cb timeout_callback, float delay, int count, void* data = nullptr);
    qtimer_uv* schedule_repeat_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data = nullptr);
    void cancel_timer(qtimer_uv* qtimer_uv_);
    bool cancel_and_destroy_timer(qtimer_uv* qtimer_uv_);
    void shutdown_mainloop();  // Careful
    bool is_timer_present_in_list(qtimer_uv* qtimer_uv_);

private:
	struct qtimer_uv_scheduler_data {
		qtimer_uv_scheduler_data(void* data, void* scheduler, const type_qtimer_uv_cb timeout_callback) : data(data), scheduler(scheduler), timeout_callback(timeout_callback) {}
		qtimer_uv_scheduler_data(const qtimer_uv_scheduler_data& qtimer_uvschedulerdata) : timeout_callback(qtimer_uvschedulerdata.timeout_callback) {
			data = qtimer_uvschedulerdata.data;
			scheduler = qtimer_uvschedulerdata.scheduler;
		}
		void* data = nullptr;
		void* scheduler = nullptr;
		const type_qtimer_uv_cb timeout_callback;
	};
    bool destroy_timer(qtimer_uv* qtimer_uv_);
    static void evtimer_scheduler_cb(qtimer_uv& qtimer_uv_);
    void destroy_all();

    uv_loop_t* loop;
    std::vector<qtimer_uv*> timers;
};

#endif /* qtimer_uv_hpp */
