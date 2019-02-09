#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_internal.h"
#include "esp_event_loop.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "lwip/err.h"

#include "tranceiver.h"


/* Parameters for the transmitter */
#define DEFAULT_WIFI_CHANNEL 1
#define DEFAULT_TRANSMIT_POWER 8 //2dbm = 1.5mW

// The data that gets sent between transmitter and reciever. This must match
// the one in reciever.c


static uint8_t filter_by_id = 1;
uint8_t last_sent_packet_count = 0;
static QueueHandle_t rx_packet_queue;


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
#define ID_LENGTH 6
#define PACKET_COUNT_OFFSET 22
#define PACKET_TYPE_OFFSET 23
#define DATA_1_OFFSET 10


static uint8_t tx_packet_buffer[sizeof(packet_header) + TRANCEIVER_MAX_PACKET_BYTES] = {0};
uint8_t rx_packet_buffer[sizeof(packet_stats) + TRANCEIVER_MAX_PACKET_BYTES] = {0};
uint8_t rx_packet_out_buff[sizeof(packet_stats) + TRANCEIVER_MAX_PACKET_BYTES] = {0};


uint16_t min_16(uint16_t a, uint16_t b){
    if (a < b){
        return a;
    }
    return b;
}

static void print_buffer(const uint8_t* buff, uint16_t len){
  uint16_t i=0;
  for (i=0; i<len; i++){
    printf(" %x", buff[i]);
  }
  printf("\n");
}


void tranceiver_set_channel(uint8_t channel){
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}


void tranceiver_set_power(int8_t power){
	esp_wifi_set_max_tx_power(power);
}


void tranceiver_set_id(const uint8_t id_bytes[6]){
    uint8_t i = 0;
    for (i=0; i<12; i+=1){
        packet_header[ID_OFFSET + i] = id_bytes[i%6];
    }
}

void tranceiver_enable_filter_by_id(uint8_t enabled){
  filter_by_id = enabled;
}


static void _handle_data_packet(void* buff, wifi_promiscuous_pkt_type_t type) {
	/* Runs whenever there is an incoming packet */
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

    //  Ensure the first two mac addresses are the same. This means it is not
    // and 802.11 packet, but is one of ours.

    /* Check that the ID matches what we expect */
	if (filter_by_id){
		if (memcmp(packet_header+ID_OFFSET, (ppkt->payload)+ID_OFFSET, ID_LENGTH) != 0) {
			return;
		}
    } else {
        if (memcmp((ppkt->payload)+ID_OFFSET, ((ppkt->payload)+ID_OFFSET+ID_LENGTH), ID_LENGTH) != 0){
            // Reject non-name packets
            return;
        }
    }

	uint16_t data_len = ppkt->rx_ctrl.sig_len - sizeof(packet_header) - 4 + 12;  //magic numbers is 4=crc bytes 12=data bytes in header
	if (data_len > TRANCEIVER_MAX_PACKET_BYTES){
		return;
	}

	// Make metadata and data continuous in memory
    packet_stats* this_packet = (packet_stats*)&rx_packet_buffer;
    this_packet->packet_type = PACKET_NONE;
	this_packet->rssi = ppkt->rx_ctrl.rssi;
	this_packet->noise_floor = ppkt->rx_ctrl.noise_floor;
	this_packet->packet_len = data_len;
    this_packet->packet_id = ppkt->payload[PACKET_COUNT_OFFSET];
    this_packet->packet_type = (uint8_t)ppkt->payload[PACKET_TYPE_OFFSET];
    memcpy(
        this_packet->source_id,
        (ppkt->payload)+ID_OFFSET,
        6
    );

    // The first 12 bytes are part of the header
    memcpy(
		rx_packet_buffer + sizeof(packet_stats),
		(ppkt->payload)+DATA_1_OFFSET,
		12
	);

    // The rest of the data is after the header
	memcpy(
		rx_packet_buffer + sizeof(packet_stats) + 12,
		(ppkt->payload)+sizeof(packet_header),
		data_len
	);
    if (rx_packet_queue != NULL){
        xQueueSend(
            rx_packet_queue,
            rx_packet_buffer,
            0
        );
    }
}

void tranceiver_get_latest_packet(uint8_t buff[], packet_stats* stats){
    if (xQueueReceive(rx_packet_queue, &rx_packet_out_buff, 0) == pdTRUE){
        packet_stats* this_packet = (packet_stats*)&rx_packet_buffer;
        memcpy(
            stats,
            this_packet,
            sizeof(packet_stats)
        );

        memcpy(
            buff,
            rx_packet_buffer + sizeof(packet_stats),
            min_16(this_packet->packet_len, TRANCEIVER_MAX_PACKET_BYTES)
        );
    } else {
        stats->packet_len = 0;
    }
}


void tranceiver_init(void){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.dynamic_tx_buf_num = 16;
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );

    rx_packet_queue = xQueueCreate(2, sizeof(rx_packet_buffer));

	//Set up a callback to be called whenever packets arrive.
	esp_wifi_set_promiscuous(true);
	wifi_promiscuous_filter_t filter;
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;  // Only data packets
	esp_wifi_set_promiscuous_filter(&filter);
	esp_wifi_set_promiscuous_rx_cb(&_handle_data_packet);


	tranceiver_set_channel(DEFAULT_WIFI_CHANNEL);
	tranceiver_set_power(DEFAULT_TRANSMIT_POWER);
}



static uint8_t tranceiver_send_packet(const packet_types packet_type, const uint8_t data[], const uint16_t data_len){
    // Copy in header
	memcpy(&tx_packet_buffer, &packet_header, sizeof(packet_header));

    // Copy in data
    uint16_t i = 0;
    uint16_t extra_bytes = 0; // Number bytes greater than the packet size
    for (i=0; i < data_len; i++){
        if (i < 12){ //First 12 bytes go into header
            tx_packet_buffer[DATA_1_OFFSET + i] = data[i];
        } else {  //The remaining data goes at teh end
            tx_packet_buffer[sizeof(packet_header) + (i - 12)] = data[i];
            extra_bytes += 1;
        }
    }

    // Set metadata
    tx_packet_buffer[PACKET_COUNT_OFFSET] = last_sent_packet_count;
    last_sent_packet_count += 1;
    tx_packet_buffer[PACKET_TYPE_OFFSET] = (uint8_t)packet_type;

    //print_buffer(tx_packet_buffer, sizeof(packet_header) + extra_bytes);

    return esp_wifi_80211_tx(
		ESP_IF_WIFI_STA,
		(void*)&tx_packet_buffer, sizeof(packet_header) + extra_bytes,
		false
	);
}


uint8_t telemetry_buffer[sizeof(telemetry_packet)] = {0};
uint8_t tranceiver_send_telemtry(const telemetry_status status, const float value, const char name[], const uint8_t name_len){
    telemetry_buffer[0] = status;
    memcpy(telemetry_buffer+1, (uint8_t*)&value, 4);
    memcpy(telemetry_buffer+5, name, min_16(name_len, TRANCEIVER_MAX_NAME_LENGTH));
    uint16_t total_size = min_16(name_len, TRANCEIVER_MAX_NAME_LENGTH) + sizeof(value) + 1;  // the 1 is the status
    return tranceiver_send_packet(PACKET_TELEMETRY, (uint8_t*)&telemetry_buffer, total_size);
}


uint8_t tranceiver_send_name_packet(const uint8_t name[], uint8_t len){
    if (len > TRANCEIVER_MAX_NAME_LENGTH){
        printf("Name too long\n");
        len = min_16(len, TRANCEIVER_MAX_NAME_LENGTH);
    }
    uint8_t concatenated[TRANCEIVER_MAX_NAME_LENGTH + 6] = {0x00};
    memcpy(concatenated, packet_header+ID_OFFSET, 6); // Copy in ID
    memcpy(concatenated + 6, name, len);
    return tranceiver_send_packet(PACKET_NAME, concatenated, len+6);
}


uint8_t tranceiver_send_control_packet(int16_t channel_values[], uint8_t num_channels){
    return tranceiver_send_packet(PACKET_CONTROL, (uint8_t*)channel_values, num_channels*2);
}
