The protocol
------------
The ESP family have certain requirements about the packets they send. They
have to send valid 802.11 packets. This isn't a problem at all because it means
that all the checksums and packet handling at the low level are done for us.

The communication protocol is asymmetric - it has a different protocol for
sending commands from the transmitter to the receiver and for sending telemtry
from the receiver to the transmitter.

### Packet Framing
The ESP family is restricted to sending valid 802.11 packets. This allows re-use
of both the checksum and MAC addresses and packet length fields. There is no
reason you can't implement this on your own hardware that doesn't do so.

In order to fit into the required structure, the packets this system sends are
of the format:

```
+------+------+------+------+
| 0x08 | 0x00 | 0x00 | 0x00 |   802.11 packet type information (must have these values)
+------+------+------+------+

+------+------+------+------+------+------+
| Uid1 | Uid2 | Uid3 | Uid4 | Uid5 | Uid6 |    Identifier of the intended receiver
+------+------+------+------+------+------+

+-----------------------------------------+
|  d0 ...                             d11 |    First 12 data bytes (always sent)
+-----------------------------------------+

+------+------+
| Pcnt | Ptyp |   Packet counter and Packet type
+------+------+

+------+------+
| 0x00 | 0x00 |   802.11 QOS control (must have these values)
+------+------+

+-----------------------------------------+
|  d12 ...                                |    Any remaining data bytes
+-----------------------------------------+

+------------+
|  CRC 16    |    802.11 CRC16. This is automatically added by the ESP hardware
+------------+
```


The reason we pack the first 12 data bytes into the 802.11 header is not just
for efficiencies sake. The ESP8266 can only receive the header when in
promiscious mode, and putting the first 12 bytes of data there allows it to
function "normally" but on a more limited level than the ESP32.


#### The Receiver ID and what we do with the MAC addresses
There may be multiple controllers and multiple receivers on the same physical
channel (there are only 13 wifi channels), so we borrow the "MAC" address from
802.11. This also allows the ESP hardware to filter the packets in hardware.

- All packets sent by a receiver use the UID of the receiver.
- All packets sent by a transmitter contain the UID of the receiver.
- The receiver will only listen to control packets if the UID of the control packet
  matches it's own UID.
- A tranmitter will only display telemetry of the receiver of which UID it is tranmitting to.

This makes binding to a device very easy (the transmitter can list devies) but
it also means it's quite easy to "shoot down" another receiver accidentally -
any transmitter can broadcast to any receiver. A future revision of this
protocol may change the transmitter to transmit with it's own UID and the
receiver can check it (This would not provide protection against malicious
action). However, to do so requires some way to "bind" the receiver, and I
don't feel like investigating methods to do so yet.

### Control Packet v1 (0x01)
A single control packets should be sufficient to control the entire system.
For this reason, a control packet is the entire state of the transmitter (all
it's knobs and dials). For this reason, a control packet simply some number of
"channels" - as many as it supports.

Channels are transmitted as an ordered collection of signed 16 bit numbers.
A transmitter can transmit more channels than it physically has, and can
transmit channels as "unspecified" by sending the value as `-32768`.

This is useful when, for example, a five channel transmitter has a knob in the
same position as an 8 channel receiver, it may wish to transmit that knob on,
say, channel 7. To do so it may transmit channel 5 and 6 as "unspecified"
(-32768). This tells the receiver that it should decide what to do with that
channel because the transmitter is not telling it what to do.

Control packets data are structured as:

```
+-----------------------------------------------------------
| C1l | C1h | C2l | C2h  | C3l  | ....
+-----------------------------------------------------------
```

Where:
 - C[x]l is the lower byte of the channel value (signed 16 bit int with -32768 being "unspecified")
 - C[x]h is the upper byte of the channel value (signed 16 bit int with -32768 being "unspecified")

It is convention to have the first five channels as:

1. analog stick 1 x-axis (aileron)
2. analog stick 1 y-axis (elevator)
3. analog stick 2 x-axis (rudder)
4. analog stick 2 y-axis (throttle)
5. Button bitarray:
    - toggle_switch_left_1
    - toggle_switch_left_2
    - toggle_switch_right_1
    - toggle_switch_right_2
    - 3_pos_switch_left (2 bits)
    - 3_pos_switch_right (2 bits)
    - push_button_left_1
    - push_button_left_2
    - push_button_right_1
    - push_button_right_2
    - pov_hat_up
    - pov_hat_down
    - pov_hat_left
    - pov_hat_right


Where stick 1 is on the right for a mode2 transmitter and stock1 is on the
left in a mode1 transmitter. Transmitters should offer the ability to switch
the two sticks.

### Telemetry Packet v1 (0x02)
Telemetry is not needed to be reliable, and there is no way to know the
telemetry that may be useful for every possile model. (eg a combat robot
may have a "weapon armed" telemetry type but a RC car may not). For this
reason the telemetry is sent as many small packets, each with a single
mapping between a name (eg battery voltage) and a value (eg 3.67).

For this reason, a telemetry packet consists of:
- A status (eg "WARNING")
- A value (eg "1.372")
- A name (eg "battery voltage)


Telelemetry packets are structured as:

```
+---------------------------------------------------+
|  St  | V1 | V2 | V3 | V4 |  ParameterName         |
+---------------------------------------------------+
```

Where:

- St is a status byte. It takes the value of 0 for OK, 1 for WARN, 2 for
  ERROR, 255 for UNSTATED (no implicit good or bad values)
- V[x] is the value. It is a 32 bit float, so it occupies four bytes
- ParameterName is a string that occupies the rest of the packet.


Some useful telemetry packets include things such as:
 - battery voltage
 - packet rssi
 - packet loss


### Device Name Packet (0x03)
In order to discover what devices are available, the receiver needs to
communicate to the transmitter that it is expecting someone to control it.
This is done by sending a "Device Available" packet. Which consists of a
char array of the devices name. This packet type should only be sent when
a receiver has not recieved any packets from a transmitter yet.
