import gc
import time
import hardware
import radio
import struct


TELEMETRY_RSSI_WARN = -80
TELEMETRY_RSSI_ERROR = -90

TELEMETRY_PACKET_LOSS_WARN = 0.3  # Warn if 30% of packets are lost
TELEMETRY_PACKET_LOSS_ERROR = 0.8  # display error if 80% of telemetry packets are lost


class PacketLossCounter:
    def __init__(self):
        self._prev_packet_id = 0
        self.percent_got = 1.0
        self._last_packet_time = time.ticks_ms()

    def submit_packet_id(self, val):
        diff = val - self._prev_packet_id
        if diff != 0:
            if diff < 0:
                diff += 2 ** 8
            self._prev_packet_id = val

            self._last_packet_time = time.ticks_ms()

            lost_percent = 1 / diff
            self.percent_got = self.percent_got * 0.7 + lost_percent * 0.3
        else:
            """Same packet again packet or the number wrapped completely"""

    @property
    def percent_loss(self):
        return 1 - self.percent_got

    def update(self):
        if time.ticks_ms() > self._last_packet_time + 500:
            if self.percent_got > 0.01:
                self.percent_got -= 0.01




class Controller:
    def __init__(self, loop_hz):
        self._loop_hz = loop_hz
        self._loop_us = 1e6 / loop_hz
        self._average_cpu = 1.0

        self.inputs = hardware.Inputs()
        self.display = hardware.Display()
        radio.init()

        self._connected = False
        self._connected_id = None

        self.packet_counter = PacketLossCounter()

        self._loop_counter = loop_hz


    def loop(self):
        self.display.update()
        self.update_system_stats()

        if self._connected:
            self._send_control()
            self._update_telemetry()
        else:
            self._find_rx()

        self.packet_counter.update()


    def _find_rx(self):
        """Listens for the first receiver to broadcast a name packet"""
        packet_data, packet_stats = radio.get_latest_packet()
        if packet_stats[3] > 0:
            # Got a packet
            if packet_stats[1] == radio.PACKET_NAME:
                self.display.show_internal_value("Device Name", packet_data.decode('utf-8'), radio.TELEMETRY_OK)
                self.display.show_internal_value("Device Id", packet_stats[0], radio.TELEMETRY_OK)
                radio.set_id(packet_stats[0])
                self._connected = True
                self._connected_id = packet_stats[0]
                self.display.set_radio_state(self._connected)
        else:
            self.display.show_internal_value("Device Name", "Not Connected", radio.TELEMETRY_ERROR)
            self.display.show_internal_value("Device Id", "Not Connected", radio.TELEMETRY_ERROR)



    def _send_control(self):
        """Sends control packets"""
        channels = [
            self.inputs.get_analog_input(self.inputs.ANALOG_CHANNEL_STICK_RIGHT_X),
            self.inputs.get_analog_input(self.inputs.ANALOG_CHANNEL_STICK_RIGHT_Y),
            self.inputs.get_analog_input(self.inputs.ANALOG_CHANNEL_STICK_LEFT_X),
            self.inputs.get_analog_input(self.inputs.ANALOG_CHANNEL_STICK_LEFT_Y),
        ]
        for chan_id, chan in enumerate(channels):
            channels[chan_id] = int(chan * ((2 ** 15) - 1))
        sent = radio.send_8266_control_packet(
            channels
        )
        if sent != 0:
            print("SEND PACKET FAILED")

    def _update_telemetry(self):
        """Updates the telemetry from the remote device"""
        packet_data, packet_stats = radio.get_latest_packet()
        if packet_stats[3] > 0:
            # Got a packet
            if packet_stats[0] == self._connected_id:
                if packet_stats[1] == radio.PACKET_TELEMETRY:
                    status = packet_data[0]
                    value = struct.unpack('f', packet_data[1:5])[0]
                    name = packet_data[5:].decode('utf-8')
                    self.display.show_external_value(name, value, status)


                # All packets have a packet ID etc.
                rssid = packet_stats[2]
                self.display.show_internal_value(
                    "RSSID", rssid,
                    format_telemetry_lesser(rssid, TELEMETRY_RSSI_WARN, TELEMETRY_RSSI_ERROR)
                )

                self.packet_counter.submit_packet_id(packet_stats[4])



    def update_system_stats(self):
        self._loop_counter += 1
        if self._loop_counter >= self._loop_hz:
            voltage = self.inputs.get_battery_volts()
            self.display.show_internal_value("Battery Voltage", voltage, radio.TELEMETRY_OK)
            self.display.show_internal_value("Uptime", time.ticks_ms() / 1000, radio.TELEMETRY_OK)
            self.display.show_internal_value("Average CPU", 100 - int(self._average_cpu * 100), radio.TELEMETRY_OK)

            self.display.show_internal_value(
                "Packet Loss", self.packet_counter.percent_loss * 100,
                format_telemetry_greater(self.packet_counter.percent_loss, TELEMETRY_PACKET_LOSS_WARN, TELEMETRY_PACKET_LOSS_ERROR)
            )
            self._loop_counter = 0



    def update(self):
        start_time = time.ticks_us()
        self.loop()
        gc.collect()
        end_time = time.ticks_us()
        sleep_time = self._loop_us - (end_time - start_time)
        sleep_time = max(0, sleep_time)  # Catch wrap-around
        cpu_percent = sleep_time / self._loop_us
        self._average_cpu = cpu_percent * 0.1 + self._average_cpu * 0.9

        if sleep_time > 0:
            self._loop_wait(sleep_time)


    def _loop_wait(self, us):
        """Function that runs when nothing to do. Sleeps for the specific number
        of microseconds"""
        time.sleep_us(int(us))

        # Busy wait so that the radio stays active. I hope to find a better way
        #start_time = time.ticks_us()
        #while time.ticks_us() - start_time < us:
        #    pass


def format_telemetry_lesser(value, warn_threshold, error_threshold):
    if value < error_threshold:
        return radio.TELEMETRY_ERROR
    elif value < warn_threshold:
        return radio.TELEMETRY_WARN
    return radio.TELEMETRY_OK

def format_telemetry_greater(value, warn_threshold, error_threshold):
    if value > error_threshold:
        return radio.TELEMETRY_ERROR
    elif value > warn_threshold:
        return radio.TELEMETRY_WARN
    return radio.TELEMETRY_OK


def start():
    c = Controller(30)
    while(1):
        c.update()


if __name__ == "__main__":
    start()
