/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file  trace_helpers.h
 * @brief wxLogTrace helper definitions.
 */

#ifndef _TRACE_HELPERS_H_
#define _TRACE_HELPERS_H_

/**
 * @defgroup trace_env_vars Trace Environment Variables
 *
 * wxWidgets provides trace control of debug messages using the WXTRACE environment variable.
 * This section defines the strings passed to WXTRACE to for debug output control of various
 * sections of the KiCad code.  See the wxWidgets <a href="http://docs.wxwidgets.org/3.0/
 * group__group__funcmacro__log.html#ga947e317db477914c12b13c4534867ec9"> wxLogTrace </a>
 * documentation for more information.
 */

///@{
/// \ingroup trace_env_vars

/**
 * Flag to enable find debug tracing.
 */
extern const wxChar* const traceFindItem;

/**
 * Flag to enable find and replace debug tracing.
 */
extern const wxChar* const traceFindReplace;

/**
 * Flag to enable draw panel coordinate debug tracing.
 */
extern const wxChar* const kicadTraceCoords;


/**
 * Flag to enable wxKeyEvent debug tracing.
 */
extern const wxChar* const kicadTraceKeyEvent;

/**
 * Flag to enable auto save feature debug tracing.
 */
extern const wxChar* const traceAutoSave;

/**
 * Flag to enable schematic library memory deletion debug output.
 */
extern const wxChar* const traceSchLibMem;

/**
 * Flag to enable legacy schematic plugin debug output.
 */
extern const wxChar* const traceSchLegacyPlugin;

/**
 * Flag to enable GEDA PCB plugin debug output.
 */
extern const wxChar* const traceKicadPcbPlugin;

/**
 * Flag to enable GEDA PCB plugin debug output.
 */
extern const wxChar* const traceGedaPcbPlugin;

/**
 * Flag to enable print controller debug output.
 */
extern const wxChar* const tracePrinting;

/**
 * Flag to enable path and file name debug output.
 */
extern const wxChar* const tracePathsAndFiles;

///@}

/**
 * Debug helper for printing wxKeyEvent information.
 *
 * @param aEvent is the wxKeyEvent to generate the print string from.
 * @return the wxKeyEvent information.
 */
extern wxString dump( const wxKeyEvent& aEvent );

/**
 * Debug helper for printing wxArrayString contents.
 *
 * @param aArray is the string array to output.
 * @return the wxArrayString contents in a formatted string for debugging output.
 */
extern wxString dump( const wxArrayString& aArray );

#endif    // _TRACE_HELPERS_H_
