Pufflux Weather Query Program

This is a command-line Java program that queries the Weather
Underground (wunderground.org) API for weather alerts for a
location, encodes the conditions as a super-tiny message,
and writes that message to standard output.

It's expected that the output is redirected to a serial
port or some other channel that talks to an Arduino running
pufflux_transmitter which forwards the message over
RF to another Arduino running pufflux_receiver).

--
Shaw Terwilliger <sterwill@tinfig.com>
