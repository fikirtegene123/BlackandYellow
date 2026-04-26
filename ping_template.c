/**
 * Driver for ping sensor
 * @file ping.c
 */

#include "ping_template.h"
#include "Timer.h"

// Global shared variables

volatile uint32_t g_start_time = 0;
volatile uint32_t g_end_time = 0;
volatile ping_state_t g_state = LOW;

// Per-measurement overflow counter
volatile int overflowCount = 0;

// Global lifetime overflow counter (defined elsewhere)
extern volatile int totalOverflowCount;


// Initialization
void ping_init(void) {

    // Enable clocks for Timer3 and Port B
    SYSCTL_RCGCTIMER_R |= 0x08;   // Timer3 clock
    SYSCTL_RCGCGPIO_R  |= 0x02;   // Port B clock
    while ((SYSCTL_PRGPIO_R & 0x02) == 0) {}

    // PB3 digital enable
    GPIO_PORTB_DEN_R |= 0x08;

    // Set PB3 as T3CCP1
    GPIO_PORTB_AFSEL_R |= 0x08;
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0x0000F000) | 0x00007000;

    // Disable Timer3B during setup
    TIMER3_CTL_R &= ~0x0100;

    // 16-bit timer configuration
    TIMER3_CFG_R = 0x4;

    // Capture mode, edge-time, count-down
    TIMER3_TBMR_R = 0x07;
    TIMER3_TBMR_R &= ~0x10;

    // Both edges trigger
    TIMER3_CTL_R |= 0x0C00;

    // Max 24-bit value
    TIMER3_TBPR_R  = 0xFF;
    TIMER3_TBILR_R = 0xFFFF;

    // Clear interrupts
    TIMER3_ICR_R = 0x0500; // clear BOTH capture + timeout

    // Enable interrupts
    TIMER3_IMR_R |= 0x0400; // capture
    TIMER3_IMR_R |= 0x0100; // overflow

    // NVIC enable (interrupt 36 → EN1 bit 4)
    NVIC_EN1_R |= 0x00000010;

    IntRegister(INT_TIMER3B, TIMER3B_Handler);
    IntMasterEnable();

    // Enable timer
    TIMER3_CTL_R |= 0x0100;
}


// Trigger pulse
void ping_trigger(void) {

    g_state = LOW;

    // Stop timer + interrupts while triggering
    TIMER3_CTL_R &= ~0x0100;
    TIMER3_IMR_R &= ~(0x0400 | 0x0100);

    // Set PB3 as GPIO output
    GPIO_PORTB_AFSEL_R &= ~0x08;
    GPIO_PORTB_DIR_R   |= 0x08;

    // Send 10us trigger pulse
    GPIO_PORTB_DATA_R &= ~0x08;
    GPIO_PORTB_DATA_R |= 0x08;
    timer_waitMicros(10);
    GPIO_PORTB_DATA_R &= ~0x08;

    // Prepare for capture
    GPIO_PORTB_DIR_R &= ~0x08;

    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0x0000F000) | 0x00007000;
    GPIO_PORTB_AFSEL_R |= 0x08;

    // Clear interrupts
    TIMER3_ICR_R = 0x0500;

    // Re-enable interrupts + timer
    TIMER3_IMR_R |= (0x0400 | 0x0100);
    TIMER3_CTL_R |= 0x0100;
}


// ISR
void TIMER3B_Handler(void) {

    // Capture event
    if (TIMER3_MIS_R & 0x0400) {

        TIMER3_ICR_R |= 0x0400;

        if (g_state == LOW) {
            overflowCount = 0;                 // reset per measurement
            g_start_time = TIMER3_TBR_R;
            g_state = HIGH;
        }
        else if (g_state == HIGH) {
            g_end_time = TIMER3_TBR_R;
            g_state = DONE;

            // Stop timer + interrupts
            TIMER3_IMR_R &= ~(0x0400 | 0x0100);
            TIMER3_CTL_R &= ~0x0100;
        }
    }

    // Overflow event
    if (TIMER3_MIS_R & 0x0100) {

        TIMER3_ICR_R |= 0x0100;

        overflowCount++;        // per measurement
        totalOverflowCount++;   // lifetime counter
    }
}


// Get pulse width
uint32_t ping_getPulseWidth(void) {

    ping_trigger();

    // Wait until measurement completes
    while (g_state != DONE) {}

    uint32_t delta;

    // Handle wrap-around safely
    if (g_start_time >= g_end_time) {
        delta = g_start_time - g_end_time;
    } else {
        delta = (g_start_time + 0x1000000) - g_end_time;
    }

    uint32_t total_counts =
        (overflowCount * 0x1000000) + delta;

    return total_counts;
}
