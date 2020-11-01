# KnokooPedalCtrl

A small modification that allows the Knokoo FES150 Fume Extractor to be turned on
and off using a 1/4" foot pedal.

![pcb render](Doc/pcb_render.jpg)

## Why?

The way my bench is setup, I cannot comfortably reach the speed control/power knob.
Life's very difficult, tell me about it. 

But because I know myself well enough to know that I won't use this (or any) fume extractor
if I have to crawl on the floor every time I want to solder something, this small project
was born.

It is also not the quietest machine, so being able to turn it on and off only when actually 
soldering has made using it much more pleasant. 


## How?

This small PIC12F1572-based board sits between the Knokoo's Main PCB and the front itensity knob.  

The Knokoo turns itself off when the intensity knob is turned all the way off.
This board interrupts the 5V rail normally provided to the potentiometer by the Knokoo PCB, and only
enables it when the foot pedal is pressed.

This way the Intensity Knob can permanently be left set to the desired intensity, and the pedal
be used to turn the Unit on and off. 

Pressing the pedal once turns the unit on and starts a timer controlled by the position of a second
potentiometer. The unit will shut off when either the timer runs out or the pedal is pressed again.

Pressing the pedal twice in short succession turns the unit on permanently until the pedal is pressed
again.

If the pedal is unplugged, the board will disable itself and the unit will function as before.

## Installation

[Detailed Installation Instructions will follow]

[Cables/Connections]

[Pedal Sense]

[Time Poti bypass]

