#ifndef __OUTPUTS_H__
#define __OUTPUTS_H__
#include <Servo.h>

typedef struct {
  uint8_t pin;
  uint8_t min;
  uint8_t max;
  uint8_t center;
  bool reverse;
  Servo servo;
} ServoConfig;


static ServoConfig LeftServo = {
  .pin=12,
  .min=40,  //Upwards
  .max=120, //Downwards
  .center=90,
  .reverse=false
};

static ServoConfig RightServo = {
  .pin=14,
  .min=60,
  .max=140,
  .center=90,
  .reverse=true
};



void init_outputs(void);
void handle_channels(float channels[], uint8_t channel_len);

#endif
