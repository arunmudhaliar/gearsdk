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
     * @param targetFPS The target frames per second.
     */
    static void update(float targetFPS);

    /**
     * @brief Resets the timer variables.
     */
    static void reset();

    /**
     * @brief Gets the delta time in seconds.
     *
     * @return The delta time in seconds.
     */
    static float getDtinSec() { return timerDTInSec; }

    /**
     * @brief Gets the delta time in milliseconds.
     *
     * @return The delta time in milliseconds.
     */
    static int getDTinMilliSec() { return timerDTInMilliSec; }

    /**
     * @brief Gets the current frames per second.
     *
     * @return The current frames per second.
     */
    static float getFPS() { return timerFPS; }

    /**
     * @brief Gets the elapsed time in seconds since the timer was initialized.
     *
     * @return The elapsed time in seconds.
     */
    static float getElapsedTime() { return timerElapsedTimeInSec; }

    /**
     * @brief Gets the current time in seconds.
     *
     * @return The current time in seconds.
     */
    static double getCurrentTimeInSec();

    /**
     * @brief Gets the current time in milliseconds.
     *
     * @return The current time in milliseconds.
     */
    static unsigned long getCurrentTimeInMilliSec();

    /**
     * @brief Sets the global time scale for the engine.
     *
     * @param timescale The new time scale value.
     */
    static void setTimeScale(float timescale) { timerTimeScale = timescale; }

    /**
     * @brief Gets the current global time scale.
     *
     * @return The current time scale value.
     */
    static float getTimeScale() { return timerTimeScale; }

private:
    static float    timerFPS;               ///< The current frames per second.
    static float    timerDTInSec;            ///< The delta time in seconds.
    static int      timerDTInMilliSec;        ///< The delta time in milliseconds.
    static float    timerElapsedTimeInSec;    ///< The elapsed time in seconds since the timer was initialized.
    static double   timerPreviousTime;      ///< The previous time, used for internal calculations.
    static float    timerAveragingTime;     ///< The time over which to average the frame rate.
    static double   timerLastTime;          ///< The last recorded time, used for internal calculations.
    static int      timerFrameCount;        ///< The frame count, used for calculating the frame rate.
    static float    timerTimeScale;         ///< The global time scale for the engine.
};
