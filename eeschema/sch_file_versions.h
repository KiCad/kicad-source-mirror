/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 CERN
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * This file contains the file format version information for the s-expression schematic
 * and symbol library file formats.
 *
 * @note Comment out the last version and add the new version as a date time stamp in the
 *       YYYYMMDD format.  Comment the changes to the file format for historical purposes.
 */

/**
 * Symbol library file version.
 */
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20200126  // Initial version.  Add alternate pin
                                                   // definitions.

//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20200820

//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20200827  // Remove host tag.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20200908  // Add include in BOM and on board support.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20201005  // Separate ki_fp_filters by spaces.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION 20210619   // Change pin overbar syntax from `~...~` to `~{...}`.
#define SEXPR_SYMBOL_LIB_FILE_VERSION 20211014   // Arc formatting.


/**
 * Schematic file version.
 */
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200310  // Initial version.  Sheet fields were named
                                                 // incorrectly (using symbol field vocabulary).
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200506  // Used "page" instead of "paper" for paper
                                                 // sizes.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200512  // Add support for exclude from BOM.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200602  // Add support for exclude from board.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200608  // Add support for bus and junction properties.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200618  // Disallow duplicate field ids.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200714  // Add alternate pin definitions.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200820
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200827  // Remove host tag.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20200828  // Add footprint to symbol_instances.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20201015  // Add sheet instance properties.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210123  // Rename "unconnected" pintype to "no_connect".
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210125  // R/W uuids for pins, labels, wires, etc.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210126  // Fix bug with writing pin uuids.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210406  // Add schematic level uuids.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210606  // Change overbar syntax from `~...~` to `~{...}`.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210615  // Update overbar syntax in net names.
#define SEXPR_SCHEMATIC_FILE_VERSION 20210621  // Update overbar syntax in bus aliases.
