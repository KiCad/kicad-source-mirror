# Allegro PCB importer worklog

The purpose of this work log is to record the development process of the KiCad Allegro importer for the purposes of demonstrating that my development did not use:

* non-free copyrighted information
* An installation of Allegro
* other non-public information

The materials used for this work are:

* A Kaitai Structs format description of the file format
    * This file was produced from the C++ struct layout of a previous GPL implementation
    * This file was written by John Beard
    * As a simple description of a format, the information is not subject to copyright other than in the presentation in the file itself.
* Open source Allegro project exports done by a third party (using the Altium conversion script from https://e2e.ti.com/support/processors-group/processors/f/processors-forum/1050931/faq-sk-tda4vm-can-somebody-provide-the-pcb-schematics-and-layout-from-sprr438-in-altium-format):
    * BeagleBone AI: https://github.com/beagleboard/beaglebone-ai
    * CutiePi v2: https://github.com/cutiepi-io/cutiepi-board
    * PreAmp: https://github.com/radualexandrub/ElectronicDevices-Circuits
    * Kinoma Create: https://github.com/Kinoma/kinoma-create-pcb

## Worklog

### 2025-05-05

* Started work on Allegro importer from the Kaitai Structs format description.
* The Kaitai Structs format description is located in `pcbnew/pcb_io/allegro/allegro_pcb.ksy`.

* The general import process will be:
  * Read the Allegro file into structures that represent the raw information in the file.
  * Convert the ensemble of these structures into KiCad objects.
* The outline of the plugin layout:
  * `allegro_stream`: provides a stream interface to decode primitive types from the Allegro file.
  * `allegro_pcb_structs`: provides low-level structures (A-Structs) that represent objects found in the Allegro file. Mostly these will follow the Kaitai Structs format description.
  * `allegro_parser`: will deal with at least converting the Allegro file into the `allegro_pcb_structs` A-structs. It may or may not later also handle the conversion to KiCad objects.

* A simple CLI tool to read the Allegro file and explore the contents of the structures is created under `utils/cli/allegro_cli`.

* Generally, A-Structs will be a 1:1 representation of the Allegro file format, as even unknown fields will read in because this may help later with debugging and understanding the file format.

* Version dependent fields use class `COND_FIELD` with a runtime version check. This documents the version dependency in the struct format.

* Implemented structs against the PreAmp project (file version 16.6) as a first step:
    * 0x06, 0x07, 0x0F, 0x10, which gets to offset 0x2a98 in the file.
    * 0x2B, 0x2D, 0x1B (nets) gets to 0x3B00

### 2024-05-06

* Implemented structs:
    * 0x1C (padstacks) and substructs - gets to 0x4e40 (next is 0x21 type)
    * 0x21 (headered block) - gets to 0xa9dc (next is 0x1D type)
    * 0x39, 0x3A - brings CutiePi up to 0x28e2c (next is 0x09)
    * 0x09 - brings CutiePi up to 0x28e58 (next is 0x0D)
    * 0x36 - some kind of of multi-type graphics (PreAmp to 0x130d4, next is 0x2a)
    * 0x2A - prings both PreAmp and CutiePi to the next 0x0D (pad)
