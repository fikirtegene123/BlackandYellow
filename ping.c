/**
 * Driver for ping sensor
 * @author Quan Nguyen (I write take the code structure from lab9_template)
 * This is what I got for the whole lab, I make some notes during the code so I could 
 * remember the concepts during the lab( I just want to save some times)
 */

#include "ping.h"
#include "Timer.h"
#include "stdint.h"
#include "stdio.h"
// Global shared variables
// Use extern declarations in the header file

volatile uint32_t g_start_time = 0;
volatile uint32_t g_end_time = 0;
volatile state_t g_state = LOW; // State of ping echo pulse

void ping_init (void){

  // YOUR CODE HERE

    IntRegister(INT_TIMER3B, TIMER3B_Handler);

    IntMasterEnable();

    // Configure and enable the timer
    init_timer();
}

void ping_trigger (void){
    g_state = LOW;
    // Disable timer and disable timer interrupt
    TIMER3_CTL_R &= ~TIMER_CTL_TBEN ;//Just disable bit 8 TBEN
    TIMER3_IMR_R &= ~TIMER_IMR_CBEIM;//from bit 8-11 is for timer B interrupt mask
    // Disable alternate function (disconnect timer from port pin)
    GPIO_PORTB_AFSEL_R &=~0x08;//Set it back to normal GPIO

    // YOUR CODE HERE FOR PING TRIGGER/START PULSE
    init_portB_output();
    // Clear an interrupt that may have been erroneously triggered
    TIMER3_ICR_R |= TIMER_ICR_CBECINT; //Clear the interrupt for capture event 

    
    // Re-enable alternate function, timer interrupt, and timer
    GPIO_PORTB_AFSEL_R |=0x08;
    TIMER3_IMR_R |= TIMER_IMR_CBEIM;
    GPIO_PORTB_PCTL_R |=0x7000 ;
    TIMER3_CTL_R |=TIMER_CTL_TBEN; //ENABLE TBEN AGAIN
}

void TIMER3B_Handler(void){

  // YOUR CODE HERE
  // As needed, go back to review your interrupt handler code for the UART lab.
  // What are the first lines of code in the ISR? Regardless of the device, interrupt handling
  // includes checking the source of the interrupt and clearing the interrupt status bit.
  // Checking the source: test the MIS bit in the MIS register (is the ISR executing
  // because the input capture event happened and interrupts were enabled for that event?
  // Clearing the interrupt: set the ICR bit (so that same event doesn't trigger another interrupt)
  // The rest of the code in the ISR depends on actions needed when the event happens.
  if(TIMER3_MIS_R & TIMER_MIS_CBEMIS){//If the interrupt is triggered
        TIMER3_ICR_R = TIMER_ICR_CBECINT;//Clear the trigger
        if(g_state == LOW){
            g_start_time = TIMER3_TBR_R;//configure as edge-time mode so store time
            //Last edge took place
            g_state = HIGH;
        }
        else if(g_state == HIGH){
            g_end_time = TIMER3_TBR_R;
            g_state = DONE;
        }
    }
}

float ping_getDistance (void){

    // YOUR CODE HERE
    while(g_state != DONE);//Wait until the signal is done

    uint32_t pulse;

   // if(g_start_time > g_end_time){
        pulse = g_start_time - g_end_time;
  //  }
//    else{
//        pulse = (g_start_time + (0xFFFFFF - g_end_time));
//    }
    //f = 16MHZ, so T is almost 6.25*10^-8--> so we just use sound of speed 
    //343 * T * number of ticks (which is and then devide by two which will give use 

    float distance_per_period = 343.0 / (2.0 * 16000000.0);

    float distance = pulse * distance_per_period;

    return distance;
}
// #include <inc/tm4c123gh6pm.h>
void init_portB_Timer(){
    SYSCTL_RCGCGPIO_R |=0x02;//Enable clock for port B
    GPIO_PORTB_AFSEL_R |=0x08 ;//Set pin 3 of port B to use alternate function
	GPIO_PORTB_PCTL_R |=0x7000 ; //Set approriate value for the bit
}
void init_timer(){
    init_portB_Timer();
    SYSCTL_RCGCTIMER_R |=0x08;//Enable Timer Module 3,for 32/64 There would be a w here (I note this down so I can remember it in the lab)
	TIMER3_CTL_R &= ~TIMER_CTL_TBEN; //Disable The timer  
    TIMER3_CFG_R |= 0x04; // select the 16-bit timer configuration
    TIMER3_TBMR_R |= 0x07; //Capture mode + edge time mode
    TIMER3_TBMR_R &= ~0x10; // Count direction

    NVIC_EN0_R |= 0x02;
    NVIC_EN1_R |= 0x10;
    TIMER3_CTL_R |= TIMER_CTL_TBEVENT_BOTH; //Triggered by both edge
    TIMER3_TBPR_R = 0xFF;//set the Timer B prescale, this will help the timer to count from FFFFFF to 000000
    TIMER3_TBILR_R = 0xFFFF; //Writing this field loads the counter for Timer B
    TIMER3_ICR_R |= TIMER_ICR_CBECINT; // clear capture event flag
    TIMER3_IMR_R |= TIMER_IMR_CBEIM; //Capture event interrupt
    TIMER3_CTL_R |=TIMER_CTL_TBEN;//Enalble the timer again
}
void init_portB_output(){
    SYSCTL_RCGCGPIO_R |=0x02;
    GPIO_PORTB_AFSEL_R &=~0x08 ;//Make it normal gpio
	GPIO_PORTB_DIR_R |= 0x08;//set output
    GPIO_PORTB_DEN_R |= 0x08;//enable digital function
    //Sending  low-hihg-low
    GPIO_PORTB_DATA_R &= ~0x08;//off
    timer_waitMicros(2);
    GPIO_PORTB_DATA_R |= 0x08;
    timer_waitMicros(5); //Wait for 5 micros second before setting output 0 (falling edge)
    GPIO_PORTB_DATA_R &= ~0x08;
}
