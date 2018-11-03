What is this
------------

The ESP8266 is a very cheap microcontroller that has built in wifi modem.
I enjoy doing radio controlled projects (Eg cars, planes) and decided that
instead of forking out $80 for a receiver, and several hundred for a
transmitter, I'd rather pay a couple dollars for an ESP8266 and supporting
components - and also have the fun of building it.


This system uses a different configuration scheme to most hobby transmitters
and receivers. On most current commercial systems, you configure the model on
the transmitter. For example, you change the gain of a servo by configuring
the transmitter. As a result, most commercial transmitters have a "model
memory" to store settings for different models. To me this is backwards the
transmitter shouldn't care what it is transmitting to. Instead, the receiver
should be programmable. This is a bit of a paradigm shift, and results in
things like dual-rate switches being sent as additional "channels"

It is intended that an interface for configuring the receiver will be
developed, and that the configuration can be up/downloaded from a receiver via
USB/serial.


### What about wifi dropouts?

The ESP8266 supports being put into "moniter" mode in which it listens to
every packet going past. It also support "promiscious" mode, which allows it
to inject arbitrary packets. Combine these two and you have the ability to
send and receive without needing to bind to the access point.
However, this requires developing a separate protocol - which is what this is!


Build Requirements
------------------
For esp8266:
    xtensa-lx106-elf-gcc

For esp32:

