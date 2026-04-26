/*
 * movement.h
 *
 *  Created on: Feb 5, 2026
 *      Author: minhquan
 */

#ifndef MOVEMENT_H_
#define MOVEMENT_H_
typedef struct {
    int start_angle;
    int end_angle;
    int middle_angle;
    float distance;
    float width;
    int IR_val;
} Object;
double move_foward(oi_t *sensor_data,double distance_mm);
double turn_right(oi_t *sensor,double degrees);
double turn_left(oi_t *sensor, double degrees);
int scanObjects(Object objects[]);
void scanRange(int start, int end);
float width_Calculation(float radius ,float arc_l);
int scanObjects_upgrade(Object objects[],Object *min_Obj);
void turn_and_move(oi_t *sensor_data,double angle , double distance);
void scanRangeLab8();
int read_avg_adc(int samples);
void final_move(oi_t *sensor_data);
void loadsong(int song_index, int num_notes, unsigned char *notes, unsigned char *duration);
void playsong(int index);
#endif /* MOVEMENT_H_ */
