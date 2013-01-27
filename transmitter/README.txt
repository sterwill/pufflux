Pufflux Transmitter

This is an Arduino program that drives a low power RF transmitter
to send commands to a remote system with a matching RF receiver
running Pufflux Receiver.

You would typically run this program on an Arudino connected to 
a normal computer via USB.  The computer runs Pufflux Query,
which computes what message to send, kicks it to the Arduino 
running this program over a serial link, and this program beams 
it over the radio to the Pufflux Receiver.

This program uses the VirtualWire library under the terms of the GPL
version 2.

    http://www.open.com.au/mikem/arduino/VirtualWire/

--
Shaw Terwilliger <sterwill@tinfig.com>
