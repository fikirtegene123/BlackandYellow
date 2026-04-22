#include <open_interface.h>
#include <cyBot_Scan.h>
#include<movement.h>
//#include <uart-interrupt.h>
#include <math.h>

#define SPEED_RIGHT 200
#define SPEED_LEFT 200
#define BACK_UP_DISTANCE 150
#define DISTANCE_MOVE 200
#define DEGREE_TURN_VERTICAL 90
#define Buffer_Length 1000
#define OBJECT_THRESHOLD 30
#define Start_Deg 0
#define Stop_Deg 180
#define dif_deg 2
#define MAX_OBJECTS 10
#define DELAY 100
#define Buffer_Lenght 50
void turn_and_move(oi_t *sensor_data,double angle , double distance);
void final_move(oi_t *sensor_data);
/**
 * We need those two extern variable to interact with interrupt
 */
extern volatile int command_flag ;
extern volatile char prev_char;


void scanRange(int start, int end){
    cyBOT_Scan_t scan;
    char Header[] = "Angle\tDistance\tRaw_IR\r\n";
    int i;
    for (i=0;i<strlen(Header);i++){
        uart_sendChar(Header[i]);
    }
    char Buffer_info[40];
    int current_angle;
    for (current_angle = start; current_angle <= end; current_angle+=dif_deg){
      cyBOT_Scan(current_angle, &scan);
        snprintf(Buffer_info,sizeof(Buffer_info),"%-10d %-18.2f\t%d\r\n",current_angle,scan.sound_dist,scan.IR_raw_val);
        for (i=0;i<strlen(Buffer_info);i++){
            uart_sendChar(Buffer_info[i]);
         }
    }

}
void scanRangeLab8(){
    char Header[] = "Distance\tRaw_IR\r\n";
    cyBOT_Scan_t scan;
    int i;
    for (i=0;i<strlen(Header);i++){
        uart_sendChar(Header[i]);
    }
    char Buffer_info[40];

    int k;
    int IR_sum = 0 ;
    float Distance_sum = 0;
    for (k =0 ;  k<16; k++){
        cyBOT_Scan(90, &scan);
        IR_sum += scan.IR_raw_val;
        Distance_sum+= scan.sound_dist;
    }
    snprintf(Buffer_info,sizeof(Buffer_info),"%-18.2f\t%d\r\n",Distance_sum/16.0, IR_sum/16);
    uart_blocking_sendStr(Buffer_info);
}
int read_avg_adc(int samples) {
    int sum = 0;
    int i ;
    for (i = 0; i < samples; i++) {
        sum += adc_read();
        timer_waitMillis(10); // small delay between samples
    }
    return sum / samples;
}
void turn_and_move(oi_t *sensor_data, double angle , double distance){
    if (distance < 0 || angle < 0){
        lcd_printf("Invalid input");
        return;
    }

    lcd_printf("Angle: %.1f Dist: %.1f", angle, distance);

    char buf[100];

    if (angle < 90.0){
        double turn = 90.0 - angle;
        turn_right(sensor_data, turn);
        sprintf(buf, "Turn right %.1f deg\r\n", turn);
    }
    else if (angle > 90.0){
        double turn = angle - 90.0;
        turn_left(sensor_data, turn);
        sprintf(buf, "Turn left %.1f deg\r\n", turn);
    }
    else {
        sprintf(buf, "No turn, moving straight\r\n");
    }

    uart_blocking_sendStr(buf);

    move_foward(sensor_data, distance);
}
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

int scanObjects_upgrade(Object objects[], Object *min_Obj){

    int Threshold = 600;

    lcd_printf("threshold is %d",Threshold);

    cyBOT_Scan_t scan;

    float min_width =0;

    int min_angle = 0;


    int object_count = 0;

    int in_object = 0;//Detect if we have hit an object or not

    int object_start = 0;

    int angle;

    for(angle = Start_Deg; angle <= Stop_Deg; angle += dif_deg)
    {
        cyBOT_Scan(angle, &scan);

        int Measured_IR ;
        if (in_object){
            int sum = 0;
            int i;
            for(i = 0; i < 5; i++){
                cyBOT_Scan(angle, &scan);
                sum += scan.IR_raw_val;
            }
          Measured_IR = sum / 5;
        }else{
            Measured_IR = scan.IR_raw_val;
        }
        // Object starts
        if(Measured_IR > 0 && (Measured_IR > Threshold) && !in_object && scan.sound_dist!=0.0)
        {
            in_object = 1;
            object_start = angle;
        }
        // Object ends
        if((Measured_IR <= Threshold || angle == Stop_Deg) && in_object)
        {
            in_object = 0;

            int object_end = angle;


            if(object_count < MAX_OBJECTS && (object_end-object_start) >4)
            {
                Object *obj = &objects[object_count];

                //obj->object_number = object_count + 1;

                obj->start_angle = object_start;

                obj->end_angle = object_end;


                obj->middle_angle = (object_start + object_end) / 2;


                // Re-scan at middle for better distance accuracy
                cyBOT_Scan(obj->middle_angle, &scan);

                timer_waitMillis(DELAY);

                obj->IR_val = scan.IR_raw_val;

                obj->distance = scan.sound_dist;

                float arc_l = Get_arc_length(object_end - object_start, scan.sound_dist);

                float width = width_Calculation(scan.sound_dist, arc_l);

                obj->width = width;

                if (min_width == 0 || width < min_width){
                    min_width = width;
                    min_Obj->distance = obj->distance;
                    min_Obj->middle_angle = obj->middle_angle;
                }

                object_count++;
            }
        }
    }

    return object_count;
}

//double move_foward (oi_t *sensor_data,double distance_mm){
//    double sum = 0;
//    int num_turn_left = 0;
//    int num_turn_right = 0;
//    while (sum<=distance_mm){
//        if (sensor_data->bumpLeft || sensor_data->bumpRight){
//            int check_turn = 0;
//            int LEFT = sensor_data->bumpLeft;
//            int RIGHT = sensor_data->bumpRight;
//            back_up(sensor_data,BACK_UP_DISTANCE);
//            sum = sum - BACK_UP_DISTANCE;
//            if (RIGHT){
//                check_turn = turn_left(sensor_data,90.0);
//                check_turn = 0;
//                num_turn_left++;
//            }else if (LEFT){
//                check_turn = turn_right(sensor_data,90.0);
//                check_turn = 1;
//                num_turn_right++;
//            }
//            double dummy = move_foward(sensor_data,250);
//            if (check_turn){
//                dummy = turn_left(sensor_data,90);
//            }else{
//                dummy = turn_right(sensor_data,90);
//            }
//            oi_update(sensor_data);
//            continue;
//        }
//        oi_setWheels(SPEED_RIGHT,SPEED_LEFT);
//        oi_update(sensor_data);
//        double travel_distance = sensor_data->distance;
//        sum = sum + travel_distance;
//    }
//    if (num_turn_left){
//        turn_right(sensor_data,90.0);
//        move_foward(sensor_data , 250.0*num_turn_left);
//        num_turn_left= 0;
//    }if (num_turn_right){
//        turn_left(sensor_data,90.0);
//        move_foward(sensor_data , 250.0*num_turn_right);
//        num_turn_right= 0;
//    }
//    oi_setWheels(0,0);
//
//    return sum;
//}
/**
 *
 */
void final_move(oi_t *sensor_data){
    int stop_move = 0;
    while(!stop_move){
        if(command_flag){
           if(command_flag == 1){
               move_foward(sensor_data,(double) DISTANCE_MOVE);
           }
           else if (command_flag == 2){
               back_up(sensor_data,(double) DISTANCE_MOVE);
           }
           else if (command_flag == 3){
               turn_left(sensor_data, (double) (DEGREE_TURN_VERTICAL));
               move_foward(sensor_data,(double) DISTANCE_MOVE);
               turn_right(sensor_data, (double) (DEGREE_TURN_VERTICAL));
           }
           else if (command_flag == 4){
               turn_right(sensor_data, (double) (DEGREE_TURN_VERTICAL));
               move_foward(sensor_data,(double) DISTANCE_MOVE);
               turn_left(sensor_data, (double) (DEGREE_TURN_VERTICAL));
           }
           else if (command_flag == 5){
               stop_move = 1;
           }
           command_flag = 0;
        }
    }
}
/**
 * Make the robot to move foward if it gets bumped at sth ,stop and send a message to Putty
 */
double move_foward (oi_t *sensor_data,double distance_mm){
    double sum = 0;
    int bump_thing = 0 ;
    char warning[Buffer_Lenght];
    strcat(warning,"\n\rObject detects on the ");
    while (sum <= distance_mm && !bump_thing){
        if (sensor_data->bumpLeft && sensor_data->bumpRight){
            strcat(warning,"middle");
            bump_thing = 1;
        }
        else if (sensor_data->bumpRight){
            strcat(warning,"right");
            bump_thing = 1;
        }
        else if (sensor_data->bumpLeft){
            strcat(warning,"left");
            bump_thing = 1;
        }
        if (bump_thing){
            uart_sendStr(warning);
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
    degrees = degrees*0.90;//Cablirate the bots , it will not turn exactly 90 degrees , so some offset
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
    degrees = degrees*0.90;
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
