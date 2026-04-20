/*
 * ping.h
 *
 *  Created on: Apr 2, 2026
 *      Author: minhquan
 */
#include "Timer.h"
#include "stdint.h"
#include "stdio.h"
#ifndef PING_H_
#define PING_H_
typedef enum{
    LOW,
    HIGH,
    DONE
}state_t;
extern volatile uint32_t g_start_time;
extern volatile uint32_t g_end_time;
extern volatile state_t g_state ;
void ping_trigger (void);
void TIMER3B_Handler(void);
float ping_getDistance (void);
void ping_init();
void init_portB_Timer();
void init_timer();
void init_portB_output();
#endif /* PING_H_ */
