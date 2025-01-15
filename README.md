# GRAINMOTHER

A Multi-Effect Audio Engine based on the BELA Platform

Grainmother is a multi-effect audio engine built on the [**BELA**](https://bela.io) platform. It’s designed for musicians and sound designers who want to create unique sounds without relying on external software. The project combines granular synthesis, reverb, and ring modulation into a single device controlled hands-on, rather than through traditional pedal interfaces.

The goal of Grainmother is to provide a flexible, standalone effects processor optimized for live performances. Leveraging BELA’s real-time audio processing capabilities and a custom hardware interface, it offers intuitive controls for creative experimentation.

This project is fully open-source, with all code and schematics available for those interested in exploring or building upon the device.

## Features

Grainmother features three distinct audio effects, each designed to provide unique sound-shaping capabilities. 

The **Ring Modulator** combines the input signal with a carrier signal, using analog simulations such as diode-based, transistor-based, and hybrid approaches. An additional LFO modulates the carrier signal, while bit crushing and noise modulation extend the effect. This allows for a range of sounds, from lush amplitude modulation to extremely harsh, crushed noise.

The **Granulator** breaks audio into short grains ranging from 1 to 100 milliseconds. In addition to standard controls like Grain Length and Density, properties such as panning and length can be randomized. A delay, dynamic feedback, and filtering options provide powerful tools for intricate sound design. This versatile set of parameters enables everything from subtle enhancements to fragmented, evolving soundscapes.

The **Reverb** delivers a rich spatial effect by blending early reflections with a late reverberation algorithm inspired by Schroeder and Moorer. Selectable types, including Church, Digital Vintage, Seasick, and Room, allow users to craft immersive spaces and fine-tune parameters such as decay, pre-delay, and modulation. A unique feature is the feedback loop within the early reflection algorithm: lower values increase echo density artificially, while higher values create unique, morphing sounds.

The three effects can be processed in either **parallel** or **series** mode, with the user having the ability to define the processing order. Each effect features an individual Mix/Wet control, along with a master control for the entire effect engine. All processing is done **true stereo**. 

A dedicated **menu**, accessible via four buttons, allows for additional settings such as potentiometer behavior, MIDI input and output channels, and advanced effect parameters.

An **OLED display** provides real-time feedback, showing parameter changes and navigating the menu.

The Grainmother includes a built-in **preset system** that allows users to save and recall custom effect configurations. This feature provides quick access to stored settings, making it ideal for maintaining consistency during live performances or efficiently switching between sounds.

The device supports **full MIDI integration** via the Mini-USB connector on the side. Parameters can be controlled using Control Change messages, and presets can be switched using Program Change messages from external MIDI devices. Additionally, the eight potentiometers can function as MIDI controllers, sending out Control Change messages for further flexibility.

## Documentation

See the full documentation of the code [here](http://julianfuchs.ch/grainmother).

## License

This project is licensed under the [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/).

## How tu Use

I would be very happy to share further insights into my concepts and designs with you. Please feel free to reach out to me for more information. Additionally, if you are interested in the product, I would be pleased to create a custom device tailored to your needs.

For inquiries, please visit: [www.julianfuchs.ch](https://www.julianfuchs.ch)

## References

- **BELA.** 2024. [bela.io](https://bela.io)
- **Bencina, Ross.** 2001. *Implementing Real-Time Granular Synthesis.*
- **Dahl, Luke, and Jean-Marc Jot.** 2000. *A Reverberator Based on Absorbent All-Pass Filters.* Proceedings of the COST G-6 Conference on Digital Audio Effects (DAFX-00).
- **Dattorro, Jon.** 1997. *Effect Design, Part 1: Reverberator and Other Filters.* Journal of the Audio Engineering Society.
- **Hoffmann-Burchardi, Richard.** 2009. *Asymmetries Make the Difference: An Analysis of Transistor-Based Analog Ring Modulators.* Proc. of the 12th Int. Conference on Digital Audio Effects. Como, Italy.
- **Moorer, James A.** 1978. *About This Reverberation Business.* Computer Music Journal.
- **Opie, Tallaine.** 2024. *Granular Synthesis.* [granularsynthesis.com](http://www.granularsynthesis.com/). Accessed October 21, 2024.
- **Parker, Julian.** 2011. *A Simple Digital Model of the Diode-Based Ring-Modulator.* Proc. of the 14th International Conference on Digital Audio Effects. Paris, France.
- **Pirkle, Will C.** 2017. *Designing Audio Effect Plugins in C++.* New York: Routledge.
- **Pirkle, Will.** 2013. *Virtual Analog 2nd Order Moog Half-Ladder Filter - App Note 8.*
- **Schroeder, Manfred R.** 1962. *Natural Sounding Artificial Reverberation.* Journal of the Audio Engineering Society.
- **Truax, Barry.** 1988. *Real-Time Granular Synthesis with a Digital Signal Processor.* Computer Music Journal, Vol. 12, No. 2.
- **Välimäki, V., J. Parker, L. Savioja, J. O. Smith, and J. S. Abel.** 2012. *Fifty Years of Artificial Reverberation.* IEEE Transactions on Audio, Speech, and Language Processing, Vol. 20, No. 5.
- **Valhalla.** 2024. [Valhalla DSP](https://valhalladsp.com). Accessed December 31, 2024.


