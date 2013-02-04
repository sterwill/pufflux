/* 
 * Pufflux Transmitter
 *
 * Copyright 2013 Shaw Terwilliger <sterwill@tinfig.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */
 
#include <VirtualWire.h>

const int led = 13;
const int tx = 8;
const int chan0 = 9;
const int chan1 = 10;
const int chan2 = 11;
const int chan3 = 12;

const short pulse_duration = 1000;

void setup() {
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(tx, OUTPUT);
  pinMode(chan0, OUTPUT);
  pinMode(chan1, OUTPUT);
  pinMode(chan2, OUTPUT);
  pinMode(chan3, OUTPUT);
}

/*
 * Sends the high nibble, then the low nibble.
 */
void send_byte(byte b) {
  send_nibble((b & 0b11110000) >> 4);
  send_nibble(b & 0b00001111);
}

/*
 * Sends the low nibble.
 */
void send_nibble(byte nibble) {
  digitalWrite(chan3, (nibble >> 3) & 0x1);
  digitalWrite(chan2, (nibble >> 2) & 0x1);
  digitalWrite(chan1, (nibble >> 1) & 0x1);
  digitalWrite(chan0, (nibble) & 0x1);
  digitalWrite(led, HIGH);
  digitalWrite(tx, HIGH);
  delay(pulse_duration);
  digitalWrite(tx, LOW);
  digitalWrite(led, LOW);
  delay(pulse_duration);
}

void loop() {
  send_byte(170);
  send_byte(255);
  delay(10000);
  return;
  
  if (Serial.available() >= 2) {
    Serial.print("Sending... ");
    send_byte(Serial.read());
    send_byte(Serial.read());
    Serial.println("done.");
  }
}

