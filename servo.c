#include "servo.h"
#include "Timer.h"
#include "stdint.h"
#include "stdio.h"
#include "REF_tm4c123gh6pm.h"
#include "lcd.h"
//Threshold 16000: 0 , 24000: 90 , 32000: 180
/* This code is written by Quan Nguyen section k-1-2
    I tried to do part 1 of this lab, pretty long and I make some node so I 
    can remember when I get to the lab */
#define PMW_PERIOD 20
#define PMW_PERIOD_MATCHING 320000
void servo_init_new(void);
void servo_move_new(uint32_t degrees);
void init_pwm(void);
void init_timer_pwm(void);
float extract_slope(void);
float extract_b(float slope);
void calibrate_servo(uint32_t calibrate_val);
static uint32_t servo_right_matching = 311500 ;   // 0
static uint32_t servo_left_matching  = 284500 ;  // 180

void init_timer_pwm(void){
    SYSCTL_RCGCGPIO_R |= 0x02;   //Enable clock for port B
    SYSCTL_RCGCTIMER_R |= 0x02;  // Timer1

    GPIO_PORTB_AFSEL_R |= 0x20; ////set AFSEL register of pin 5 port B
    GPIO_PORTB_DEN_R   |= 0x20;
    GPIO_PORTB_PCTL_R &= ~0x00F00000;//Clean the bit field
    GPIO_PORTB_PCTL_R |=  0x00700000;//Set so it functions as timer

    TIMER1_CTL_R &= ~TIMER_CTL_TBEN;   // disable Timer1B during setup
    TIMER1_CFG_R = 0x04;               // 16-bit timer
    TIMER1_TBMR_R = 0x0A;              // periodic + PWM mode
   // TIMER1_CTL_R |= TIMER_CTL_TBPWML; // //GPTM Timer B PWM Output Level

    TIMER1_TBPR_R   = 0x04;            // upper 8 bits of 320000
    TIMER1_TBILR_R  = 0xE200;          // lower 16 bits of 320000

    TIMER1_TBPMR_R    = 0x04;          // upper 8 bits of 296000
    TIMER1_TBMATCHR_R = 0x8440;        // lower 16 bits of 296000

    TIMER1_CTL_R |= TIMER_CTL_TBEN;
}
void init_pwm(void){
    SYSCTL_RCGC0_R |= 0x0010000; //Enable PWM Clock Gating Control pin 20 step 1 in datasheet
    SYSCTL_RCGC2_R |= 0x02; //Enable bit 1 Port B glock gating control
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; //Enable clock devidor
    SYSCTL_RCC_R |= SYSCTL_RCC_PWMDIV_2; //Devide by 2
    PWM0_CTL_R &= ~0xFFFFFFFF; //This is weird , I should ask Jason
    PWM0_0_GENA_R |= 0x0000008C;//Counter = load drive pwmA High, counter = 0 do nothing, comparator A down drive PMWA Low
    PWM0_0_GENB_R |= 0x0000080C; //opposite to GENA
    PWM0_0_LOAD_R |= 0x0000018F;
    PWM0_0_CMPA_R |= 0x0000012B; // Set the pulse width of the MnPWM0 pin for a 25% duty cycle
    PWM0_0_CMPB_R |= 0x00000063;
    PWM0_0_CTL_R |= 0x00000001;
    PWM0_ENABLE_R |= 0x00000003;
}
void servo_init_new(void){
    init_timer_pwm();
    //init_pwm(void);
}

//void servo_move_new(uint32_t degrees)
//{
//    if (degrees > 180) {
//        degrees = 180;
//    }
//
//
//    float time_pulse = ((float)degrees + 180) / 180;   // get the time_pulse value
//
//    float match_time = (float) PMW_PERIOD - time_pulse;                  // find match time
//
//    //match_time / clock period and clock period is liek 1/f of the system clock --> match time * f
//    uint32_t match_val = (uint32_t)(match_time * 16000.0); // counts at 16 MHz
//
//    TIMER1_TBMATCHR_R = match_val & 0xFFFF;         // lower 16 bits
//    TIMER1_TBPMR_R    = (match_val >> 16) & 0xFF;   // upper 8 bits
//    lcd_printf("match_val:%d \n angle val:%d",match_val,degrees);
//    timer_waitMillis(200);
//}
// The formula I extracted is angle (Degrees) = 180 * time_pulse(ms) - 180;
//    Therefore , if we want to get time pulse we should have (angle + 180)/ 180 or angle/180 + 1
void servo_move_new(uint32_t degrees)
{
    if (degrees > 180) {
        degrees = 180;
    }

    float slope = extract_slope();
    float b_val = extract_b(slope);

    float time_pulse = ((float)degrees - b_val) / slope;   // get the time_pulse value

    float match_time = (float) PMW_PERIOD - time_pulse;                  // find match time

    //match_time / clock period and clock period is liek 1/f of the system clock --> match time * f
    uint32_t match_val = (uint32_t)(match_time * 16000.0); // counts at 16 MHz

    TIMER1_TBMATCHR_R = match_val & 0xFFFF;         // lower 16 bits
    TIMER1_TBPMR_R    = (match_val >> 16) & 0xFF;   // upper 8 bits
    lcd_printf("match_val:%d \n angle val:%d",match_val,degrees);
    timer_waitMillis(200);
}
float extract_slope(void){
    float up_time_right = (float) (PMW_PERIOD_MATCHING - servo_right_matching);//pulse_width in duty cycle
    float up_time_left = (float) (PMW_PERIOD_MATCHING - servo_left_matching);//pulse_width in duty cycle 
    float pulse_time_at_right = (float) up_time_right / 16000.0;//This is millisecond
    float pulse_time_at_left = (float) up_time_left /16000.0;
    return 180.0/(pulse_time_at_left-pulse_time_at_right);
}
float extract_b(float slope){
    float up_time_right = (float) (PMW_PERIOD_MATCHING - servo_right_matching);
    float pulse_time_at_right = (float) up_time_right / 16000.0;//This is millisecond
    return -(pulse_time_at_right*slope);
}
void calibrate_servo(uint32_t calibrate_val){
    
    TIMER1_TBMATCHR_R = calibrate_val & 0xFFFF;         // lower 16 bits
    TIMER1_TBPMR_R    = (calibrate_val >> 16) & 0xFF;   // upper 8 bits

    timer_waitMillis(200);
}
