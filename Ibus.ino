#include <Arduino.h>

// --- Config ---
// Output iBUS on GPIO 4 (Connect to FC RX)
#define IBUS_TX_PIN 4  

// --- Variables ---
volatile uint32_t channel_start[6];
volatile uint16_t receiver_input[6] = {1500, 1500, 1500, 1500, 1500, 1500};

// Pins for PWM Input
const int ch_pins[6] = {16, 17, 18, 19, 32, 33};

// --- Separate Interrupt Handlers (Fixes the Crosstalk Bug) ---
// We need separate functions so we know EXACTLY which pin triggered the interrupt

void IRAM_ATTR isr_ch0() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[0])) channel_start[0] = now;
  else receiver_input[0] = now - channel_start[0];
}

void IRAM_ATTR isr_ch1() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[1])) channel_start[1] = now;
  else receiver_input[1] = now - channel_start[1];
}

void IRAM_ATTR isr_ch2() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[2])) channel_start[2] = now;
  else receiver_input[2] = now - channel_start[2];
}

void IRAM_ATTR isr_ch3() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[3])) channel_start[3] = now;
  else receiver_input[3] = now - channel_start[3];
}

void IRAM_ATTR isr_ch4() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[4])) channel_start[4] = now;
  else receiver_input[4] = now - channel_start[4];
}

void IRAM_ATTR isr_ch5() {
  uint32_t now = micros();
  if (digitalRead(ch_pins[5])) channel_start[5] = now;
  else receiver_input[5] = now - channel_start[5];
}

// --- Setup ---
void setup() {
  Serial.begin(115200); // Debug to PC
  
  // Setup iBUS Serial on Serial2 (TX=4)
  // RX is set to -1 (unused) because we only send
  Serial2.begin(115200, SERIAL_8N1, -1, IBUS_TX_PIN);

  // Setup Pins and Attach SEPARATE Interrupts
  pinMode(ch_pins[0], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[0]), isr_ch0, CHANGE);
  pinMode(ch_pins[1], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[1]), isr_ch1, CHANGE);
  pinMode(ch_pins[2], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[2]), isr_ch2, CHANGE);
  pinMode(ch_pins[3], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[3]), isr_ch3, CHANGE);
  pinMode(ch_pins[4], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[4]), isr_ch4, CHANGE);
  pinMode(ch_pins[5], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(ch_pins[5]), isr_ch5, CHANGE);
}

// --- Manual iBUS Packet Sender ---
void sendIBus() {
  uint8_t buffer[32];
  uint16_t checksum = 0xFFFF;

  // Header
  buffer[0] = 0x20;
  buffer[1] = 0x40;

  // Read volatile data safely
  uint16_t safe_channels[6];
  noInterrupts();
  for(int i=0; i<6; i++) safe_channels[i] = receiver_input[i];
  interrupts();

  // Fill Channels 1-6
  for(int i=0; i<6; i++) {
    uint16_t val = constrain(safe_channels[i], 1000, 2000);
    buffer[2 + 2*i] = val & 0xFF;
    buffer[2 + 2*i + 1] = (val >> 8);
  }

  // Fill Empty Channels 7-14 with 1500 (Center)
  for(int i=6; i<14; i++) {
    uint16_t val = 1500;
    buffer[2 + 2*i] = val & 0xFF;
    buffer[2 + 2*i + 1] = (val >> 8);
  }

  // Calculate Checksum
  for(int i=0; i<30; i++) checksum -= buffer[i];
  buffer[30] = checksum & 0xFF;
  buffer[31] = (checksum >> 8);

  // Send to Flight Controller
  Serial2.write(buffer, 32);
}

void loop() {
  sendIBus();
  
  // Debug to Serial Monitor (PC)
  Serial.print("CH1: "); Serial.print(receiver_input[0]);
  Serial.print(" | CH2: "); Serial.println(receiver_input[1]);

  delay(10); // Wait 10ms (approx 100Hz refresh rate)
}