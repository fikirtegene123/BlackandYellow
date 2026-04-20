/*
 * servo.h
 *
 *  Created on: Apr 9, 2026
 *      Author: minhquan
 */
#ifndef SERVO_H_
#define SERVO_H_
#include "stdint.h"
#include "stdio.h"

void servo_init_new(void);
void servo_move_new(uint32_t degrees);
void init_pwm(void);
float extract_slope(void);
float extract_b(float slope);
void calibrate_servo(uint32_t calibrate_val);
void init_timer_pwm(void);
#endif /* SERVO_H_ */
