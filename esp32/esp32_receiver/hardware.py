import machine
import radio
import time


class Drive:
    def __init__(self):
        self.front_left = ServoDrive(14, 74, -1)
        self.front_right = ServoDrive(27, 76, 1)
        self.rear_left = ServoDrive(26, 77, -1)
        self.rear_right = ServoDrive(25, 68, 1)

        self.velocity = 0.0
        self.turn = 0.0

    def set_targets(self, velocity, turn):
        self.velocity = velocity
        self.turn = turn

    def update(self):
        left = self.velocity - self.turn
        right = self.velocity + self.turn
        self.front_left.set_velocity(left)
        self.rear_left.set_velocity(left)
        self.front_right.set_velocity(right)
        self.rear_right.set_velocity(right)

class ServoDrive:
    STEPS_TO_MAX_VEL = 60
    def __init__(self, pin, center, invert=False):
        self.pin = pin
        self.center = center
        self._pwm = machine.PWM(machine.Pin(self.pin))
        self._pwm.freq(50)
        self.invert = invert
        self.set_velocity(0)

    def set_velocity(self, velocity):
        self._pwm.duty(
            int(self.center + (velocity * self.STEPS_TO_MAX_VEL) * self.invert)
        )

