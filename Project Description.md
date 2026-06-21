# ABISS Project Description

**ABISS (Arduino-Based Intrinsic Stimulation System)** is a low-cost, open-source hardware platform for auditory and visual intrinsic optical signal imaging (IOSI). The system uses an Arduino Nano-controlled printed circuit board to coordinate sensory stimulation, trial timing, and image-acquisition triggering — replacing the need for commercial systems (TDT, National Instruments), dedicated acquisition computers, or laboratory-specific MATLAB/LabVIEW/Synapse software.

## Motivation

Intrinsic optical signal imaging (IOSI) is a wide-field optical imaging method that maps cortical activity by measuring stimulus-evoked changes in tissue reflectance. Since its development in the 1980s, IOSI has become a widely used tool for studying the functional organization of sensory cortex — including retinotopic, tonotopic, and somatotopic maps — without requiring exogenous dyes or genetically encoded indicators. Despite its conceptual simplicity, implementing IOSI often requires not only optical expertise, but also familiarity with stimulus generation, camera triggering, trial timing, and experiment-control software. Many existing workflows depend on laboratory-specific combinations of commercial hardware and software, creating practical barriers for new users and contributing to lab-to-lab variability.

ABISS was developed to address these barriers. By integrating stimulus generation and experimental control into a single programmable board, ABISS provides a portable, reproducible, and affordable alternative for laboratories seeking to implement or expand intrinsic imaging experiments.

## System

ABISS provides two independent stimulus channels:

- **Audio channel:** Generates user-selectable tone trains (e.g., 3 kHz, 10 kHz, 30 kHz) via an AD9833-based direct digital synthesis (DDS) module over SPI, with per-tone amplitude control and smooth fade-in/fade-out ramps managed by a PT2258 digital volume controller over I²C and amplified by an LT1970 stage.

- **Video channel:** Generates high-contrast drifting-bar visual stimuli (440 × 480 px, 60 Hz) via a direct VGA interface bit-banged through GPIO pins, with timing managed by hardware timer ISRs. Visual patterns include horizontal and vertical sweeping checkerboard bars suitable for phase-encoded retinotopic mapping.

Both channels issue a synchronized 20 Hz digital camera trigger, active only during periods of active stimulus presentation. Experimental protocols are fully configurable by editing and uploading Arduino firmware — no separate control computer is required during acquisition.

## Validation

ABISS was validated at both the hardware and biological levels:

- **Auditory:** ABISS-generated tone stimuli at 3, 10, and 30 kHz produced frequency-dependent intrinsic response maps in mouse auditory cortex that were spatially comparable to maps generated using a commercial TDT RZ6 system. Structural similarity index (SSIM) analysis showed that ABISS–TDT cross-system similarity was comparable to a TDT self-consistency benchmark (median ΔSSIM: −0.012, −0.036, and 0.017 at 3, 10, and 30 kHz respectively; all 95% bootstrap confidence intervals include zero).

- **Visual:** ABISS-driven drifting-bar stimulation produced horizontal and vertical retinotopic maps and a visual field sign map with alternating mirror and nonmirror representations consistent with the known organization of mouse visual cortex, enabling delineation of V1, LM, AL, RL, AM, and PM.

## Cost and Form Factor

The current prototype is approximately 4 × 3 inches, weighs approximately 150 g, and can be assembled for roughly $150 in components (excluding camera, illumination source, monitor, and speaker). A full bill of materials with sourcing links is provided in the associated paper.

## Appropriate Use Cases

ABISS is well-suited for:
- Auditory and visual cortical mapping in head-fixed rodents
- Teaching and pilot intrinsic imaging experiments
- Small laboratories and portable imaging setups
- Any paradigm that can be defined with simple tone sequences or drifting-bar visual stimuli

ABISS is not optimized for experiments requiring multi-channel spatial audio, complex naturalistic visual stimuli, closed-loop behavioral control, or large-scale synchronized multi-channel acquisition, which may be better served by commercial systems or GPU-based stimulus platforms.

## Associated Paper

> Qu Z, Kazemi K, Wu T, Doddapujar SN, Marrazzo TA, Gazzola M, Gritton HJ. *ABISS: An Arduino-Based Intrinsic Stimulation System for Low-Cost Auditory and Visual Intrinsic Signal Imaging.* 2026.

Departments of Bioengineering, Electrical Engineering, Comparative Biosciences, and Mechanical Science and Engineering — University of Illinois Urbana-Champaign.
