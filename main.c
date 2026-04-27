/**
 * lab6-interrupt_template.c
 *
 * Template file for CprE 288 Lab 6
 *
 * @author Diane Rover, 2/15/2020
 *
 */

#include "Timer.h"
#include "ping_template.h"
#include "lcd.h"
#include <adc.h>
#include <math.h>
#include "stdint.h"
#include <servo.h>
// Uncomment or add any include directives that you want to use
#include "open_interface.h"
#include "button.h"

//#include "uart.h"
extern volatile int command_flag ;
extern volatile char prev_char;
int main(void){
    lcd_init();
    //Init interrupt
    uart_interrupt_init();
    //Init oi sensor to track movement and sensors
    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);
    //Init Timer
    timer_init();
    //Init servo
    servo_init_new();
    //Init Pin sensors
    ping_init();
    //Call move function
    final_move(sensor_data);
    //Free the object
    oi_free(sensor_data);
}
//int main(void){
//    lcd_init();
//    servo_init_new();
//    button_init();
//    int mode = 1; //counterclockwise mode by default
//    lcd_printf("%s","start checking");
//    int current_angle = 90;
//    uint32_t match = 300000; // starting point
//
//    while (1){
//        int btn = button_getButton();
//
//        if (btn == 1){
//            match += 500;   // small step
//            calibrate_servo(match);
//        }
//        else if (btn == 2){
//            match -= 500;
//            calibrate_servo(match);
//        }
//
//        lcd_printf("match: %d", match);
//    }
//}

