#ifndef STEPS_COUNTER_H_INCLUDED
#define STEPS_COUNTER_H_INCLUDED

#include <stdint.h>
#include "esp_err.h"

/*
 * Initialize the library and the accelerometer
 **/
esp_err_t steps_counter_init();

/*
 * Enables the periodic ISR task which executes the algorithm to count steps
 **/
void steps_counter_start_ISR_task();

/*
 * Returns the number of steps.
 * NOTE: this function MAY BLOCK waiting for a mutex periodically locked by an ISR
 **/
int32_t steps_counter_get_steps();

/**
 * Resets the number of steps
 * NOTE: this function MAY BLOCK waiting for a mutex periodically locked by an ISR
 **/
void steps_counter_reset_steps();

#endif /* STEPS_COUNTER_H_INCLUDED */
