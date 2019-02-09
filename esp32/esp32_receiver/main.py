import gc
import time
import hardware
import radio
import struct


TELEMETRY_RSSI_WARN = -80
TELEMETRY_RSSI_ERROR = -90

TELEMETRY_PACKET_LOSS_WARN = 0.3  # Warn if 30% of packets are lost
TELEMETRY_PACKET_LOSS_ERROR = 0.8  # display error if 80% of telemetry packets are lost


class Receiver:
    def __init__(self, loop_hz):
        self.drive = hardware.Drive()
        radio.init()
        self._loop_us = 1000 / loop_hz

        self.telemetry_manager = TelemetryManager("Crawler", 100)


    def loop(self):
        packet_data, packet_stats = radio.get_latest_packet()
        if packet_stats[3] > 0:
            # Got a packet
            if packet_stats[1] == radio.PACKET_CONTROL:
                values = struct.unpack('h'*int(len(packet_data)/2), packet_data)
                f_values = [v/((2**16)/2) for v in values]
                self.drive.set_targets(f_values[1], f_values[0])

        self.telemetry_manager.update()
        self.drive.update()


    def update(self):
        start_time = time.ticks_us()
        self.loop()
        gc.collect()
        end_time = time.ticks_us()
        sleep_time = self._loop_us - (end_time - start_time)
        sleep_time = max(0, sleep_time)  # Catch wrap-around

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



class TelemetryManager:
    def __init__(self, name, ms_between_sends):
        self.name = name
        self.ms_between = ms_between_sends

        self._prev_time = 0
        self.telemetries = []
        self.telemetry_pointer = 1
        self.send_next()

    def update(self):
        if time.ticks_ms() > self._prev_time + self.ms_between:
            self.send_next()

    def send_next(self):
        self._prev_time = time.ticks_ms()
        if self.telemetry_pointer > len(self.telemetries):
            radio.send_name_packet(self.name)
            self.telemetry_pointer = 0
        else:
            print("Sending telemetry not implemented")
            self.telemetry_pointer += 1


def start():
    c = Receiver(30)
    while(1):
        c.update()


if __name__ == "__main__":
    start()
