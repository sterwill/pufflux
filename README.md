# Pufflux: A Colorful Cloud in my House

Pufflux is a suite of software that gets weather information from the 
Internet and sends it to a special piece of hardware that lights up a 
puffy cloud hanging in my house to tell me the weather.

## Diagram

<pre>
 { wunderground.org } <-(http)-> pufflux-query -(serial)-> pufflux-transmitter -(radio)-> pufflux-receiver
</pre>

pufflux-query runs via cron, its output is redirected to an Arduino 
hanging off a serial port running pufflux-transmitter, which sends 
it via low-power radio to pufflix-receiver.

The amount of data pufflux-transmitter sends to pufflux-receiver is 
tiny because the radio link operates at a very low speed.  An
animation style and speed corresponding to conditions, as well as
two colors, are packed into 16 bits.
