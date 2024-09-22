#pragma once
#include "sdktypes.hpp"

/**
 * @class timer
 * @brief Manages the timing and frame rate for the game.
 *
 * This class provides functions to initialize, update, and reset the timer,
 * as well as to retrieve various time-related values such as delta time,
 * frame rate, and elapsed time.
 * @author Arun A
 * @copyright Copyright (c) 2024 homenet25
 */

class timer {
   public:
	/**
	 * @brief Initializes the timer.
	 *
	 * This function should be called when the game initializes.
	 */
	static void init();

	/**
	 * @brief Updates the timer variables for each frame.
	 */
	static void update();

	/**
	 * @brief Updates the timer variables for each frame with a target FPS.
	 *
	 * @param target_fps The target frames per second.
	 */
	static void update(double target_fps);

	/**
	 * @brief Resets the timer variables.
	 */
	static void reset();

	/**
	 * @brief Gets the delta time in seconds.
	 *
	 * @return The delta time in seconds.
	 */
	static double get_dt_in_sec() { return timer_dt_in_sec; }

	/**
	 * @brief Gets the delta time in milliseconds.
	 *
	 * @return The delta time in milliseconds.
	 */
	static int get_dt_in_milli_sec() { return timer_dt_in_milli_sec; }

	/**
	 * @brief Gets the current frames per second.
	 *
	 * @return The current frames per second.
	 */
	static double get_fps() { return timer_fps; }

	/**
	 * @brief Gets the elapsed time in seconds since the timer was initialized.
	 *
	 * @return The elapsed time in seconds.
	 */
	static double get_elapsed_time() { return timer_elapsed_time_in_sec; }

	/**
	 * @brief Gets the current time in seconds.
	 *
	 * @return The current time in seconds.
	 */
	static double get_current_time_in_sec();

	/**
	 * @brief Gets the current time in milliseconds.
	 *
	 * @return The current time in milliseconds.
	 */
	static unsigned long get_current_time_in_milli_sec();

	/**
	 * @brief Sets the global time scale for the engine.
	 *
	 * @param timescale The new time scale value.
	 */
	static void set_time_scale(double timescale) { timer_time_scale = timescale; }

	/**
	 * @brief Gets the current global time scale.
	 *
	 * @return The current time scale value.
	 */
	static double get_time_scale() { return timer_time_scale; }

   private:
	static double timer_fps;				  ///< The current frames per second.
	static double timer_dt_in_sec;			  ///< The delta time in seconds.
	static int timer_dt_in_milli_sec;		  ///< The delta time in milliseconds.
	static double timer_elapsed_time_in_sec;  ///< The elapsed time in seconds since the timer was initialized.
	static double timer_previous_time;		  ///< The previous time, used for internal calculations.
	static double timer_averaging_time;		  ///< The time over which to average the frame rate.
	static double timer_last_time;			  ///< The last recorded time, used for internal calculations.
	static int timer_frame_count;			  ///< The frame count, used for calculating the frame rate.
	static double timer_time_scale;			  ///< The global time scale for the engine.
	static constexpr double MIN_FRAME_TIME_SEC = 0.03;
};
