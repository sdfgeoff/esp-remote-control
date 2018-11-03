#include <stdio.h>
#include <Arduino.h>
#include <string.h>
extern "C" {
  #include <user_interface.h>
}

#include "tranceiver.h"
#include <stdlib.h>

/* Parameters for the transmitter */
#define DEFAULT_WIFI_CHANNEL 1
#define DEFAULT_TRANSMIT_POWER 8 //2dbm = 1.5mW

uint8_t last_sent_packet_count = 0;
uint8_t laset_received_packet_id = 0;

uint8_t packet_header[] = {
	0x08, 0x00, // Data packet (normal subtype)
	0x00, 0x00, // Duration and ID
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Recipient Address
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Transmitter Address
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID Address
	0x00, 0x00, // Sequence Control
	0x00, 0x00, // QOS control

	/* Data gets substituted here */
	/* Then the ESP automatically adds the checksum for us */
};

#define ID_OFFSET 4
#define PACKET_COUNT_OFFSET 16
#define PACKET_TYPE_OFFSET 21


static uint8_t tx_packet_buffer[sizeof(packet_header) + TRANCEIVER_MAX_PACKET_BYTES] = {0};

/* ESp Gubbage. I can't find what file this should be #included from. Everyone online just
 * has this in the file directly....
 * 
 * Copied from https://github.com/kalanda/esp8266-sniffer/blob/master/src/main.ino
 */
#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04

struct RxControl {
  signed rssi:8; // signal intensity of packet
  unsigned rate:4;
  unsigned is_group:1;
  unsigned:1;
  unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
  unsigned legacy_length:12; // if not 11n packet, shows length of packet.
  unsigned damatch0:1;
  unsigned damatch1:1;
  unsigned bssidmatch0:1;
  unsigned bssidmatch1:1;
  unsigned MCS:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
  unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not
  unsigned HT_length:16;// if is 11n packet, shows length of packet.
  unsigned Smoothing:1;
  unsigned Not_Sounding:1;
  unsigned:1;
  unsigned Aggregation:1;
  unsigned STBC:2;
  unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not.
  unsigned SGI:1;
  unsigned rxend_state:8;
  unsigned ampdu_cnt:8;
  unsigned channel:4; //which channel this packet in.
  unsigned:12;
};

struct LenSeq{
  u16 len; // length of packet
  u16 seq; // serial number of packet, the high 12bits are serial number,
  // low 14 bits are Fragment number (usually be 0)
  u8 addr3[6]; // the third address in packet
}; 

struct sniffer_buf{
  struct RxControl rx_ctrl;
  u8 buf[36]; // head of ieee80211 packet
  u16 cnt; // number count of packet
  struct LenSeq lenseq[1]; //length of packet 
};

struct sniffer_buf2{
  struct RxControl rx_ctrl;
  u8 buf[112]; //may be 240, please refer to the real source code
  u16 cnt;
  u16 len; //length of packet
}; 

/* 
 *  Actual code.....
 */



void tranceiver_set_channel(uint8_t channel){
  Serial.print("Set channel to: ");
  Serial.println(channel);
	wifi_set_channel(channel);
}


void tranceiver_set_power(int8_t power){
	system_phy_set_max_tpw(power);
}


static void print_buffer(uint8_t* buff, uint16_t len){
  uint16_t i=0;
  for (i=0; i<len; i++){
    Serial.print(" ");
    Serial.print(buff[i], HEX);
  }
  Serial.println();
}

void tranceiver_set_id(const uint8_t id_bytes[6]){
  uint8_t i = 0;
  for (i=0; i<12; i+=1){
    packet_header[ID_OFFSET + i] = id_bytes[i%6];
  }
  Serial.println("Set header to: ");
  print_buffer(packet_header, sizeof(packet_header));
}


static void _handle_data_packet(uint8_t* buffer, uint16_t len) {
	/* Runs whenever there is an incoming packet */
  if (len == sizeof(sniffer_buf2)){
    // Management Packet
    return;
  } else if (len == sizeof(RxControl)){
    // Indesipherable packet (unsupported packet type)
    return;
  } else if (len % 10 != 0){
    Serial.println("Unknown Packet Duration?");
    return;
  }
  
	
	struct sniffer_buf *snifferPacket = (struct sniffer_buf*) buffer;

  //The ESP8266 doesn't provide all the data, maxing out with the first 36 bytes
  uint16_t actual_length = snifferPacket->lenseq[0].len; 
  uint16_t provided_length = min(actual_length, 36);

  //Serial.println("Got Packet");
  //print_buffer(snifferPacket->buf, provided_length);
  //Check that the id of the incoming packet matches and that it is a data packet etc...
	if (memcmp(packet_header, snifferPacket->buf, sizeof(packet_header)) != 0) {
    return;
	}

  Serial.println("Got Correct Packet");
}


void tranceiver_init(void){
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(DEFAULT_WIFI_CHANNEL);
  wifi_promiscuous_enable(0);
  delay(10);
  wifi_set_promiscuous_rx_cb(_handle_data_packet);
  delay(10);
  wifi_promiscuous_enable(1);

  uint8_t mac[6] = {0};
  wifi_get_macaddr(STATION_IF, mac);
  tranceiver_set_id(mac);
  tranceiver_set_channel(DEFAULT_WIFI_CHANNEL);
  tranceiver_set_power(DEFAULT_TRANSMIT_POWER);
}


// Send a raw packet
static uint8_t tranceiver_send_packet(const packet_types packet_type, const uint8_t data[], const uint16_t data_len){
	if (data_len > TRANCEIVER_MAX_PACKET_BYTES){
		return 1;
	}

  // Copy in header
	memcpy(&tx_packet_buffer, &packet_header, sizeof(packet_header)); 

  // Set metadata
  tx_packet_buffer[PACKET_COUNT_OFFSET] = last_sent_packet_count;
  last_sent_packet_count += 1;
  tx_packet_buffer[PACKET_TYPE_OFFSET] = (uint8_t)packet_type;

  // Copy in data
  uint16_t i = 0;
  for (i=0; i<data_len; i++){
    tx_packet_buffer[sizeof(packet_header) + i] = data[i];
  }
	//memcpy((&tx_packet_buffer)+sizeof(packet_header), data, data_len);

  //Send it
  Serial.print("Sending: ");
  print_buffer(tx_packet_buffer, sizeof(packet_header) + data_len);
  
	wifi_send_pkt_freedom(
		(uint8_t*)&tx_packet_buffer, sizeof(packet_header) + data_len,
		false
	);

	return 0;
}

uint8_t telemetry_buffer[sizeof(telemetry_packet)] = {0};
uint8_t tranceiver_send_telemtry(const telemetry_status status, const float value, const char name[], const uint8_t name_len){
  telemetry_buffer[0] = status;
  memcpy(telemetry_buffer+1, (uint8_t*)&value, 4);
  memcpy(telemetry_buffer+5, name, min(name_len, TRANCEIVER_MAX_NAME_LENGTH));
  uint16_t total_size = min(name_len, TRANCEIVER_MAX_NAME_LENGTH) + sizeof(value) + 1;  // the 1 is the status
  return tranceiver_send_packet(PACKET_TELEMETRY, (uint8_t*)&telemetry_buffer, total_size);
}


uint8_t tranceiver_send_control_packet(int16_t channel_values[], uint8_t num_channels){
  return tranceiver_send_packet(PACKET_CONTROL, (uint8_t*)channel_values, num_channels*2);
}


uint8_t tranceiver_send_name_packet(const uint8_t name[], uint8_t len){
  return tranceiver_send_packet(PACKET_NAME, name, len);
}
