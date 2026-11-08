# ABISS Project Description

**ABISS (Arduino-Based Intrinsic Stimulation System)** is a low-cost, open-source hardware platform for auditory and visual intrinsic optical signal imaging (IOSI). The system uses a custom printed circuit board built around an Arduino Nano to coordinate sensory stimulation, trial timing, and image-acquisition triggering. By consolidating these functions into a programmable device, ABISS offers a portable alternative to commercial stimulus-control hardware and laboratory-specific control software for routine functional cortical mapping.

## Motivation

Intrinsic optical signal imaging is a wide-field optical imaging method that maps cortical activity by measuring stimulus-evoked changes in tissue reflectance. It has been used to study the functional organization of sensory cortex, including retinotopic and tonotopic maps, without requiring exogenous dyes or genetically encoded indicators. Although the optical setup can be relatively straightforward, experiments also require reliable stimulus generation, trial timing, camera triggering, and coordination with image acquisition. These functions are often distributed across commercial hardware, acquisition computers, and laboratory-specific software, which can increase cost and make experimental workflows more difficult to reproduce.

ABISS addresses this practical barrier by integrating auditory or visual stimulus generation, experiment timing, and camera triggering into one programmable hardware platform. Users can modify experimental protocols by editing and uploading the appropriate Arduino firmware rather than developing a separate stimulus-control application.

## System

ABISS uses one Arduino Nano and a custom PCB for two independent stimulus configurations. The auditory and visual firmware are separate programs; users upload the program corresponding to the modality they intend to run. The two stimulus modalities are not generated simultaneously by the supplied firmware.

### Audio channel

The audio configuration generates programmable sine-wave tones using an AD9833-based GY-9833 direct digital synthesis (DDS) module controlled over SPI. The supplied firmware cycles through 3, 10, and 30 kHz tones. A PT2258 digital volume controller communicates over I²C, and an LT1970 amplifier stage drives the external speaker.

The firmware includes a per-frequency attenuation list (`volumeList`). In the supplied sketch, the assignment that transfers the selected list value to `targetVolume` is commented out. To apply the listed per-frequency attenuation values, users must uncomment `targetVolume = (double)currentVolume;` in `generateTone()`. With that line left commented, the active tone is played with the PT2258 attenuation set to zero. The supplied firmware does not use fade-in or fade-out ramps; the ramp code is intentionally commented out.

### Visual channel

The visual configuration generates VGA-compatible high-contrast drifting checkerboard bars using Arduino GPIO pins and hardware timer interrupts. The stimulus coordinate space is 440 × 480 pixels, with a 60 Hz VGA refresh rate. Vertical bars can sweep horizontally, and horizontal bars can sweep vertically, supporting phase-encoded mapping of azimuth and elevation. Bar motion is updated at 20 Hz. Checkerboard polarity flicker is an optional firmware setting controlled by `colorFlicker`; it is disabled by default and can be enabled by changing the Boolean setting.

### Camera triggering and experimental control

Both firmware programs provide a 20 Hz digital camera-trigger output. In the visual configuration, the trigger is synchronized to the VGA refresh cycle and is active during bar movement, with triggering paused during blank intervals. In the audio configuration, triggering spans the programmed pre-stimulus, stimulus, and post-stimulus acquisition window, then pauses during the inter-trial rest period. A start input allows a user or compatible external signal to initiate the programmed sequence.

ABISS handles stimulus control and camera-trigger generation; it does not replace the camera or the computer used to configure image acquisition and store acquired images. In the reported experiments, camera acquisition was configured using FLIR SpinView and images were streamed to a desktop computer.

## Validation

ABISS was evaluated through engineering measurements and biological intrinsic-imaging experiments.

- **Auditory validation:** ABISS-generated 3, 10, and 30 kHz stimuli produced frequency-dependent intrinsic response maps that were spatially comparable to maps generated using a commercial TDT RZ6 system. SSIM comparisons found no significant difference between ABISS–RZ6 cross-system similarity and the RZ6 odd–even self-consistency benchmark at 3, 10, or 30 kHz (Wilcoxon rank-sum p-values: 0.8413, 0.6905, and 1.000, respectively; five recording sessions per condition).
- **Visual validation:** ABISS-driven drifting-bar stimulation produced horizontal and vertical retinotopic maps and visual field-sign maps with alternating mirror and non-mirror representations consistent with the known organization of mouse visual cortex. The maps supported putative delineation of V1 and higher visual areas LM, RL, AM, P, and PM. Across four animals, median odd–even SSIM was 0.908 for elevation maps, 0.875 for azimuth maps, and 0.641 for visual field-sign maps.
- **Engineering validation:** Auditory tone spectra showed the intended 3, 10, and 30 kHz frequency components. Extended recordings demonstrated stable timing between stimulus presentation and camera triggering. Visual timing was independently measured using a photodiode positioned at the display.

## Cost and Form Factor

The prototype is approximately 4 × 3 × 1 inches and weighs approximately 150 g. The paper's bill of materials estimates the hardware cost at approximately $138 (less than $150), excluding external equipment such as the camera, illumination source, optical components, computer, monitor, and speaker.

## Appropriate Use Cases

ABISS is well suited for:

- Auditory and visual cortical mapping in head-fixed rodents.
- Teaching demonstrations and pilot intrinsic-imaging experiments.
- Laboratories seeking a portable, reproducible, and relatively inexpensive stimulus-control platform.
- Experiments based on programmable tone sequences or high-contrast drifting-bar visual stimuli.

ABISS is not intended to replace specialized commercial systems for experiments requiring binaural or multi-speaker spatial audio, complex naturalistic visual scenes, continuously adjustable grayscale output, real-time display warping, closed-loop behavioral control, or large-scale synchronized multi-channel acquisition.

## Associated Paper

Qu Z, Kazemi K, Wu T, Doddapujar SN, Marrazzo TA, Gazzola M, Gritton HJ. *ABISS: An Open-Source, Low-Cost Platform for Auditory and Visual Intrinsic Optical Signal Imaging.* 2026.

Departments of Bioengineering, Electrical Engineering, Comparative Biosciences, and Mechanical Science and Engineering — University of Illinois Urbana-Champaign.
