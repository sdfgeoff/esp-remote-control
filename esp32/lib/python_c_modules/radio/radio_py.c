#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "tranceiver.h"

/* This python module exposes all functions that talk to the actuators and
 * sensors on board the robot */

STATIC mp_obj_t radio_init(void) {
    tranceiver_init();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(radio_init_obj, radio_init);


STATIC mp_obj_t radio_get_latest_packet(void) {
    packet_stats packet_data;
    uint8_t latest_packet_raw[TRANCEIVER_MAX_PACKET_BYTES] = {0};
    tranceiver_get_latest_packet(latest_packet_raw, &packet_data);

    //Assemble data into bytestring
    vstr_t in_data;
    vstr_init_len(&in_data, packet_data.packet_len);
    for (int i=0; i<packet_data.packet_len; i++){
        in_data.buf[i] = latest_packet_raw[i];
    }

    //Assemble source id into tuple
    mp_obj_t source_id[6];
    for (int i=0; i<6; i++){
        source_id[i] = mp_obj_new_int(packet_data.source_id[i]);
    }

    mp_obj_t packet_stats_array[5];
    packet_stats_array[0] = mp_obj_new_tuple(6, source_id);
    packet_stats_array[1] = mp_obj_new_int(packet_data.packet_type);
    packet_stats_array[2] = mp_obj_new_int(packet_data.rssi);
    packet_stats_array[3] = mp_obj_new_int(packet_data.packet_len);
    packet_stats_array[4] = mp_obj_new_int(packet_data.packet_id);

    mp_obj_t output[2];
    output[0] = mp_obj_new_str_from_vstr(&mp_type_bytes, &in_data);
    output[1] = mp_obj_new_tuple(5, packet_stats_array);

    return mp_obj_new_tuple(2, output);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(radio_get_latest_packet_obj, radio_get_latest_packet);


STATIC mp_obj_t radio_filter_by_id(mp_obj_t enabled) {
    tranceiver_enable_filter_by_id(mp_obj_get_int(enabled));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(radio_filter_by_id_obj, radio_filter_by_id);


STATIC mp_obj_t radio_set_channel(mp_obj_t channel) {
    tranceiver_set_channel(mp_obj_get_int(channel));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(radio_set_channel_obj, radio_set_channel);

STATIC mp_obj_t radio_set_power(mp_obj_t power) {
    tranceiver_set_power(mp_obj_get_int(power));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(radio_set_power_obj, radio_set_power);


STATIC mp_obj_t radio_set_id(mp_obj_t id_bytes) {
    mp_obj_t* id_py;
    mp_obj_get_array_fixed_n(id_bytes, 6, &id_py);

    uint8_t id[6] = {0};
    for (uint8_t i=0; i<sizeof(id); i++){
        id[i] = mp_obj_get_int(id_py[i]);
    }

    tranceiver_set_id(id);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(radio_set_id_obj, radio_set_id);

STATIC mp_obj_t radio_send_control_packet(mp_obj_t channels) {
    mp_obj_t* channel_values_py;
    size_t num_channels = 0;
    mp_obj_get_array(channels, &num_channels, &channel_values_py);
    if (num_channels > 32){
        num_channels = 32;
        printf("Python bindings are only able to send first 32 channels currently\n");
    }

    int16_t channel_values[32] = {0};
    for (uint16_t i=0; i<num_channels; i++){
        channel_values[i] = mp_obj_get_int(channel_values_py[i]);
    }

    int16_t res = tranceiver_send_control_packet(channel_values, num_channels);

    return mp_obj_new_int(res);
}
MP_DEFINE_CONST_FUN_OBJ_1(radio_send_control_packet_obj, radio_send_control_packet);


STATIC const mp_map_elem_t radio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_radio) },
    // Functions
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&radio_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_filter_by_id), (mp_obj_t)&radio_filter_by_id_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_latest_packet), (mp_obj_t)&radio_get_latest_packet_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_channel), (mp_obj_t)&radio_set_channel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_power), (mp_obj_t)&radio_set_power },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_id), (mp_obj_t)&radio_set_id_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_send_control_packet), (mp_obj_t)&radio_send_control_packet_obj },

    // Constants
    { MP_ROM_QSTR(MP_QSTR_TELEMETRY_OK), MP_ROM_INT(TELEMETRY_OK) },
    { MP_ROM_QSTR(MP_QSTR_TELEMETRY_WARN), MP_ROM_INT(TELEMETRY_WARN) },
    { MP_ROM_QSTR(MP_QSTR_TELEMETRY_ERROR), MP_ROM_INT(TELEMETRY_ERROR) },
    { MP_ROM_QSTR(MP_QSTR_TELEMETRY_UNDEFINED), MP_ROM_INT(TELEMETRY_UNDEFINED) },

    { MP_ROM_QSTR(MP_QSTR_PACKET_NONE), MP_ROM_INT(PACKET_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PACKET_CONTROL), MP_ROM_INT(PACKET_CONTROL) },
    { MP_ROM_QSTR(MP_QSTR_PACKET_NAME), MP_ROM_INT(PACKET_NAME) },
    { MP_ROM_QSTR(MP_QSTR_PACKET_TELEMETRY), MP_ROM_INT(PACKET_TELEMETRY) },

    { MP_ROM_QSTR(MP_QSTR_CHANNEL_VALUE_UNDEFINED), MP_ROM_INT(CHANNEL_VALUE_UNDEFINED) },
};


STATIC MP_DEFINE_CONST_DICT (
    mp_module_radio_globals,
    radio_globals_table
);

const mp_obj_module_t radio_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_radio_globals,
};

