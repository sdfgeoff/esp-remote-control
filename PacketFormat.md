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
If so, I'd suggest a framing something like:
```
+-----+------+------+------+-----------+
| UID | meta | Plen | Data | Checksum  |
+-----+------+------+------+-----------+
```
Where:
 - UID is the ID of the receiver (6 bytes)
 - Meta is the metadata about the packet (6 bytes)
 - Plen is the length of the data section (2 bytes)
 - Data is one the contents of the packet
 - Checksum is a checksum (suggested crc16)


#### The Receiver ID and what we do with the MAC addresses
There may be multiple controllers and multiple receivers on the same physical
channel (there are only 13 wifi channels). We also need to distinguish our
control packets from general wifi traffic. For this reason a unique identifier
for the receiver is used.

- All packets sent by a receiver include it's UID.
- The receiver will only listen to control packets if the UID of the control packet
  matches it's own UID.
- A tranmitter will only display telemetry of the receiver of which UID it is tranmitting to.

In the 802.11 format, there are three six-byte addresses. In order to prevent
other devices from interfering with the system, we fill them in a specific way.
We set the first two addresses (transmitter and receiver) to the ID.
No valid 802.11 packets will ever(?) be sent out with matching tranmitter and
receiver addresses, so most devices will probably ignore our packets, and
we will ignore everyone elses packets.
The third address is used to communicate the packet's metatdata

#### Metadata

```
+-----------------------------------------+
| Pid | 0x00 | 0x00 | 0x00 | 0x00  | Pty  |
+-----------------------------------------+
```
Where:
 - Pid is the packet ID. Every time a device sends a packet, it increments the
   counter. This (obviously) overflows at 255. It can be used to compute the
   packet loss. by counting skipped values.
 - The final byte is the packet type. Currently there are the types:
    - 0x01 (Control Packet v1)
    - 0x02 (Telemetry Packet v1)
    - 0x03 (Device Available Packet v1)

The other bytes are unused and should be left at zero. In future versions of
the protocol they may be used.



### Control Packet v1
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

Control packets are structured as:

```
+-----------------------------------------------------------
| C1l | C1h | C2l | C2h  | C3l  | ....
+-----------------------------------------------------------
```

Where:
 - C[x]l is the lower byte of the channel value (signed 16 bit int with -32768 being "unspecified")
 - C[x]h is the upper byte of the channel value (signed 16 bit int with -32768 being "unspecified")


### Telemetry Packet v1
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


### Device Available Packet
In order to discover what devices are available, the receiver needs to
communicate to the transmitter that it is expecting someone to control it.
This is done by sending a "Device Available" packet. Which consists of a
char array of the devices name. This packet type should only be sent when
a receiver has not recieved any packets from a transmitter yet.
