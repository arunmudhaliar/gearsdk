#include "qtimer_uv.hpp"
#include <algorithm>

qtimer_uv_scheduler::qtimer_uv_scheduler() {}

qtimer_uv_scheduler::~qtimer_uv_scheduler() {
    destroy_all();
}

void qtimer_uv_scheduler::destroy_all() {
    for (auto it = timers.cbegin(); it != timers.cend(); it++) {
        qtimer_uv* qtimer_uv_ = *it;
        qtimer_uv_->finished = true;
        uv_timer_stop(&qtimer_uv_->timer);
        qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) qtimer_uv_->data;
        GX_DELETE(data);
        GX_DELETE(qtimer_uv_);
    }
}

void qtimer_uv_scheduler::evtimer_scheduler_cb(qtimer_uv& qtimer_uv_) {
    qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) qtimer_uv_.data;
    data->timeout_callback(qtimer_uv_);
    if (qtimer_uv_.finished) {
        qtimer_uv_scheduler* scheduler = (qtimer_uv_scheduler*) data->scheduler;
        scheduler->destroy_timer(&qtimer_uv_);
    }
}

bool qtimer_uv_scheduler::destroy_timer(qtimer_uv* qtimer_uv_) {
    if (!qtimer_uv_->finished) {
        return false;
    }

    size_t oldSz = timers.size();
    timers.erase(std::remove(timers.begin(), timers.end(), qtimer_uv_), timers.end());
    if (oldSz != timers.size()) {
        qtimer_uv_scheduler_data* data = (qtimer_uv_scheduler_data*) qtimer_uv_->data;
        GX_DELETE(data);
        GX_DELETE(qtimer_uv_);
        return true;
    }
    return false;
}

void qtimer_uv_scheduler::cancel_timer(qtimer_uv* qtimer_uv_) {
    if (!is_timer_present_in_list(qtimer_uv_)) {
        return;
    }
    qtimer_uv_->finished = true;
    uv_timer_stop(&qtimer_uv_->timer);
}

bool qtimer_uv_scheduler::cancel_and_destroy_timer(qtimer_uv* qtimer_uv_) {
    if (!is_timer_present_in_list(qtimer_uv_)) {
        return false;
    }
    qtimer_uv_->finished = true;
    uv_timer_stop(&qtimer_uv_->timer);
    return destroy_timer(qtimer_uv_);
}

bool qtimer_uv_scheduler::is_timer_present_in_list(qtimer_uv* qtimer_uv_) {
    if (qtimer_uv_ == nullptr) return false;
    auto result = std::find(timers.begin(), timers.end(), qtimer_uv_);
    return (result != timers.end());
}

qtimer_uv* qtimer_uv_scheduler::schedule_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data) {
    qtimer_uv* qtimer_uv_ = DEBUG_NEW qtimer_uv(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay);
    timers.push_back(qtimer_uv_);
    return qtimer_uv_;
}

qtimer_uv* qtimer_uv_scheduler::schedule_count_timer(type_qtimer_uv_cb timeout_callback, float delay, int count, void* data) {
    qtimer_uv* qtimer_uv_ = DEBUG_NEW qtimer_uv(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay, count);
    timers.push_back(qtimer_uv_);
    return qtimer_uv_;
}

qtimer_uv* qtimer_uv_scheduler::schedule_repeat_timer(type_qtimer_uv_cb timeout_callback, float delay, void* data) {
    qtimer_uv* qtimer_uv_ = DEBUG_NEW qtimer_uv(loop, evtimer_scheduler_cb, DEBUG_NEW qtimer_uv_scheduler_data(data, this, timeout_callback), delay, -1);
    timers.push_back(qtimer_uv_);
    return qtimer_uv_;
}

void qtimer_uv_scheduler::shutdown_mainloop() {
    uv_stop(loop);
}
