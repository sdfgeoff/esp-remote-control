#ifndef __tranciever_h__
#define __tranciever_h__


//This module handles injecting and sniffing packets. It implements the

#define TRANCEIVER_MAX_PACKET_BYTES 64
#define TRANCEIVER_MAX_NAME_LENGTH 16
#define CHANNEL_VALUE_UNDEFINED -32768


typedef enum {
  TELEMETRY_OK = 0,
  TELEMETRY_WARN = 1,
  TELEMETRY_ERROR = 2,
  TELEMETRY_UNDEFINED = 255
} telemetry_status;


typedef enum {
  PACKET_NONE = 0x00,
  PACKET_CONTROL = 0x01,
  PACKET_TELEMETRY = 0x02,
  PACKET_NAME = 0x03,
} packet_types;


typedef struct {
  int8_t rssi;
  uint8_t source_id[6];
  uint8_t packet_id;
  uint8_t packet_len;
  packet_types packet_type;

  int8_t noise_floor;
} packet_stats;

typedef struct {
  telemetry_status status;
  float value;
  char name[TRANCEIVER_MAX_NAME_LENGTH];
} telemetry_packet;


/* Start the tranceiver */
void tranceiver_init();

/*
 * Returns the latest packet and the metadata about it
 */
void tranceiver_get_latest_packet(uint8_t buff[], packet_stats* stats);

/*
 * Send the specified
 * Returns nonzero if not sent
*/
uint8_t tranceiver_send_telemtry(const telemetry_status status, const float value, const char name[], const uint8_t name_len);

/*
 * Sends a control packet with the specified channels
 * Returns nonzero if not sent
 */
uint8_t tranceiver_send_control_packet(int16_t channel_values[], uint8_t num_channels);
uint8_t tranceiver_send_8266_control_packet(int16_t channel_values[], uint8_t num_channels);

/*
 * Broadcasts this devices name to the world
 */
uint8_t tranceiver_send_name_packet(const uint8_t name[], uint8_t len);


/* Sets the transmit power. Lower numbers are more power. It appears that
 * the number provided here is about equal to the power in dbm/4. Eg:
 *  - set_transmit_power(78) will transmit at about 19.5dbm (90mW)
 *  - set_transmit_power(44) will transmit at about 11dbm (12.6mW)
 *
 * There is some granularity, so not all values are unique. For more info,
 * see the ESP32 docs on esp_wifi_set_max_tx_power
 *
 * Check your local regulations for the maximum permissible.
 */
void tranceiver_set_power(int8_t transmit_power);


/* Sets the wifi transmission frequency. Check your countries regulations.
 * (most countries allow use of 1 - 11. Some countries restrict the use of
 * 12-14).
 */
void tranceiver_set_channel(uint8_t channel);


/*
 * To differentiate the transceiver packets from normal wifi traffic, the
 * packet is sent with an ID. This ID should be 6 bytes long.
 */
void tranceiver_set_id(const uint8_t id_bytes[6]);

/*
 *  Only packets that match the trancievers ID are receivable with the
 * get_latest_packet function
 */
void tranceiver_enable_filter_by_id(uint8_t enabled);

#endif
