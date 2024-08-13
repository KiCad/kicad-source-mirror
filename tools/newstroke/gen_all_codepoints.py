#! /usr/bin/env python
"""
Generates a list of all codepoints in the Unicode range, skipping the surrogate range.

This is useful for generating a list of codepoints to test the newstroke tool.
"""

s = ""

# Skip the surrogate range
ranges = [
    (0x0000, 0xD800),
    (0xE000, 0xFFEF),
]

for rng in ranges:

    for i in range(rng[0], rng[1]):

        if i % 256 == 0:
            if i != 0:
                s += "\n"
            s += f"U+{i:04X} "
        elif i % 16 == 0:
            s += " "

        if i < 32:
            s += " "
        else:
            s += chr(i)

print(s)