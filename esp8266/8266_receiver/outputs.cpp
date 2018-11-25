#include "outputs.h"

void initServo(ServoConfig* servo){
  servo->servo.attach(servo->pin);
  servo->servo.write(servo->center);
}

void writeServo(ServoConfig* servo, float percent){
  if (servo->reverse){
    percent *= -1;
  }
  if (percent > 0){
    int8_t delta_up = servo->max - servo->center;
    servo->servo.write(servo->center + percent * delta_up);
  } else {
    int8_t delta_down = servo->center - servo->min;
    servo->servo.write(servo->center + percent * delta_down);
  }
}

void init_outputs(){
  initServo(&LeftServo);
  initServo(&RightServo);
}

void handle_channels(float channels[], uint8_t channel_len){
  float left = -channels[1] + channels[0];
  float right = -channels[1] - channels[0];
  writeServo(&LeftServo, left);
  writeServo(&RightServo, right);
}
