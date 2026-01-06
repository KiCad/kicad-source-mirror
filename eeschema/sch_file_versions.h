/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20210619  // Change pin overbar syntax from `~...~` to `~{...}`.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20211014  // Arc formatting.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220101  // Class flags.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220102  // Fonts.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220126  // Text boxes.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220328  // Text box start/end -> at/size.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220331  // Text colors.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220914  // Symbol unit display names.
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20220914  // Don't save property ID
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20230620  // ki_description -> Description Field
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20231120  // generator_version; V8 cleanups
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20240529  // Embedded Files
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20240819  // Embedded Files - Update hash algorithm to Murmur3
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20241209  // Private flags for SCH_FIELDs
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20250318  // ~ no longer means empty text
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20250324  // Jumper pin groups
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20250829  // Rounded Rectangles
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20250901  // Stacked Pin notation
//#define SEXPR_SYMBOL_LIB_FILE_VERSION  20250925  // Bus alias in Project File
#define SEXPR_SYMBOL_LIB_FILE_VERSION 20251024 // Updated properties formatting (do_not_autoplace, show_name)

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
//#define SEXPR_SCHEMATIC_FILE_VERSION 20210621  // Update overbar syntax in bus aliases.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20211123  // R/W uuids for junctions.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220101  // Circles, arcs, rects, polys & beziers
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220102  // Dash-dot-dot
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220103  // Label fields
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220104  // Fonts
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220124  // netclass_flag -> directive_label
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220126  // Text boxes
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220328  // Text box start/end -> at/size
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220331  // Text colors
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220404  // Default schematic symbol instance data.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220622  // New simulation model format.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220820  // Fix broken default symbol instance data.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220822  // Hyperlinks in text objects
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220903  // Field name visibility
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220904  // Do not autoplace field option
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220914  // Add support for DNP
//#define SEXPR_SCHEMATIC_FILE_VERSION 20220929  // Don't save property ID
//#define SEXPR_SCHEMATIC_FILE_VERSION 20221002  // Move instance data back into symbol definition.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20221004  // Move instance data back into symbol definition.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20221110  // Move sheet instance  data to sheet definition.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20221126  // Remove value and footprint from instance data.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20221206  // Simulation model fields V6 -> V7
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230121  // SCH_MARKER specific sheet path serialisation
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230221  // Modern power symbols (editable value = net)
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230409  // Add exclude_from_sim markup
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230620  // ki_description -> Description Field
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230808  // Move Sim.Enable field to exclude_from_sim attr
//#define SEXPR_SCHEMATIC_FILE_VERSION 20230819  // Allow multiple library symbol inheritance depth.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20231120  // generator_version; V8 cleanups
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240101  // Tables.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240417  // Rule areas
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240602  // Sheet attributes
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240620  // Embedded Files
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240716  // Multiple netclass assignments
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240812  // Netclass color highlighting
//#define SEXPR_SCHEMATIC_FILE_VERSION 20240819  // Embedded Files - Update hash algorithm to Murmur3
//#define SEXPR_SCHEMATIC_FILE_VERSION 20241004  // Use booleans for 'hide' in symbols
//#define SEXPR_SCHEMATIC_FILE_VERSION 20241209  // Private flags for SCH_FIELDs
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250114  // Full paths for text variable cross references
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250222  // Hatched fills for shapes
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250227  // Support for local power symbols
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250318  // ~ no longer means empty text
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250425  // uuids for tables
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250513  // Groups can have design block lib_id
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250610  // DNP, etc. flags for rule areas
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250827  // Custom body styles
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250829  // Rounded Rectangles
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250901  // Stacked Pin notation
//#define SEXPR_SCHEMATIC_FILE_VERSION 20250922  // Schematic variants.
//#define SEXPR_SCHEMATIC_FILE_VERSION 20251012  // Flat schematic hierarchy support
//#define SEXPR_SCHEMATIC_FILE_VERSION 20251028  // Updated properties formatting (do_not_autoplace, show_name)
#define SEXPR_SCHEMATIC_FILE_VERSION 20260101  // PCB variants
