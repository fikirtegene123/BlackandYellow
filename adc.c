/*
 * adc.c
 *
 *  Created on: Mar 26, 2026
 *      Author: minhquan
 */
#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
void adc_init(){
    SYSCTL_RCGCADC_R |= 0x01; //Enable two modules (I think I only need one but I enable both for sure)

    SYSCTL_RCGCGPIO_R |= 0x02;//Enable port B

    while (!(SYSCTL_RCGCGPIO_R & 0x02)){};

    GPIO_PORTB_DIR_R &= ~0x10; //Make PB4 input

    GPIO_PORTB_AFSEL_R |= 0x10;//Enable Additional function for Pin 4 port B

    GPIO_PORTB_DEN_R &= ~0x10; //configure the AINx to be analog input

    GPIO_PORTB_AMSEL_R |= 0x10;

    //Configure Sample Sequencer
    //ADC sample rate
    ADC0_PC_R &= ~0xF; //Reset the whole 4 bits so there won't be any noise

    ADC0_PC_R |= 0x1;


    ADC0_SSPRI_R = 0x0123;        // Sequencer 3 is highest priority

    ADC0_ACTSS_R &= ~0x0008;      //   disable sample sequencer 3

    ADC0_EMUX_R &= ~0xF000;       //   seq3 is software trigger

    ADC0_SSMUX3_R &= ~0x000F;

    ADC0_SSMUX3_R = 10;           //  set channel

    ADC0_SSCTL3_R = 0x0006;       //   no TS0 D0, yes IE0 END0

    ADC0_IM_R &= ~0x0008;         //   disable SS3 interrupts

    ADC0_ACTSS_R |= 0x0008; //Enable sample sequencer 3 again
}
int adc_read(void){
    ADC0_PSSI_R = 0x08;

    while((ADC0_RIS_R & 0x08) == 0){}; // wait

    int result = ADC0_SSFIFO3_R ; // 12-bit result

    ADC0_ISC_R = 0x08;               // clear flag

    return result;
}


