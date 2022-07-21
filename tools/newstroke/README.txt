Newstroke Font Readme
=====================

Newstroke is a stroke (plotter) font originally designed for KiCAD.

Project homepage: http://vovanium.ru/sledy/newstroke

Files
-----
font.lib         - main glyph library in KiCAD library format
symbol.lib       - glyph library for most math, tech and other symbols
font_draft1.lib  - old draft glyph library with the metrics from Hersheys Simplex
font.pro         - KiCAD project
charlist.txt     - unicode glyph map list
fontconv.awk     - AWK script for 'compiling' project to c-source used by KiCAD
newstroke_font.h - generated c header with font

Requirements
------------
KiCAD (http://kicad.sourceforge.net/) - for glyph editing
AWK - for font generating

Usage
-----
* Edit glyps with KiCAD EESchema library editor.
* Add Unicode positions to charlist.
* Generate font using following command line:

awk -f fontconv.awk symbol.lib font.lib charlist.txt >newstroke_font.h


Released under CC0 licence.
