#include <inttypes.h>
#include <Arduino.h>
#include <Servo.h>
#include "tranceiver.h"

#define BATTERY_SCALER 620
#define SERVO_LEFT_PIN 14
#define SERVO_RIGHT_PIN 12
const uint8_t name[] = "Tichy Stick v3";

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
  .min=20,
  .max=140,
  .center=90,
  .reverse=true
};

static ServoConfig RightServo = {
  .pin=14,
  .min=20,
  .max=140,
  .center=90,
  .reverse=true
};


bool is_connected = false;


void initServo(ServoConfig* servo){
  servo->servo.attach(servo->pin);
  servo->servo.write(servo->center);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Begin Init Servos");
  initServo(&LeftServo);
  initServo(&RightServo);
  Serial.println("Begin Init Tranceiver");
  tranceiver_init();
  tranceiver_set_channel(1);

  Serial.println("Init Complete");
}

// ------------------------ Sensors -----------------------
uint16_t getBatteryMillVolts(){
  uint16_t raw = analogRead(A0);
  uint16_t corrected = raw * BATTERY_SCALER / 100;
  return corrected;
}

uint16_t telem_counter = 0;

// the loop function runs over and over again forever
void loop() {
  telem_counter += 1;
  if (telem_counter > 10000){
    telem_counter = 0;
  }
  
  if (telem_counter % 1000 == 0){
    uint16_t batt_voltage = getBatteryMillVolts();
    telemetry_status batt_status = TELEMETRY_OK;
    if (batt_voltage < 2500+700){
      Serial.println("Battery Voltage Low");
      batt_status = TELEMETRY_ERROR;
    }
    tranceiver_send_telemtry(
        batt_status, 
        float(batt_voltage)/1000, 
        "battery voltage", sizeof("battery voltage")
    );
    
  }
  if (telem_counter % 1000 == 500 and not is_connected){
    tranceiver_send_name_packet(name, sizeof(name));
  }
  delay(1);
}
