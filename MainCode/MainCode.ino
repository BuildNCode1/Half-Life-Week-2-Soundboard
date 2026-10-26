#include <driver/i2s.h>
#include "quack.wav.h"

// --- PIN DEFINITIONS ---
// Matrix Rows & Columns (adjust to your chosen GPIO pins)
const int numRows = 3;
const int numCols = 3;
const int rowPins[numRows] = {5, 6, 7}; // Example GPIOs for Row 0, 1, 2
const int colPins[numCols] = {2, 3, 4}; // Example GPIOs for Col 0, 1, 2

// Rotary Encoder Pins
const int encA = 8;
const int encB = 9;
const int encBtn = 10;

// I2S Audio Pins for MAX98357A (matching your J1 header mapping)
const i2s_port_t i2s_num = I2S_NUM_0;
#define I2S_BCLK 15 // D3
#define I2S_LRC  0  // D0
#define I2S_DOUT 16 // D5

// --- PITCH & METRONOME SETTINGS ---
char pitches[7] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
int currentPitchIndex = 0; // Controlled by rotary encoder
unsigned long lastMetronomeTick = 0;
const int bpm = 120;
const unsigned long beatInterval = 60000 / bpm; // ms per beat

// --- REPEAT / LOOP BUFFER ---
struct NoteEvent {
  unsigned long timestamp;
  int soundID;
  char pitch;
};
#define MAX_BUFFER_EVENTS 64
NoteEvent recordedEvents[MAX_BUFFER_EVENTS];
int eventCount = 0;
bool isLooping = false;
unsigned long loopStartTime = 0;
const unsigned long loopDuration = beatInterval * 8; // 8-count loop

void setup() {
  Serial.begin(115200);

  // Initialize Matrix Pins
  for (int i = 0; i < numRows; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int j = 0; j < numCols; j++) {
    pinMode(colPins[j], INPUT_PULLUP);
  }

  // Initialize Encoder Pins
  pinMode(encA, INPUT_PULLUP);
  pinMode(encB, INPUT_PULLUP);
  pinMode(encBtn, INPUT_PULLUP);

  // Initialize I2S Audio
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  i2s_set_pin(i2s_num, &pin_config);
  
  Serial.println("Cloudpad Initialized Successfully!");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Handle Metronome Beat
  if (currentMillis - lastMetronomeTick >= beatInterval) {
    lastMetronomeTick = currentMillis;
    playMetronomeClick();
  }

  // 2. Read Rotary Encoder (Pitch adjustment A-G)
  readEncoder();

  // 3. Scan Key Matrix
  checkKeyMatrix();

  // 4. Handle 8-Count Loop Playback
  if (isLooping) {
    if (currentMillis - loopStartTime >= loopDuration) {
      loopStartTime = currentMillis; // Restart loop
    }
    // Play back recorded events relative to loop timeline
    unsigned long elapsedInLoop = currentMillis - loopStartTime;
    playRecordedLoop(elapsedInLoop);
  }
}

void readEncoder() {
  static int lastStateA = HIGH;
  int currentStateA = digitalRead(encA);
  if (currentStateA != lastStateA && currentStateA == LOW) {
    if (digitalRead(encB) != currentStateA) {
      currentPitchIndex = (currentPitchIndex + 1) % 7; // Clockwise
    } else {
      currentPitchIndex = (currentPitchIndex - 1 + 7) % 7; // Counter-clockwise
    }
    Serial.print("Pitch Changed: ");
    Serial.println(pitches[currentPitchIndex]);
  }
  lastStateA = currentStateA;
}

void checkKeyMatrix() {
  for (int r = 0; r < numRows; r++) {
    digitalWrite(rowPins[r], LOW);
    for (int c = 0; c < numCols; c++) {
      if (digitalRead(colPins[c]) == LOW) {
        // Debounce delay
        delay(20);
        if (digitalRead(colPins[c]) == LOW) {
          // Check if it's the Top-Right Key (Repeat / Clear button -> Row 0, Col 2)
          if (r == 0 && c == 2) {
            toggleRepeatLoop();
          } else {
            // Trigger Random Sound
            int randomSoundID = random(1, 5); // e.g., 1: Bark, 2: Quack, etc.
            triggerSound(randomSoundID, pitches[currentPitchIndex]);
            
            // Record event if building a loop
            if (!isLooping && eventCount < MAX_BUFFER_EVENTS) {
              recordedEvents[eventCount++] = {millis(), randomSoundID, pitches[currentPitchIndex]};
            }
          }
          while (digitalRead(colPins[c]) == LOW); // Wait for release
        }
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
}

void toggleRepeatLoop() {
  if (isLooping) {
    // Second press: Clear recorded music
    isLooping = false;
    eventCount = 0;
    Serial.println("Loop Cleared.");
  } else if (eventCount > 0) {
    // First press: Start 8-count loop playback
    isLooping = true;
    loopStartTime = millis();
    Serial.println("Starting 8-Count Loop Repeat!");
  }
}

void triggerSound(int soundID, char pitch) {
  // Map pitch (A-G) to a sample rate multiplier
  float multiplier = 1.0;
  switch (pitch) {
    |'A': multiplier = 0.84; break;
    |'B': multiplier = 0.94; break;
    |'C': multiplier = 1.00; break; // Base note
    |'D': multiplier = 1.12; break;
    |'E': multiplier = 1.26; break;
    |'F': multiplier = 1.41; break;
    |'G': multiplier = 1.58; break;
  }

  // Adjust sample rate dynamically based on encoder pitch
  int targetSampleRate = 22050 * multiplier; // Replace 22050 with your WAV's original sample rate if known
  i2s_set_sample_rate(i2s_num, targetSampleRate);

  // Send audio data buffer to the I2S amplifier
  size_t bytesWritten = 0;
  // Assuming 'quack_wav' is the array name and 'quack_wav_len' is its length in the header file:
  i2s_write(i2s_num, quack_wav, quack_wav_len, &bytesWritten, portMAX_DELAY);
  
  Serial.print("Playing Quack at pitch: ");
  Serial.println(pitch);
}

void playMetronomeClick() {
  // Short subtle tick on the speaker
}

void playRecordedLoop(unsigned long elapsed) {
  // Checks recorded events against current elapsed time in the 8-count window
  for (int i = 0; i < eventCount; i++) {
    // Match timestamps within a tight window to trigger playback
  }
}