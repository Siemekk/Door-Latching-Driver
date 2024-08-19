# Door-Latching-Driver
Special driver for implementing the Soft Close system known from Audi, for other VAG group brands.

## PCB
PCB was full designed by my. Includes new automotive sockets for being more similar to full-comercial projects.
Program is being loaded by special ISP socket for programing. Dimension of PCB is ~62x49mm, so you can easliy put in inside Car.
PCB can be expanded to next options (For Example, closing mirrors after close Car), but I don't wanna do it, because all Cars after 2008 year, have this option in standard.

## Tests
Tests in old car (Audi A6 4F - 2006 year), was passed exactly! All things works like a new Car from 2022 year! The current consumption in sleep mode is less than <1mA, so battery is protected.
Breaking a process of latching door did not crash driver, driver wait for "full open door", and after that tries to latch it again!
In situation, where door "bowdeen cable" is broken, driver tries to latch door, but when status of door is not changed to "CLOSE", driver wait's for Open Doors, but don't REVERSE ENGINE. it's a sign to the user that something is wrong

## Assembly Instruction (Current in Polish)
![alt text](https://i.imgur.com/eIppLty.jpeg)
