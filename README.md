# Pufflux: A Colorful Cloud in my House

Pufflux is a lamp that looks like a cloud. The lamp contains an Arudino-compatible microcontroller with a wifi
connection and many RGB LEDs. It periodically fetches active weather alerts for my area and selects appropriate
colors and an animation style to match.  If there aren't any active alerts, it does a default animation that
fades random colors in and out.

Weather alert information comes from the United States 
[National Weather Service's web API](https://www.weather.gov/documentation/services-web-api).

![Pufflux Hanging from the Ceiling](photo.jpg)

## History

This is version 2 of the project. Version 1 was built in 2013 around an AVR microcontroller that didn't have 
any wifi hardware, and used a garage door opener-style RF receiver instead. This required an additional box 
with a microcontroller (and RF transmitter) to be connected to an Internet-connected computer in order to 
feed it weather forecasts. Now that wifi-enabled microcontrollers are cheap and easy to get, I decided to 
rebuild version 2 around it.  In version 2 there's only software for the microcontroller inside the lamp,
since it can read the weather all on its own.

You can find the code for version 1 in the "v1" branch.  There's a lot of Java in v1, including out-of-date
dependencies.  v2 is C/C++ for the microcontroller.

# SSL Certificates

If you want to run this software on your own microcontroller, you'll have to upload SSL certificates 
to your microcontroller's firmware.  See [the Adafruit documentation](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/updating-ssl-certificates)
for details.  Include certificates for these domains:

- geocode.arcgis.com
- api.weather.gov
