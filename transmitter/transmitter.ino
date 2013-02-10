/* 
 * Pufflux Transmitter
 *
 * Copyright 2013 Shaw Terwilliger <sterwill@tinfig.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
// #define DEBUG
 
#define TX_LED   13
#define TX       8
#define CHAN_0   9
#define CHAN_1   10
#define CHAN_2   11
#define CHAN_3   12

/*
 * How long in milliseconds we raise TX to transmit a nibble, and how long
 * we lower TX between nibbles.
 */
#define PULSE_DURATION 500

void setup() {
  Serial.begin(115200);
  pinMode(TX_LED, OUTPUT);
  pinMode(TX, OUTPUT);
  pinMode(CHAN_0, OUTPUT);
  pinMode(CHAN_1, OUTPUT);
  pinMode(CHAN_2, OUTPUT);
  pinMode(CHAN_3, OUTPUT);
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
  digitalWrite(CHAN_3, (nibble >> 3) & 0x1);
  digitalWrite(CHAN_2, (nibble >> 2) & 0x1);
  digitalWrite(CHAN_1, (nibble >> 1) & 0x1);
  digitalWrite(CHAN_0, (nibble) & 0x1);
  digitalWrite(TX_LED, HIGH);
  digitalWrite(TX, HIGH);
  delay(PULSE_DURATION);
  digitalWrite(TX, LOW);
  digitalWrite(TX_LED, LOW);
  delay(PULSE_DURATION);
}

void loop() {
  if (Serial.available() >= 2) {
    byte first = Serial.read();
    byte second = Serial.read();
#ifdef DEBUG    
    Serial.print("Sending [0x");
    Serial.print(first, HEX);
    Serial.print(",0x");
    Serial.print(second, HEX);
    Serial.println("]...");
#endif    
    send_byte(first);
    send_byte(second);
#ifdef DEBUG
    Serial.println("done.");
#endif    
  }
}

