/*
 * adc.h
 *
 *  Created on: Mar 26, 2026
 *      Author: minhquan
 */
#include <inc/tm4c123gh6pm.h>
#ifndef ADC_H_
#define ADC_H_

void adc_init(void);
int adc_read(void);
float average_ir(void);
double ir_to_cm(float y);

#endif /* ADC_H_ */
