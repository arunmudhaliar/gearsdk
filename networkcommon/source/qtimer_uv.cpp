#include "qtimer_uv.hpp"

#include <algorithm>

qtimer_uv_scheduler::qtimer_uv_scheduler() {}

qtimer_uv_scheduler::~qtimer_uv_scheduler() {
	destroy_all();
}

void qtimer_uv_scheduler::destroy_all() {
	for (auto it = timers.cbegin(); it != timers.cend(); it++) {
		qtimer_uv* timer_uv = *it;
		timer_uv->finished = true;
		uv_timer_stop(&timer_uv->timer);
		qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) timer_uv->data;
		GX_DELETE(data);
		GX_DELETE(timer_uv);
	}
}

void qtimer_uv_scheduler::uvtimer_scheduler_cb(qtimer_uv& timer_uv) {
	qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) timer_uv.data;
	data->TIMEOUT_CALLBACK(timer_uv);
	if (timer_uv.finished) {
		qtimer_uv_scheduler* scheduler = (qtimer_uv_scheduler*) data->scheduler;
		scheduler->destroy_timer(&timer_uv);
	}
}

bool qtimer_uv_scheduler::destroy_timer(qtimer_uv* timer_uv) {
	if (!timer_uv->finished) {
		return false;
	}

	size_t old_sz = timers.size();
	timers.erase(std::remove(timers.begin(), timers.end(), timer_uv), timers.end());
	if (old_sz != timers.size()) {
		qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) timer_uv->data;
		GX_DELETE(data);
		GX_DELETE(timer_uv);
		return true;
	}
	return false;
}

void qtimer_uv_scheduler::cancel_timer(qtimer_uv* timer_uv) {
	if (!is_timer_present_in_list(timer_uv)) {
		return;
	}
	timer_uv->finished = true;
	uv_timer_stop(&timer_uv->timer);
}

bool qtimer_uv_scheduler::cancel_and_destroy_timer(qtimer_uv* timer_uv) {
	if (!is_timer_present_in_list(timer_uv)) {
		return false;
	}
	timer_uv->finished = true;
	uv_timer_stop(&timer_uv->timer);
	return destroy_timer(timer_uv);
}

void qtimer_uv_scheduler::async_cancel_and_destroy_timer(qtimer_uv* timer_uv, const type_async_qtimer_uv_cancel_finalize_cb CB) {
	if (timer_uv == nullptr) {
		debug_print_error(__LOGTAG__, "failed async_cancel_and_destroy_timer, timer_uv == nulll !!!");
		return;
	}
	if (CB == nullptr) {
		debug_print_error(__LOGTAG__, "failed async_cancel_and_destroy_timer, cb == nulll !!!");
		return;
	}
	uv_async_t* async_stop = DEBUG_NEW uv_async_t;
	async_qtimer_uv_cancel_tuple* tuple = DEBUG_NEW async_qtimer_uv_cancel_tuple(this, timer_uv, CB);
	async_stop->data = tuple;
	uv_async_init(loop, async_stop, stop_timer_async_cb);
	uv_async_send(async_stop);
}

void qtimer_uv_scheduler::stop_timer_async_cb(uv_async_t* handle) {
	async_qtimer_uv_cancel_tuple* tuple = reinterpret_cast<async_qtimer_uv_cancel_tuple*>(handle->data);
	qtimer_uv* timer = tuple->timer;
	qtimer_uv_scheduler* scheduler = tuple->scheduler;
	if (!scheduler->cancel_and_destroy_timer(timer)) {
		debug_print_error(__LOGTAG__, "cancel_and_destroy_timer returned false !!!");
		tuple->CB(false);
	}
	tuple->CB(true);
	GX_DELETE(tuple);
	uv_close((uv_handle_t*) handle, nullptr);
	// Run the loop again to process the cleanup
	uv_run(scheduler->loop, UV_RUN_ONCE);
	GX_DELETE(handle);
}

bool qtimer_uv_scheduler::is_timer_present_in_list(qtimer_uv* timer_uv) {
	if (timer_uv == nullptr)
		return false;
	auto result = std::find(timers.begin(), timers.end(), timer_uv);
	return (result != timers.end());
}

qtimer_uv* qtimer_uv_scheduler::schedule_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data) {
	qtimer_uv* timer_uv = DEBUG_NEW qtimer_uv(loop, uvtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay);
	timers.push_back(timer_uv);
	return timer_uv;
}

qtimer_uv* qtimer_uv_scheduler::schedule_count_timer(type_qtimer_uv_cb timeout_callback, float delay, int count, void* data) {
	qtimer_uv* timer_uv = DEBUG_NEW qtimer_uv(loop, uvtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay, count);
	timers.push_back(timer_uv);
	return timer_uv;
}

qtimer_uv* qtimer_uv_scheduler::schedule_repeat_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data) {
	qtimer_uv* timer_uv = DEBUG_NEW qtimer_uv(loop, uvtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay, -1);
	timers.push_back(timer_uv);
	return timer_uv;
}
