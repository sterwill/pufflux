* Pufflux: A Colorful Cloud in my House

Pufflux is a suite of software that gets weather information from the Internet and sends it to a special piece of hardware that lights up a puffy cloud hanging in my house to tell me the weather.

<pre>
 { wunderground.org } <-(http)-> pufflux-query -(serial)-> pufflux-transmitter -(radio)-> pufflux-receiver
</pre>

pufflux-query runs via cron, its output is redirected to an Arduino hanging off a serial port running pufflux-transmitter, which sends it via low-power radio to pufflix-receiver.
