#include "telemetry.h"
#include "Arduino.h"


TelemChannel* channels[TELEMETRY_MAX_CHANNELS];
uint8_t num_channels = 0;
uint8_t current_channel = 0;
uint16_t time_last_sent = 0;

int8_t register_telem(TelemChannel* channel){
  if (num_channels == TELEMETRY_MAX_CHANNELS){
    Serial.print("Max number telemetry channels reached");
    return -1;
  }
  channels[num_channels] = channel;
  num_channels += 1;
  return 0;
}


void update_telemetry(){
  uint16_t cur_time = millis();
  if (time_last_sent < uint16_t(cur_time - TELEMETRY_MS_BETWEEN_MESSAGES)){
    time_last_sent = cur_time;
    
    if (channels[current_channel] == NULL){
      current_channel = 0;
      return;
    }
    TelemChannel* cur_channel = channels[current_channel];
    
    Serial.print(cur_channel->status);
    Serial.print(" | ");
    Serial.print(cur_channel->name);
    Serial.print(" : ");
    Serial.println(cur_channel->value);
    
    tranceiver_send_telemtry(
      cur_channel->status, 
      cur_channel->value,
      cur_channel->name,
      strlen(cur_channel->name)
    );
    current_channel += 1;
  }
};


telemetry_status status_from_value_greater(float value, float warn, float error){
  if (value > error){
    return TELEMETRY_ERROR;
  }
  if (value > warn){
    return TELEMETRY_WARN;
  }
  return TELEMETRY_OK;
}

telemetry_status status_from_value_lesser(float value, float warn, float error){
  if (value < error){
    return TELEMETRY_ERROR;
  }
  if (value < warn){
    return TELEMETRY_WARN;
  }
  return TELEMETRY_OK;
}
