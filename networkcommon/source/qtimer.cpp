//
//  Copyright 2024 homenet25
//  qtimer.cpp
//  networkcommon
//
//  Created by Arun A on 27/10/23.
//

#include "qtimer.hpp"
#include <algorithm>

qtimer_sceduler::qtimer_sceduler() {}

qtimer_sceduler::~qtimer_sceduler() {
	destroy_all();
}
void qtimer_sceduler::destroy_all() {
	for (auto it = timers.cbegin(); it != timers.cend(); it++) {
		qtimer* qtimer_ = *it;
		qtimer_->finished = true;
		ev_timer_stop(qtimer_->loop, &qtimer_->timer);
		qtimer_sceduler_data* data = (qtimer_sceduler_data*) qtimer_->data;
		GX_DELETE(data);
		GX_DELETE(qtimer_);
	}
}

void qtimer_sceduler::evtimer_scheduler_cb(qtimer& qtimer_) {
	qtimer_sceduler_data* data = (qtimer_sceduler_data*) qtimer_.data;
	data->timeout_callback(qtimer_);
	if (qtimer_.finished) {
		qtimer_sceduler* scheduler = (qtimer_sceduler*) data->scheduler;
		scheduler->destroy_timer(&qtimer_);
	}
}

bool qtimer_sceduler::destroy_timer(qtimer* qtimer_) {
	if (!qtimer_->finished) {
		return false;
	}

	size_t oldSz = timers.size();
	timers.erase(std::remove(timers.begin(), timers.end(), qtimer_), timers.end());
	if (oldSz != timers.size()) {
		qtimer_sceduler_data* data = (qtimer_sceduler_data*) qtimer_->data;
		GX_DELETE(data);
		GX_DELETE(qtimer_);
		return true;
	}
	return false;
}

void qtimer_sceduler::cancel_timer(qtimer* qtimer_) {
    if(!is_timer_present_in_list(qtimer_)) {
        return;
    }
	qtimer_->finished = true;
	ev_timer_stop(qtimer_->loop, &qtimer_->timer);
}

bool qtimer_sceduler::cancel_and_destroy_timer(qtimer* qtimer_) {
    if(!is_timer_present_in_list(qtimer_)) {
        return false;
    }
	qtimer_->finished = true;
	ev_timer_stop(qtimer_->loop, &qtimer_->timer);
	return destroy_timer(qtimer_);
}

bool qtimer_sceduler::is_timer_present_in_list(qtimer* qtimer_) {
    if (qtimer_==nullptr) return false;
    auto result = std::find(timers.begin(), timers.end(), qtimer_);
    return (result != timers.end());
}

qtimer* qtimer_sceduler::schedule_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* qtimer_ = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay);
	timers.push_back(qtimer_);
	return qtimer_;
}

qtimer* qtimer_sceduler::schedule_count_timer(type_qtimer_cb timeout_callback, float delay, int count, void* data) {
	qtimer* qtimer_ = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, count);
	timers.push_back(qtimer_);
	return qtimer_;
}

qtimer* qtimer_sceduler::schedule_repeat_timer(type_qtimer_cb timeout_callback, float delay, void* data) {
	qtimer* qtimer_ = DEBUG_NEW qtimer(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_sceduler_data(data, this, timeout_callback), delay, -1);
	timers.push_back(qtimer_);
	return qtimer_;
}

void qtimer_sceduler::shutdown_mainloop() {
	ev_break(EV_A_ EVBREAK_ONE);
}
