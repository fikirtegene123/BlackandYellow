#include <open_interface.h>
#include<movement.h>
//#include <uart-interrupt.h>
#include <math.h>
#include <ping_template.h>
#include <servo.h>
#include <adc.h>
#include <Timer.h >
#define SPEED_RIGHT 100
#define SPEED_LEFT 100
#define BACK_UP_DISTANCE 150
#define DISTANCE_MOVE 200
#define DEGREE_TURN_VERTICAL 5
#define DEGREE_TURN_BIG 15
#define Buffer_Length 1000
#define OBJECT_THRESHOLD 30
#define Start_Deg 0
#define Stop_Deg 180
#define dif_deg 2
#define MAX_OBJECTS 10
#define DELAY 100
#define Buffer_Lenght 50
#define FRONT_RIGHT 2700
#define FRONT_LEFT 2700
#define RIGHT_VERGE 2700
#define LEFT_VERGE 2490
#define HOLE 200
void turn_and_move(oi_t *sensor_data,double angle , double distance);
void final_move(oi_t *sensor_data);
static void bot_brake();
/**
 * We need those two extern variable to interact with interrupt
 */
extern volatile int command_flag ;
extern volatile char prev_char;
void buzzer_sound();

static void back_up(oi_t *sensor,double distance);
int Distance_to_IR(float x){
    float y = -0.0032 * x * x * x
              + 1.5531 * x * x
              - 238.1 * x
              + 12064;
    return y;
}

float width_Calculation(float radius ,float arc_l){
    return 2*radius*sin(arc_l/(2*radius));
}
float Get_arc_length(int degree, float radius){
    return 2*3.14*radius*(degree/360.0);
}

void calibrate_ir_sensor() {
    int i;
    char msg[100];

    uart_sendStr("IR Calibration Mode\r\n");
    uart_sendStr("Place object at known distance, then send any key.\r\n");

    while (1) {

        // wait a little between samples
        timer_waitMillis(500);

        float sum = 0;

        for (i = 0; i < 20; i++) {
            sum += average_ir();   // or adc_read() if that is your raw IR function
            timer_waitMillis(20);
        }

        float avg_ir = sum / 20.0;

        sprintf(msg, "RAW_IR: %.2f\r\nDISTANCE: %.2f\n\r", avg_ir, ir_to_cm(avg_ir));
        if (command_flag ==1){
            uart_sendStr(msg);
        }
    }
}


int verge_detect(oi_t *d){
    if (d->cliffFrontLeftSignal >= FRONT_LEFT){
        return 1;
    }else if (d->cliffLeftSignal >= LEFT_VERGE){
        return 2;
    }else if (d->cliffFrontRightSignal >= FRONT_RIGHT){
        return 3;
    }else if (d->cliffRightSignal >= RIGHT_VERGE){
        return 4;
    }else if (d->cliffFrontLeftSignal <= HOLE) {
        return 5;
    }else if (d->cliffLeftSignal <= HOLE){
        return 6;
    }else if (d->cliffFrontRightSignal <= HOLE){
        return 7;
    }else if (d->cliffRightSignal <= HOLE){
        return 8;
    }
    return 0;
}
void final_move(oi_t *sensor_data){
    int stop_move = 0;

    while(!stop_move){
        oi_update(sensor_data);
        if(command_flag){
           if(command_flag == 1){
               move_foward(sensor_data,(double) DISTANCE_MOVE);
           }
           else if (command_flag == 2){
               back_up(sensor_data,(double) DISTANCE_MOVE);

           }
           else if (command_flag == 3){
               turn_left(sensor_data, (double) (DEGREE_TURN_VERTICAL));

           }
           else if (command_flag == 4){
               turn_right(sensor_data, (double) (DEGREE_TURN_VERTICAL));
           }
           else if (command_flag == 5){
               stop_move = 1;
           }
           else if (command_flag == 6) {
                scan180();
           }
           else if (command_flag == 7) {
               turn_right(sensor_data, (double) (180));
               scan180();
               turn_right(sensor_data, (double) (180));
           }else if (command_flag == 100){
               make_sound();
           }
           command_flag = 0 ;
    }
    }
}
void static bot_brake(){
    oi_setWheels(-50, -50);
    timer_waitMillis(50);
    oi_setWheels(0,0);
}
/**
 * Make the robot to move foward if it gets bumped at sth ,stop and send a message to Putty
 */
double move_foward (oi_t *sensor_data,double distance_mm){
    double sum = 0;
    int bump_thing = 0 ;
    char warning[Buffer_Lenght];
    sprintf(warning,"Object detects on the ");

    while (sum <= distance_mm && !bump_thing){

        oi_update(sensor_data);

        int verge = verge_detect(sensor_data);
        if (sensor_data->bumpLeft && sensor_data->bumpRight){
                    bot_brake();
                    sprintf(warning,"bump middle\n");
                    bump_thing = 1;

                }
                else if (sensor_data->bumpRight){
                    bot_brake();
                    sprintf(warning,"bump right\n");
                    bump_thing = 1;

                }
                else if (sensor_data->bumpLeft){
                    bot_brake();
                    sprintf(warning,"bump left\n");
                    bump_thing = 1;

        }
        if (bump_thing){
            uart_sendStr(warning);
            buzzer_sound();
            continue;
        }
        if (verge){
            bot_brake();
            sprintf(warning,"Has get to the border, detects on ");
            if (verge == 1){
               strcat(warning,"Cliff Front Left\n");
            }else if (verge == 2){
                strcat(warning,"Cliff Left\n");
            }else if (verge == 3){
                strcat(warning,"Cliff Front Right\n");
            }else if(verge ==4){
                strcat(warning,"Cliff Right\n");
            }else if(verge >= 5){
                sprintf(warning,"Has get near a hole detected on ");
                if (verge ==5 ){
                    strcat(warning,"Cliff Front Left\n");
                }else if (verge ==6){
                    strcat(warning,"Cliff Left\n");
                }else if (verge ==7){
                    strcat(warning,"Cliff Front Right\n");
                }else {
                    strcat(warning,"Cliff Right\n");
                }
            }
            bump_thing = 1 ;
        }
        if (bump_thing){
            uart_sendStr(warning);
            buzzer_sound();
            continue;
        }
        oi_setWheels(SPEED_RIGHT,SPEED_LEFT);
        oi_update(sensor_data);
        double travel_distance = sensor_data->distance;
        sum = sum + travel_distance;
    }
    oi_setWheels(0,0);
    return sum;
}
/**
 * Similar to moving foward but just go backward
 * I use the interrupt for the UART message sending so remember to use uart_interrupt_init
 */
static void back_up(oi_t *sensor_data,double distance){
    //turn the opposite way to move
    turn_right(sensor_data, 90.0);
    turn_right(sensor_data, 90.0);
    move_foward(sensor_data, distance);
    //turn the opposite way to move
    turn_right(sensor_data, 90.0);
    turn_right(sensor_data, 90.0);
}
/**
 * Make the robot turn right
 */
double turn_right(oi_t *sensor,double degrees){
    degrees = degrees*0.94;//Cablirate the bots , it will not turn exactly 90 degrees , so some offset
    double turn_already = 0;
    oi_setWheels(-SPEED_RIGHT,SPEED_LEFT);
    /**
         * Turn and update
     */
    while (turn_already < degrees){
        oi_update(sensor);
        turn_already += fabs(sensor->angle);
    }
    oi_setWheels(0,0);
    return turn_already;
}
double turn_left(oi_t *sensor,double degrees){
    degrees = degrees*0.97;
    double turn_already = 0;
    oi_setWheels(SPEED_RIGHT, -SPEED_LEFT);
    /**
     * Turn and update
     */
    while (turn_already < degrees){
        oi_update(sensor);
        turn_already += fabs(sensor->angle);
    }
    oi_setWheels(0,0);
    return turn_already;
}

void loadsong(int song_index, int num_notes, unsigned char *notes, unsigned char *duration)
{
    int i;
    sendchar_song(140);
    sendchar_song(song_index);
    sendchar_song(num_notes);
    for (i = 0; i < num_notes; i++) {
        sendchar_song(notes[i]);
        sendchar_song(duration[i]);
    }
}

/// Plays a given song; use oi_load_song(...) first
void playsong(int index) {
    sendchar_song(141);
    sendchar_song(index);
}
void scan180(){
    uart_sendStr("Angle(Degrees) \t CM :\r\n");

    int angle =0;
    char irmessage[50];
    while (angle<=180){
        servo_move_new(angle);

        timer_waitMillis(20); // CRITICAL: Give the servo 20ms to move/settle
        //uint32_t pulse_width = ping_getPulseWidth();
        float ir_average = average_ir();

        float ir_av;

        ir_av = ir_to_cm(ir_average);

        sprintf(irmessage, "%d \t %.2f\r\n", angle, ir_av);
        //  to generate putty file for graphical display
        uart_sendStr(irmessage);
        angle = angle + 2;
    }

    uart_sendStr("END\n");
}
//Initialize UART4 for song playing
//void song_init(){
//    //enable clock to GPIO port C
//     SYSCTL_RCGCGPIO_R |= 0x04;
//
//     //enable clock to UART4
//     SYSCTL_RCGCUART_R |= 0x10;
//
//     //wait for GPIOC and UART4 peripherals to be ready
//     while ((SYSCTL_PRGPIO_R & 0x04) == 0) {};
//     while ((SYSCTL_PRUART_R & 0x10) == 0) {};
//
//     //enable alternate functions on port C pins
//     GPIO_PORTC_AFSEL_R |= 0x30;
//
//     //enable digital functionality on port B pins
//     GPIO_PORTC_DEN_R |= 0x30;
//
//     GPIO_PORTC_DIR_R |= 0x20;
//     GPIO_PORTC_DIR_R &= ~0x10;
//
//     //enable UART4 port C pins
//     GPIO_PORTC_PCTL_R = GPIO_PORTC_PCTL_R & (~0x00FF0000); //Reset the entire resistor to make sure the set value is correct
//     GPIO_PORTC_PCTL_R |= 0x110000;//pin 4 and 5
//
//     //calculate baud rate
//     uint16_t iBRD = (16000000.0) / (16.0 * 115200.0);
//
//     float dum_cal = (16000000.0) / (16.0 * 115200.0) - iBRD;
//
//     uint16_t fBRD = dum_cal * 64.0 + 0.5;
//
//     //turn off UART1 while setting it up
//     UART4_CTL_R &= ~0x01;
//
//     //set baud rate
//     //note: to take effect, there must be a write to LCRH after these assignments
//     UART4_IBRD_R = iBRD;
//     UART4_FBRD_R = fBRD;
//
//     //set frame, 8 data bits, 1 stop bit, no parity, no FIFO
//     //note: this write to LCRH must be after the BRD assignments
//     UART4_LCRH_R = 0x0000060;
//
//     //use system clock as source
//     //note from the datasheet UARTCCC register description:
//     //field is 0 (system clock) by default on reset
//     //Good to be explicit in your code
//     UART4_CC_R = 0x0;
//
//     //re-enable UART1 and also enable RX, TX (three bits)
//     //note from the datasheet UARTCTL register description:
//     //RX and TX are enabled by default on reset
//     //Good to be explicit in your code
//     //Be careful to not clear RX and TX enable bits
//     //(either preserve if already set or set them)
//     UART4_CTL_R |= 0x301;
//}
void make_sound(){

        unsigned char notes[] = {
            60, 60, 62, 60, 65, 64,
            60, 60, 62, 60, 67, 65,
            60, 60, 72, 69
        };

        unsigned char durations[] = {
            12, 12, 24, 24, 24, 48,
            12, 12, 24, 24, 24, 48,
            12, 12, 24, 24
        };

        oi_loadSong(0, 16, notes, durations);
        oi_play_song(0);

}
void buzzer_sound(){
    // Phase 1: rapid pulse (low-mid)
        unsigned char notes1[] = {
            85, 70, 85, 70, 85, 70, 85, 70,
            88, 72, 88, 72, 88, 72, 88, 72
        };
        unsigned char dur1[] = {
            4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,4,4
        };

        // Phase 2: higher + faster (more panic)
        unsigned char notes2[] = {
            95, 75, 95, 75, 95, 75, 95, 75,
            100, 80, 100, 80, 100, 80, 100, 80
        };
        unsigned char dur2[] = {
            3,3,3,3,3,3,3,3,
            3,3,3,3,3,3,3,3
        };

        // Phase 3: final impact hits
        unsigned char notes3[] = {
            110, 90, 110, 90, 110, 90, 110, 60
        };
        unsigned char dur3[] = {
            6,6,6,6,6,6,6,20
        };

        oi_loadSong(1, 16, notes1, dur1);
        oi_loadSong(2, 16, notes2, dur2);
        oi_loadSong(3, 8,  notes3, dur3);

        oi_play_song(1);
        timer_waitMillis(500);

        oi_play_song(2);
        timer_waitMillis(400);

        oi_play_song(3);
}
void check_cliff_sensors(oi_t *sensor) {
    oi_update(sensor);

    char msg[100];

    sprintf(msg,
        "FL: %d | L: %d | FR: %d | R: %d\r\n",
        sensor->cliffFrontLeftSignal,
        sensor->cliffLeftSignal,
        sensor->cliffFrontRightSignal,
        sensor->cliffRightSignal);
    if (command_flag ==1){
        uart_sendStr(msg);
    }
}
