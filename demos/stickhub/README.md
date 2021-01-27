# StickHub

By Stefan Hamminga
https://rbts.co
See LICENSE.md for license details

The Space Friendly 7-Port USB 2 Hub - KiCAD design files

A tiny (16.5 x 40mm) USB 2.0 hub with 7 ports for connecting lots of USB devices to, for example, a Raspberry Pi or Jetson board. Each port uses a JST SH 1.0mm pitch connector, making a hub assembly far more compact and lightweight than possible with regular USB plugs.

In contrast to cheap hubs each port supports LS, FS and HS independently for optimal performance of all devices. Power is taken from the upstream port by default, but removing the fuse (or replacing it by a diode) allows using external 5V supply.

Full ESD protection: A TVS per data line and a 2012 capacitor per power line. Inrush current control and short circuit protection. 308uF downstream capacitance. Individual status LEDs for the downsteam ports.

