# Can be viewed in: https://ide.kaitai.io/
#
# This file is a formal specification of the binary format used in Altium SchDoc for the Storage file.
# Files need to manually extracted using a program which can read the Microsoft Compound File Format.
#
# While I do not create a parser using this file, it is still very helpful to understand the binary
# format.

meta:
  id: altium_storage
  endian: le
  encoding: UTF-8

seq:
  - id: properties_length
    type: u4
  - id: properties
    type: str
    size: properties_length
  - id: record
    type: record
    repeat: eos


types:
  record:
    seq:
      - size: 5
      - id: filename_length
        type: u1
      - id: filename
        type: str
        size: filename_length
      - id: data_length
        type: u4
      - id: data
        size: data_length