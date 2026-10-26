# Cloudpad - ESP32-C3 Audio Macropad

![Main PCB Render](images/MainRender)

Cloudpad is a custom-designed, compact 3x3 mechanical macropad featuring an integrated I2S audio amplifier, rotary encoder pitch shifter, metronome, and looping soundboard capabilities.

## Hardware Components
* **MCU:** Seeed Studio XIAO ESP32-C3
* **Audio Amplifier:** Adafruit MAX98357A I2S Class-D Amplifier
* **Speaker:** 3070 Cavity Enclosed Speaker (8Ω / 3W)
* **Inputs:** 3x3 Mechanical Key Matrix + Rotary Encoder with Push-button

## Pin Mapping Reference
* **I2S BCLK:** GPIO 15 (D3)
* **I2S LRC:** GPIO 0 (D0)
* **I2S DIN:** GPIO 16 (D5)
* **Matrix Rows:** GPIO 5, 6, 7
* **Matrix Columns:** GPIO 2, 3, 4
* **Rotary Encoder A/B/Button:** GPIO 8, 9, 10

---
![PCB With Case](images/MainRenderWithCasse)
