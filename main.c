#include "Timer.h"
#include "ping_template.h"  
#include "lcd.h"
#include <adc.h>
#include <math.h>
#include "stdint.h"
#include <servo.h>
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

