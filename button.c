/*
 * button.c
 *
 *  Created on: Jul 18, 2016
 *      Author: Eric Middleton, Zhao Zhang, Chad Nelson, & Zachary Glanz.
 *
 *  @edit: Lindsey Sleeth and Sam Stifter on 02/04/2019
 *  @edit: Phillip Jones 05/30/2019: Merged Spring 2019 version with Fall 2018
 *  @edit: Diane Rover 02/01/20: Corrected comments about ordering of switches for new LCD board and added busy-wait on PRGPIO
 */



//The buttons are on PORTE 3:0
// GPIO_PORTE_DATA_R -- Name of the memory mapped register for GPIO Port E,
// which is connected to the push buttons
#include "button.h"
uint8_t button_getButton();
void button_init();
/**
 * Initialize PORTE and configure bits 0-3 to be used as inputs for the buttons.
 */
static uint8_t initialized = 0;

void button_init() {

	SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R | 0x10;

    while((SYSCTL_PRGPIO_R & 0x10) == 0){}; // Wait until Port E is ready
    //Direction: Set PE0-PE3 to 0 (Input)
    GPIO_PORTE_DIR_R &= ~0x0F;

    GPIO_PORTE_AFSEL_R &= ~0x0F;  // 0 = Regular GPIO
    // Digital Enable: Set PE0-PE3 to 1
    GPIO_PORTE_DEN_R |= 0x0F;


	initialized = 1;
}



/**
 * Returns the position of the rightmost button being pushed.
 * @return the position of the rightmost button being pushed. 1 is the leftmost button, 4 is the rightmost button.  0 indicates no button being pressed
 */
uint8_t button_getButton() {


    if (!initialized){
        return 15;
    }

    unsigned int button_status = (GPIO_PORTE_DATA_R & 0x0F);
        if (!(GPIO_PORTE_DATA_R & 0x01)){ //check the first bit
            return 1;
        } else if (!(GPIO_PORTE_DATA_R & 0x02)){ //check the second bit
            return 2;
        } else if (!(GPIO_PORTE_DATA_R & 0x04)){ //check the third bit
             return 3;
        } else if (!(GPIO_PORTE_DATA_R & 0x08)){ //check the fourth bit
            return 4;
        }
     return 0;
}
