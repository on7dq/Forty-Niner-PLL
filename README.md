# Forty-Niner-PLL
Another 49’er project with a digital VFO

![image](https://user-images.githubusercontent.com/17215772/211576822-f3ac75cb-22e6-434e-8b8a-b635ca80044b.png)


This project started as a basic 49er project, based on the QST article from March 2016, by Jack Purdum W8TEE et al.
"A Modular 40 Meter CW Transceiver with VFO".

Some changes were neeeded though.

The AD9850 DDS for the VFO has become difficult to source, and very expensive, so it was replaced by a Si5351 module.

A similar acrylic case for the project was found, it's the box of a video tape of the Philips VCR standard.

This project is a work in progress, as we are still adding sompe stuff, but the software is already very capable.
Features that are already implemented are: 
selection of band and band limits, one preprogrammed CQ message, adjustable OFFSET with corresponding sidetone, 
PTT delay, CQ WPM, RIT, SPLIT, SPOT, etc ...
All parameters and the last used frequency are stored in EEPROM in the Arduino.

The code includes a DDS for sidetone generation as a pure sinewave, a calibration routine for the Si5351 to 1/100th of a Hz.

Credits

The hardware is a cooperation between Luc ON7DQ and Gil ONL12523.
The software is the work of Gil ONL12523.
The documentation was written by Luc ON7DQ
