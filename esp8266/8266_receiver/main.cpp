#include <inttypes.h>
#include <Arduino.h>
#include "tranceiver.h"
#include "outputs.h"
#include "telemetry.h"

#define BATTERY_SCALER 620
#define SERVO_LEFT_PIN 14
#define SERVO_RIGHT_PIN 12
const uint8_t name[] = "Tichy Stick v3";

TelemChannel telem_batt_voltage = {
  "Battery Voltage",
  TELEMETRY_UNDEFINED,
  0.0,
};
TelemChannel telem_rssi = {
  "RSSI",
  TELEMETRY_UNDEFINED,
  0.0,
};

void setup() {
  Serial.begin(115200);
  Serial.println("Begin Init Radio");
  tranceiver_init();
  tranceiver_set_channel(1);
  tranceiver_enable_filter_by_id(true);
  Serial.println("Begin Init Servos");
  init_outputs();
  Serial.println("Begin Init Telemetry");
  register_telem(&telem_batt_voltage);
  register_telem(&telem_rssi);
  Serial.println("Init Complete");
}

// ------------------------ Sensors -----------------------
uint16_t getBatteryMillVolts(){
  uint16_t raw = analogRead(A0);
  uint16_t corrected = raw * BATTERY_SCALER / 100;
  return corrected;
}

uint16_t telem_counter = 0;
uint8_t latest_packet[TRANCEIVER_MAX_PACKET_BYTES] = {0};
packet_stats latest_packet_stats;


// the loop function runs over and over again forever
void loop() {
  telem_counter += 1;
  if (telem_counter > 10000){
    telem_counter = 0;
  }

  if (telem_counter % 100 == 50){
    tranceiver_send_name_packet(name, sizeof(name));
  }

  tranceiver_get_latest_packet(latest_packet, &latest_packet_stats);
  if (latest_packet_stats.packet_len != 0){
    if (latest_packet_stats.packet_type == PACKET_CONTROL){
      float channels[6] = {0};
      for (uint8_t i=0; i<6; i++){
        int16_t raw = 0;
        raw = latest_packet[i*2];
        raw += latest_packet[i*2 + 1] << 8;
        channels[i] = float(raw) / (32767.0);
      }
      handle_channels(channels, 6);
    }
    
    telem_rssi.value = latest_packet_stats.rssi;
    telem_rssi.status = status_from_value_lesser(telem_rssi.value, -70, -90);
  }
  
  telem_batt_voltage.value = getBatteryMillVolts() / 1000.0;
  telem_batt_voltage.status = status_from_value_lesser(telem_batt_voltage.value, 3.3, 2.7);
  update_telemetry();
  
  // Ensure the other tasks on the 8266 have time to run
  delay(10);  
}
