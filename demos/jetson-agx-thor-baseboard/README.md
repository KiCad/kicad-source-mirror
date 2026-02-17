# Jetson AGX Thor Baseboard

Copyright (c) 2025 [Antmicro](https://www.antmicro.com)

![Jetson Thor Baseboard](img/jetson-thor-baseboard-render.png)

## Overview

This project contains open hardware design files for a baseboard supporting [NVIDIA Jetson AGX Thor (T5000)](https://www.nvidia.com/en-us/autonomous-machines/embedded-systems/jetson-thor/) System on Module.
The baseboard break-routes common IO-interfaces for standard desktop usage.
The baseboard also features an FMC-style expansion connector, so it is possible to combine it with other baseboards and custom-designed accessories, in order to accelerate the development of new Thor-based products.
The PCB design files were prepared in KiCad 9.x.
The PCB layout design is currently in progress.

## Key features

* 2 x 50-pin FFC connectors exposing 4 x 4-lane CSI ports from the SoM with independent I2C configuration buses per each CSI port
* SFP port exposing Jetson Thor XFI interface for fiber or copper modules
* M.2 (key-M) for NVMe storage (suggested usage)
* M.2 (key-E) for wireless adapters
* FMC+ expansion connector with PCIe x2, PCIe x1, DisplayPort, CAN, UART, SPI, I2S
* On-board USB 3.2 hub with two downstream-facing ports exposed on USB-C connectors
* Recovery USB-C port for writing Board Support Package (BSP) images to the SoM
* Generic USB-C port which supports video ALT mode and 100 W USB PD sink/source
* Debug USB-C port with debug console and 100 W USB PD sink/source support
* General purpose on-board 8KB EEPROM
* On-board Infineon/SLB9673AU20FW2610XTMA1 TPM
* Two user buttons
* Two user LEDs
* FAN connector
* 100 x 180 mm (3.93 x 7.09 inch) PCB outline

The Jeston AGX Thor Baseboard has two 50-pin FFC connectors which makes it electrically compatible with a variety of camera modules and video accessories developed by Antmicro. These are:

* [GMSL Deserializer Board](https://github.com/antmicro/gmsl-deserializer)
* [SDI-MIPI Video Converter](https://github.com/antmicro/sdi-mipi-video-converter-hw)
* [HDMI to MIPI CSI-2 Bridge](https://github.com/antmicro/hdmi-mipi-bridge)
* [Composite Video to MIPI CSI-2 Bridge](https://github.com/antmicro/cvbs-mipi-bridge)

In a basic configuration, the Jetson AGX Thor Baseboard should be powered with a DC source (24VDC, >130W) through a DC (Molex Mini-Fit) locking connector.

## Project structure

The main directory contains KiCad PCB project files, the LICENSE, and a README.
The remaining files are stored in the following directories:

* `img` - contains graphics for this README

## Licensing

This project is published under the [Apache-2.0](LICENSE) license.

