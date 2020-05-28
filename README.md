BKM-129X-MCU

An attempt to replicate the workings of the microcontroller on
the Sony BKM-129X RGB/Component option card.

Copy if you wanna, but cool kids give credit where due...

I did not write digitalWriteFast.h it was found here:
https://github.com/NicksonYap/digitalWriteFast
I could find no apparent license so if the authors does
not wish me to include it, please email me...

Version 1.0 confirmed working on:
PVM-9L2
PVM-14L4
PVM-20L5
BVM-D14H5U
BVM-D9H5U

"Working" as in, the monitors detect the card as a BKM-129X.
The different monitors are more or less picky regarding the 
signals but that is up to the board that implements this part
of the complete BKM-129X board. For examples of this, see
https://github.com/skumlos/bkm-129x-simple

Special credit goes out to Nick Carney and Bob from RetroRGB
for rigorious testing and being patient with my software :)

Copyright © 2020 Martin Hejnfelt<martin@hejnfelt.com>
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.

The license only covers bkm-129x.ino
