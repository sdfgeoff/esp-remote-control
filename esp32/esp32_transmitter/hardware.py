import machine
import radio
import time


BATT_VOLTAGE_CALIB = 0.0016767922235722964

class Inputs:
    ANALOG_CHANNEL_STICK_RIGHT_Y = 0
    ANALOG_CHANNEL_STICK_RIGHT_X = 1
    ANALOG_CHANNEL_STICK_LEFT_Y = 2
    ANALOG_CHANNEL_STICK_LEFT_X = 3

    DIGITAL_CHANNEL_STICK_LEFT = 0
    DIGITAL_CHANNEL_STICK_RIGHT = 1
    def __init__(self):
        self._batt_adc = machine.ADC(machine.Pin(39))
        self._batt_adc.atten(machine.ADC.ATTN_11DB)

        self.analog_inputs = [
            machine.ADC(machine.Pin(32)),
            machine.ADC(machine.Pin(33)),
            machine.ADC(machine.Pin(34)),
            machine.ADC(machine.Pin(35)),
        ]

        self.digital_inputs = [
            machine.Pin(25),
            machine.Pin(26),
        ]

        for ana_in in self.analog_inputs:
            ana_in.atten(ana_in.ATTN_11DB)


    def get_battery_volts(self):
        """Returns the battery voltage"""
        raw = self._batt_adc.read()
        return raw * BATT_VOLTAGE_CALIB


    def update(self):
        pass

    def get_analog_input(self, channel):
        """Returns the analog input as a percentage that is centered in the
        middle of the ADC's range"""
        raw = self.analog_inputs[channel].read()
        return (2048 - raw) / 2048


class Display():
    def __init__(self):
        self._led = RGBLed(13, 12, 14)

        self._internal_table = {}
        self._external_table = {}
        self._radio_state = False

        self._last_refresh_time = 0

    def show_internal_value(self, name, value, status):
        self._internal_table[name] = (status, value)

    def show_external_value(self, name, value, status):
        self._external_table[name] = (status, value)

    def redraw(self):
        """Redraw the display"""
        max_state = 0
        print()
        print("Internal:\n---------")
        for name in self._internal_table:
            status, value = self._internal_table[name]
            print("{} | {} : {}".format(status, name, value))
            if status > max_state:
                max_state = status

        if self._radio_state:
            print("External:\n---------")
            for name in self._external_table:
                status, value = self._external_table[name]
                print("{} | {} : {}".format(status, name, value))
                if status > max_state and status != radio.TELEMETRY_UNDEFINED:
                    max_state = status

            self._led.show_warn_level(max_state)
        else:
            self._led.show_not_connected()

    def update(self):
        cur_time = time.ticks_ms()
        if cur_time > self._last_refresh_time + 1000:
            self._last_refresh_time = cur_time
            self.redraw()


    def set_radio_state(self, connected):
        """Is the controller connected/transmitting or not?"""
        self._external_table = {}
        self._radio_state = connected


class RGBLed:
    ON = 0
    OFF = 1
    FLASHING = 2
    def __init__(self, r_pin, g_pin, b_pin):
        self._r_pin = machine.PWM(machine.Pin(r_pin))
        self._g_pin = machine.PWM(machine.Pin(g_pin))
        self._b_pin = machine.PWM(machine.Pin(b_pin))

        self._r_pin.freq(2)
        self._g_pin.freq(2)
        self._b_pin.freq(2)

        self._r_pin.duty(self.OFF)
        self._g_pin.duty(self.OFF)
        self._b_pin.duty(self.ON)


    def show_not_connected(self):
        self._set_state(self._r_pin, self.OFF)
        self._set_state(self._b_pin, self.FLASHING)
        self._set_state(self._g_pin, self.OFF)

    def show_warn_level(self, warn_level):
        if warn_level == radio.TELEMETRY_OK:
            self._set_state(self._r_pin, self.OFF)
            self._set_state(self._g_pin, self.ON)
            self._set_state(self._b_pin, self.OFF)
        elif warn_level == radio.TELEMETRY_WARN:
            self._set_state(self._r_pin, self.ON)
            self._set_state(self._g_pin, self.OFF)
            self._set_state(self._b_pin, self.OFF)
        elif warn_level == radio.TELEMETRY_ERROR:
            self._set_state(self._r_pin, self.FLASHING)
            self._set_state(self._g_pin, self.OFF)
            self._set_state(self._b_pin, self.OFF)
        else:
            print("Displaying unknown warn state!?")
            self._set_state(self._r_pin, self.OFF)
            self._set_state(self._g_pin, self.OFF)
            self._set_state(self._b_pin, self.FLASHING)

    def _set_state(self, led, state):
        if state == self.ON:
            led.freq(2)
            led.duty(1)
        elif state == self.OFF:
            led.freq(2)
            led.duty(1023)
        elif state == self.FLASHING:
            led.freq(2)
            led.duty(512)


