.. _qtimer_hpp:

qtimer.hpp and qtimer.cpp
==========================

The `qtimer` and `qtimer_scheduler` classes are designed for managing and scheduling timers using the `libev` event loop. The `qtimer` class represents a single timer, while the `qtimer_scheduler` class handles the scheduling and management of multiple timers.

**Header File: `qtimer.hpp`**

The header file defines two classes: `qtimer` and `qtimer_scheduler`. 

**Class: `qtimer`**

The `qtimer` class provides functionalities to create and manage timers. Key components include:

- **Constructor and Destructor**
  - **`qtimer(struct ev_loop* loop, type_qtimer_cb timeout_callback, void* data, float delay = 1.0f, float count = 1)`**: Initializes a new timer with the specified delay and count. The timer uses the provided callback function.
  - **`virtual ~qtimer()`**: Destructor for cleaning up resources.

- **Methods**
  - **`void update_delay(float new_delay)`**: Updates the timer's delay and restarts it with the new delay.
  - **`ev_timer timer`**: The underlying `libev` timer structure.
  - **`struct ev_loop* loop`**: Pointer to the event loop.
  - **`const type_qtimer_cb timeout_callback`**: Callback function invoked when the timer expires.
  - **`int count`**: Number of times the timer should fire before stopping (use `-1` for infinite repeat).
  - **`bool finished`**: Indicates whether the timer has completed its execution.
  - **`float delay`**: Time interval between timer events.
  - **`void* data`**: User-defined data passed to the callback.

**Class: `qtimer_scheduler`**

The `qtimer_scheduler` class manages a collection of `qtimer` instances. Key components include:

- **Constructor and Destructor**
  - **`qtimer_scheduler()`**: Initializes the scheduler.
  - **`virtual ~qtimer_scheduler()`**: Cleans up all managed timers.

- **Methods**
  - **`void set_loop(struct ev_loop* loop)`**: Sets the event loop for the scheduler.
  - **`qtimer* schedule_timer(type_qtimer_cb timeout_callback, float delay, void* data = nullptr)`**: Schedules a one-time timer.
  - **`qtimer* schedule_count_timer(type_qtimer_cb timeout_callback, float delay, int count, void* data = nullptr)`**: Schedules a timer that fires a specified number of times.
  - **`qtimer* schedule_repeat_timer(type_qtimer_cb timeout_callback, float delay, void* data = nullptr)`**: Schedules a repeating timer.
  - **`void cancel_timer(qtimer* qtimer_)`**: Cancels a running timer without destroying it.
  - **`bool cancel_and_destroy_timer(qtimer* qtimer_)`**: Cancels and destroys a timer.
  - **`bool is_timer_present_in_list(qtimer* qtimer_)`**: Checks if a timer is present in the scheduler's list.
  - **`void shutdown_mainloop()`**: Breaks out of the event loop.

**Example Usage**

Here's a simple example demonstrating how to use the `qtimer` and `qtimer_scheduler` classes:

.. code-block:: cpp

    #include "qtimer.hpp"
    #include <ev.h>

    void example_callback(qtimer& timer) {
        debug_print(LOG_LEVEL_0, __LOGTAG__, "Timer expired");
    }

    int main() {
        struct ev_loop* loop = ev_default_loop(0);
        qtimer_scheduler scheduler;
        scheduler.set_loop(loop);

        // Schedule a one-time timer
        scheduler.schedule_timer([](qtimer& timer) {
            debug_print(LOG_LEVEL_0, __LOGTAG__, "One-time timer expired");
        }, 5);

        // Schedule a repeating timer
        scheduler.schedule_repeat_timer([](qtimer& timer) {
            debug_print(LOG_LEVEL_0, __LOGTAG__, "Repeating timer expired");
        }, 5);

        // Schedule a count timer
        scheduler.schedule_count_timer([](qtimer& timer) {
            debug_print(LOG_LEVEL_0, __LOGTAG__, "Count timer expired");
        }, 5, 3);

        ev_run(loop, 0);
        return 0;
    }

This example demonstrates the creation of a `qtimer_scheduler`, setting an event loop, and scheduling various types of timers.

