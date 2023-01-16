# Forty-Niner-PLL
Another 49’er project with a digital VFO

![1 Forty Niner PLL in box](https://user-images.githubusercontent.com/17215772/212662253-904d3b2f-a5b2-466a-9a71-697ea7a8484d.jpg)

This project started as a basic 49er project, based on the QST article from March 2016, by Jack Purdum W8TEE et al.
"A Modular 40 Meter CW Transceiver with VFO".

Some changes were needed though.

The AD9850 DDS for the VFO has become difficult to source, and very expensive, so it was replaced by a Si5351 module.

A similar acrylic case for the project was found, it's the box of a video tape of the Philips VCR standard (1971).

This project is a work in progress, as we are still adding sompe stuff, but the software is already very capable.
Features that are already implemented are: 
selection of band and band limits, adjustable OFFSET with corresponding sidetone, PTT delay, WPM, 
Receive functions : RIT, SPLIT and SPOT.
Selection of pre-programmed CQ or Beacon messages, with adjustable beaconing time up to 8 hours.
All parameters, the last used frequencyand, the RIT and SPLIT status are stored in EEPROM in the Arduino.

The code includes a DDS routine for generation of a pure sinewave sidetone, and a calibration routine for the Si5351 to 1/100th of a Hz.

Credits

The hardware is a cooperation between Luc ON7DQ and Gil ONL12523.
The software is the work of Gil ONL12523.
The documentation was written by Luc ON7DQ
Contributions of other authors are credited in the source code files.