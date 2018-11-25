#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include "tranceiver.h"

#define TELEMETRY_MAX_CHANNELS 10
#define TELEMETRY_MS_BETWEEN_MESSAGES 200

typedef struct {
  const char* name;
  telemetry_status status; 
  float value;
} TelemChannel;

/* 
 *  Registers a telemetry channel as something to be sent
 */
int8_t register_telem(TelemChannel* channel);

/* Cycles through the telemetry channels in turn, sending one each
 *  TELEMETRY_MS_BETWEEN_MESSAGES ms appart.
 */
void update_telemetry();

/* 
 *  Derive a status from thresholds
 */
telemetry_status status_from_value_greater(float value, float warn, float error);
telemetry_status status_from_value_lesser(float value, float warn, float error);

#endif
