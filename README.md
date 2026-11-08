# ABISS: An Open-Source, Low-Cost Platform for Auditory and Visual Intrinsic Optical Signal Imaging

**Zhili Qu, Kimia Kazemi, Tianyang Wu, Shambulingayya N. Doddapujar, Tyler A. Marrazzo, Mattia Gazzola, Howard J. Gritton**  
University of Illinois Urbana-Champaign — Departments of Bioengineering, Electrical Engineering, Comparative Biosciences, and Mechanical Science and Engineering

---

## Overview

ABISS (Arduino-Based Intrinsic Stimulation System) is a compact, low-cost, open-source hardware platform for intrinsic optical signal imaging (IOSI). A custom printed circuit board built around an Arduino Nano integrates auditory or visual stimulus generation, trial timing, and image-acquisition triggering. Experimental protocols are defined in Arduino firmware, allowing users to reconfigure the platform by editing and uploading the firmware for the desired stimulus modality.

ABISS is designed to reduce the hardware and software complexity associated with routine functional cortical mapping. It replaces much of the stimulus-control and synchronization infrastructure often assembled from commercial systems and laboratory-specific software. It does **not** replace the imaging camera or the computer used to configure image acquisition and store image data.

Auditory validation found that ABISS–TDT RZ6 cross-system map similarity was comparable to the RZ6 odd–even self-consistency benchmark at 3, 10, and 30 kHz. Visual validation demonstrated horizontal and vertical retinotopic maps and visual field-sign organization consistent with mouse visual cortex.

The prototype is approximately 4 × 3 × 1 inches, weighs approximately 150 g, and has an estimated hardware bill of materials of approximately $138 (less than $150), excluding the camera, illumination source, optical components, computer, monitor, and speaker.

## Repository Structure

Use these filenames for the firmware when adding them to the repository. The visual sketch can be renamed to remove any duplicate-download suffix such as `(1)`; renaming the file does not require changing the code.

```text
ABISS/
├── Audio/
│   └── Audio_Stimulus.ino
├── Visual/
│   └── VGA_with_switching_checkerboard.ino
├── README.md
└── Project_Description.md
```

The associated paper also states that the public repository includes supporting hardware files and custom MATLAB scripts for stimulus validation and auditory/visual IOSI analysis. Include those materials in the repository before describing the repository as a complete release of all paper-associated code and hardware documentation.

## Hardware

ABISS is built around an **Arduino Nano** installed on a custom PCB. The PCB provides three BNC-style coaxial connectors for digital input/output:

| Connector | Arduino pin | Function in the reported configuration |
|---|---:|---|
| J2 | D8 | Synchronized camera-trigger output |
| J3 | D7 | General-purpose digital I/O, reserved for future use |
| J4 | D9 | External TTL start input |

The connector routing is fixed by the PCB. The signal behavior can be changed in firmware, subject to the hardware connection.

### Audio hardware

An AD9833-based GY-9833 module generates sine-wave tones. The PT2258 digital volume controller communicates over I²C, and an LT1970 amplifier stage drives the external speaker. A MAX660 charge-pump voltage inverter generates an approximately −5 V rail from the +5 V input for the LT1970 amplifier stage.

### Visual hardware

The VGA interface is driven directly by Arduino GPIO pins. The visual sketch generates a 440 × 480 pixel stimulus area at a 60 Hz VGA refresh rate. The framebuffer uses one byte for each group of eight horizontal pixels and one buffer row for each 16 scanlines, reducing memory requirements on the Arduino Nano.

### Power

The board is powered from a regulated +5 V DC supply through its barrel jack. The MAX660 generates the approximately −5 V rail required by the audio amplifier.

### Wiring — Visual Stimulus Module (VGA DB15)

| Arduino pin | Signal | Series resistor | DB15 pin |
|---|---|---:|---:|
| D3 | HSYNC | 68 Ω | 13 |
| D4 | RED | 470 Ω | 1 |
| D5 | GREEN | 470 Ω | 2 |
| D6 | BLUE | 470 Ω | 3 |
| D10 | VSYNC | 68 Ω | 14 |
| D8 | Camera-trigger output (20 Hz) | — | — |
| D9 | Start button/input (active LOW; button to GND) | — | — |
| GND | Ground | — | 5, 6, 7, 8, 10 |

### Wiring — Audio Stimulus Module

| Arduino pin / interface | Signal |
|---|---|
| D2 (`FNC_PIN`) | AD9833 module function/control pin |
| D8 (`LED_PIN`) | Camera-trigger output (20 Hz; this is not an indicator LED) |
| D9 (`BUTTON_PIN`) | Start/stop button input (button to GND) |
| I²C SDA/SCL | PT2258 digital volume controller (address `0x8C`) |

On the standard Arduino Nano, I²C uses A4 (SDA) and A5 (SCL).

### Abbreviated Bill of Materials

| Component | Description | Part number / reference | Quantity |
|---|---|---|---:|
| Arduino Nano | Central microcontroller | 1050-1001-ND | 1 |
| MAX660CPA+ | Charge-pump voltage inverter | MAX660CPA+-ND | 1 |
| GY-9833 (AD9833) | DDS signal-generator module | Amazon ASIN B082W7XM47 | 1 |
| PT2258 EVC Click | Digital volume-controller module | 1471-MIKROE-3358-ND | 1 |
| LT1970CFE | Audio power amplifier | LT1970CFE#PBF-ND | 1 |
| DE15 VGA connector | VGA output | L77HDE15SD1CH4FVGA-ND | 1 |
| 100 µF electrolytic capacitor | Bulk smoothing capacitors | 1189-1300-ND | 2 |
| R1 | 1 Ω resistor | See Extended Data Table 1 in paper | 1 |
| R2 | 3 kΩ resistor | CF14JT3K00TR-ND | 1 |
| R3 | 10 kΩ resistor | CF14JT10K0TR-ND | 1 |
| R4 | 5 kΩ resistor | RNF18FTD4K99TR-ND | 1 |
| R5–R7 | 470 Ω VGA RGB resistors | 470QBK-ND | 3 |
| R8–R9 | 68 Ω VGA sync resistors | CF18JT68R0TR-ND | 2 |
| J2, J3, J4 | BNC-style digital I/O connectors | 2057-RF1-01A-D-00-75-M-ND | 3 |
| J10 | Coaxial audio-output connector | 2057-RF1-01A-D-00-75-M-ND | 1 |

The paper's Extended Data Table 1 provides the full bill of materials, source links, and component-cost breakdown. Its listed component total is approximately $138.

## Software Dependencies

### Visual firmware

- `TimerHelpers.h` — hardware timer configuration helpers used by the visual sketch.
- `avr/pgmspace.h` — AVR program-memory utilities.
- `avr/sleep.h` — AVR sleep-mode control.
- `string.h` — standard memory utilities.

The AVR headers are provided with the AVR toolchain used by the Arduino Nano board package. Ensure `TimerHelpers.h` is available to the Arduino IDE when compiling the visual sketch.

### Audio firmware

- [AD9833 Arduino library](https://github.com/Billwilliams1952/AD9833-Library-Arduino) — DDS waveform generation.
- [Protothreads](http://dunkels.com/adam/pt/) — cooperative task scheduling (`pt.h`).
- [PT2258 library](https://github.com/BenoitSchillings/pt2258) — I²C digital volume control.
- `Wire` — Arduino I²C library, included with the Arduino platform.

## Firmware Overview

The repository contains two independent Arduino programs for the same Nano-based hardware. Upload **one firmware at a time**, depending on whether the experiment requires auditory or visual stimulation. The programs are not intended to generate both modalities simultaneously.

### Visual Stimulus Firmware

The visual sketch uses hardware timer interrupts to generate VGA synchronization signals. Timer1 is configured for vertical synchronization and Timer2 for horizontal synchronization. RGB output is written directly through GPIO pins. The display refreshes at 60 Hz, while stimulus motion and camera-trigger timing are updated at 20 Hz (once every three VGA frames). The main loop uses `SLEEP_MODE_IDLE` between interrupt-driven scanline operations.

The visual stimulus consists of ten checkerboard bars on a black background. Vertical bars sweep horizontally, or horizontal bars sweep vertically, depending on the selected starting direction. The opposite direction is run automatically to complete each bidirectional pair. Camera triggering is enabled during active bar motion and disabled during the blank intervals.

**Visual firmware parameters in the supplied sketch:**

| Parameter | Default in sketch | Description |
|---|---:|---|
| `setBarSpeed` | `0.078125` | Movement increment per 20 Hz logic update, expressed in the active buffer-coordinate units (bytes for horizontal movement; buffer rows for vertical movement) |
| `barsPerBatch` | `10` | Number of bars in each directional sweep |
| `hThicknessBytes` | `5` (40 px) | Vertical-bar width in horizontal buffer bytes |
| `vThicknessRows` | `3` (48 px) | Horizontal-bar height in buffer rows |
| `hSpacingBytes` | `60` | Spacing between vertical bars, in bytes |
| `vSpacingRows` | `33` | Spacing between horizontal bars, in buffer rows |
| `numPairs` | `10` | Number of forward/reverse pairs; `0` means run indefinitely |
| `sweepsPerPhase` | `1` | Number of sweeps in each direction before reversing |
| `colorFlicker` | `false` | Optional checkerboard polarity inversion; set to `true` to enable |
| `FLICKER_TOGGLE_FRAMES` | `15` | Number of 60 Hz VGA frames between polarity toggles when flicker is enabled |
| `interPhasePauseFrames` | `400` logic ticks (20 s) | Blank pause between the forward and reverse sweeps of a pair |
| `interPairPauseFrames` | `600` logic ticks (30 s) | Blank pause between completed bidirectional pairs |
| `direction` | `TOP_TO_BOTTOM` | Initial sweep direction; can be changed to `LEFT_TO_RIGHT`, `RIGHT_TO_LEFT`, or `BOTTOM_TO_TOP` |
| `stopAtEnd` | `true` | Clear the screen and stop after the configured number of pairs |

**Timing note:** the firmware's motion increment is expressed in buffer coordinates, not directly in pixels per second. Horizontal positions use 8-pixel bytes, while vertical positions use buffer rows representing 16 scanlines. The associated paper reports the visual experimental velocity in pixels per second; use the paper's Methods section when referring to the published experimental protocol.

The paper's visual IOSI protocol used ten bidirectional sweep cycles per orientation and a 20-second blank interval between opposite-direction sweeps. The supplied sketch also has a 30-second blank interval between completed bidirectional pairs. Camera triggering is paused during both blank intervals in the current sketch.

### Audio Stimulus Firmware

The audio sketch uses Protothreads to coordinate the camera-trigger output and tone sequence in a cooperative, non-preemptive loop. Timing is based on the Arduino `micros()` clock. The AD9833 module generates the tone, while the PT2258 applies the selected attenuation setting. **The supplied firmware does not implement fade-in or fade-out ramps.** The ramp-related variables and code remain in the sketch as commented-out material and are not part of the active stimulus sequence.

The default frequency list is 3, 10, and 30 kHz. The firmware advances through this list for 60 individual tone presentations, giving 20 presentations per frequency, and then returns to its idle state. The button on D9 toggles the running state. After a button press, the program waits for `buttonToStartTime` before beginning the sequence. The paper's direct ABISS–RZ6 biological comparison reports 10 ABISS trials per frequency; therefore, distinguish the autonomous default count in this supplied sketch from the trial count reported for that particular experiment.

**Audio firmware parameters in the supplied sketch:**

| Parameter | Default in sketch | Description |
|---|---:|---|
| `frequencyList` | `{3000, 10000, 30000}` Hz | Tone frequencies, repeated in sequence |
| `volumeList` | `{14, 7, 7}` | Intended PT2258 attenuation values for the corresponding frequencies (0 = loudest; 79 = muted); see note below |
| `delayUntilStimulus` | `1.50 s` | Delay from the start of the camera-trigger sequence to tone onset |
| `stimulusTime` | `1.0 s` | Tone duration |
| `delayAfterStimulus` | `1.50 s` | Camera-triggered post-tone acquisition period |
| `restTimeInSeconds` | `27.0 s` | Rest period after the camera-triggered acquisition window |
| `blinksPerSecond` | `20.0` | Camera-trigger frequency |
| `buttonToStartTime` | `1.0 s` | Delay after the start button press before the sequence starts |

**Per-frequency attenuation note:** in `generateTone()`, the sketch contains the commented-out line:

```cpp
//targetVolume = (double)currentVolume;
```

Uncomment this assignment if you want the values in `volumeList[]` to set the PT2258 attenuation for each frequency. With the line left commented, `targetVolume` remains initialized to zero, so the active tone is played with attenuation set to `0` (the loudest setting on the PT2258 scale). The sketch initially mutes the channel at attenuation `79`, sets attenuation to `targetVolume` during the tone, and returns to attenuation `79` after the tone. No fade ramp is applied.

The programmed audio timing is a 4-second camera-triggered acquisition window: 1.5 seconds before tone onset, 1.0 second of tone, and 1.5 seconds after tone offset. The camera-trigger output then pauses for 27 seconds. This gives a 31-second start-to-start cycle. The 20 Hz trigger output has a 10 ms HIGH interval within each 50 ms period.

## Experimental Protocols Reported in the Paper

The following summarizes the biological imaging protocols reported in the associated manuscript. These method descriptions are distinct from configurable defaults in the firmware tables above.

### Auditory Intrinsic Optical Signal Imaging

An auditory IOSI trial consisted of a 1.5-second baseline, a 1-second pure tone at 75 dB SPL (3, 10, or 30 kHz), and a 28.5-second inter-trial interval. Images were acquired at 20 Hz under 625 nm red-light illumination. Response maps were calculated as relative reflectance change (ΔR/R), using frames 1–30 for the baseline and frames 41–80 for the response, followed by Gaussian smoothing (σ = 3) and averaging across trials.

### Visual Intrinsic Optical Signal Imaging

Visual stimulation used phase-encoded drifting checkerboard bars in four cardinal directions. Vertical bars moved horizontally to map azimuth, and horizontal bars moved vertically to map elevation. The paper reports a bar width of 40 pixels for horizontal traversals and 48 pixels for vertical traversals, with a programmed velocity of 25 pixels/s. Ten bidirectional sweep cycles were acquired per orientation, except for one animal with nine complete cycles. A 20-second blank interval separated the forward and reverse sweeps, and camera triggering was paused during this interval.

Retinotopic maps were generated using phase-encoded Fourier analysis. Opposite-direction responses were combined to compensate for hemodynamic delay, and visual field-sign maps were calculated from the local gradients of the azimuth and elevation maps.

### Engineering Validation

For auditory timing validation, the paper Methods and Results describe a separate protocol using a 2-second pre-stimulus period, a 2-second tone, and a 2-second post-stimulus period, repeated for 40 cycles of the three-frequency sequence (120 tone presentations). The Figure 3 caption also contains a conflicting statement that the sequence was repeated 10 times for 30 presentations, despite later referring to 120 trials; that caption should be reconciled against the validation data before publication. For visual timing validation, photodiode measurements were used to track bar arrival and departure at a fixed screen location while camera-trigger pulses were recorded simultaneously.

## Validation Results

### Auditory

Five ABISS–RZ6 cross-system recording sessions and five RZ6 self-consistency sessions were analyzed. No significant differences were detected between the cross-system and self-consistency SSIM distributions at 3 kHz (p = 0.8413), 10 kHz (p = 0.6905), or 30 kHz (p = 1.000), using two-sided Wilcoxon rank-sum tests.

### Visual

Visual retinotopic and visual field-sign maps were generated in four animals. Median odd–even SSIM was 0.908 for elevation maps (range 0.800–0.942), 0.875 for azimuth maps (range 0.770–0.906), and 0.641 for visual field-sign maps (range 0.541–0.687). ABISS-derived V1 areas ranged from 1.29 to 2.42 mm² (median 1.94 mm²), compared with 0.90 to 3.50 mm² (median 2.61 mm²) in eight published maps; the difference was not statistically significant (Wilcoxon rank-sum p = 0.3677).

## Operation

### Visual stimulus

1. Upload `VGA_with_switching_checkerboard.ino` to the Arduino Nano.
2. Connect the VGA output to the monitor using the wiring table above. Connect the camera-trigger output on D8 to the camera's trigger input.
3. Connect the start button between D9 and GND.
4. Set `direction` in the sketch to choose the initial sweep axis and direction. The opposite direction is generated automatically.
5. Press the button to begin. The firmware runs the configured number of bidirectional pairs, blanks the display during the inter-phase and inter-pair pauses, and stops after `numPairs` pairs when `stopAtEnd` is `true`.

### Audio stimulus

1. Upload `Audio_Stimulus.ino` to the Arduino Nano.
2. Connect the AD9833 module, PT2258 volume-controller module, LT1970 amplifier stage, and external speaker. Connect the camera-trigger output on D8 and the start/stop button between D9 and GND.
3. Confirm the desired frequencies and timing values in the audio firmware. Uncomment `targetVolume = (double)currentVolume;` in `generateTone()` if the per-frequency values in `volumeList[]` should be applied.
4. Press the button. After the configured one-second start delay, the firmware begins its sequence of 60 tone presentations (20 repetitions of each frequency) and then returns to idle.

Only one of the two firmware programs is uploaded and run at a time on the supplied Arduino Nano hardware.

## Limitations

The Arduino Nano provides sufficient timing and memory resources for the simple stimuli implemented here, but it limits display complexity and computational flexibility. The present VGA implementation is intended for high-contrast patterns such as drifting checkerboard bars rather than complex naturalistic scenes, high-resolution textures, continuously adjustable grayscale, or real-time geometric correction for curved displays. The audio implementation provides a single-channel speaker output and is not designed for binaural or multi-speaker spatial stimulation. ABISS is also not intended for closed-loop behavioral control or large-scale synchronized multi-channel acquisition; those applications may be better served by specialized commercial systems or more capable stimulus platforms.

## Citation

If you use ABISS in your research, please cite the associated paper:

> Qu Z, Kazemi K, Wu T, Doddapujar SN, Marrazzo TA, Gazzola M, Gritton HJ. *ABISS: An Open-Source, Low-Cost Platform for Auditory and Visual Intrinsic Optical Signal Imaging.* 2026.

---
