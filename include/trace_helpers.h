/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kicommon.h>
#include <wx/arrstr.h>
#include <wx/event.h>
#include <wx/string.h>

#include <stdarg.h>
#include <map>

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
 *
 * Use "KICAD_FIND_ITEM" to enable.
 */
extern KICOMMON_API const wxChar* const traceFindItem;

/**
 * Flag to enable find and replace debug tracing.
 *
 * Use "KICAD_FIND_REPLACE" to enable.
 */
extern KICOMMON_API const wxChar* const traceFindReplace;

/**
 * Flag to enable draw panel coordinate debug tracing.
 *
 * Use "KICAD_COORDS" to enable.
 */
extern KICOMMON_API const wxChar* const kicadTraceCoords;

/**
 * Flag to enable wxKeyEvent debug tracing.
 *
 * Use "KICAD_KEY_EVENTS" to enable.
 */
extern KICOMMON_API const wxChar* const kicadTraceKeyEvent;

/**
 * Flag to enable tracing of the tool handling stack.
 *
 * Use "KICAD_TOOL_STACK" to enable.
 */
extern KICOMMON_API const wxChar* const kicadTraceToolStack;

/**
 * Flag to enable tracing of the coroutine call stack.
 *
 * Use "KICAD_COROUTINE_STACK" to enable.
 */
extern KICOMMON_API const wxChar* const kicadTraceCoroutineStack;

/**
 * Flag to enable auto save feature debug tracing.
 *
 * Use "KICAD_AUTOSAVE" to enable.
 */
extern KICOMMON_API const wxChar* const traceAutoSave;

/**
 * Flag to enable schematic library memory deletion debug output.
 *
 * Use "KICAD_SCH_LIB_MEM" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchLibMem;

/**
 * Flag to enable legacy schematic plugin debug output.
 *
 * Use "KICAD_SCH_PLUGIN" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchPlugin;

/**
 * Flag to enable legacy schematic plugin debug output.
 *
 * Use "KICAD_SCH_LEGACY_PLUGIN" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchLegacyPlugin;

/**
 * Flag to enable KiCad PCB plugin debug output.
 *
 * Use "KICAD_PCB_PLUGIN" to enable.
 */
extern KICOMMON_API const wxChar* const traceKicadPcbPlugin;

/**
 * Flag to enable GEDA PCB plugin debug output.
 *
 * Use "KICAD_GEDA_PLUGIN" to enable.
 */
extern KICOMMON_API const wxChar* const traceGedaPcbPlugin;

/**
 * Flag to enable print controller debug output.
 *
 * Use "KICAD_PRINT" to enable.
 */
extern KICOMMON_API const wxChar* const tracePrinting;

/**
 * Flag to enable path and file name debug output.
 *
 * Use "KICAD_PATHS_AND_FILES" to enable.
 */
extern KICOMMON_API const wxChar* const tracePathsAndFiles;

/**
 * Flag to enable locale debug output.
 *
 * Use "KICAD_LOCALE" to enable.
 */
extern KICOMMON_API const wxChar* const traceLocale;

/**
 * Flag to enable locale debug output.
 *
 * Use "KICAD_FONTS" to enable.
 */
extern KICOMMON_API const wxChar* const traceFonts;

/**
 * Flag to enable debug output of #BASE_SCREEN and it's derivatives.
 *
 * Use "KICAD_SCREEN" to enable.
 */
extern KICOMMON_API const wxChar* const traceScreen;

/**
 * Flag to enable debug output of display positioning logic.
 *
 * Use "KICAD_DISPLAY_LOCATION" to enable.
 */
extern KICOMMON_API const wxChar* const traceDisplayLocation;

/**
 * Flag to enable debug output of zoom-scrolling calculations in
 * #KIGFX::ZOOM_CONTROLLER and derivatives.
 *
 * Use "KICAD_ZOOM_SCROLL" to enable.
 */
extern KICOMMON_API const wxChar* const traceZoomScroll;

/**
 * Flag to enable debug output of symbol library resolver results
 *
 * Use "KICAD_SYM_RESOLVE" to enable.
 */
extern KICOMMON_API const wxChar* const traceSymbolResolver;

/**
 * Flag to enable debug output of schematic symbol sheet path manipulation code.
 *
 * Use "KICAD_SCH_SHEET_PATHS" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchSheetPaths;

/**
 * Flag to enable debug output of current sheet tracking in the schematic editor.
 *
 * Use "KICAD_SCH_CURRENT_SHEET" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchCurrentSheet;

/**
 * Flag to enable debug output of schematic field rendering and positioning.
 *
 * Use "KICAD_SCH_FIELD_RENDERING" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchFieldRendering;

/**
 * Flag to enable debug output of schematic painter operations.
 *
 * Use "KICAD_SCH_PAINTER" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchPainter;

/**
 * Flag to enable debug output of schematic symbol reference resolution.
 *
 * Use "KICAD_SCH_SYMBOL_REF" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchSymbolRef;

/**
 * Flag to enable debug output of schematic drag net collision detection.
 *
 * Use "KICAD_SCH_DRAG_NET_COLLISION" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchDragNetCollision;

/**
 * Flag to enable debug output for cross-probe flash operations.
 *
 * Use "CROSS_PROBE_FLASH" to enable.
 */
extern KICOMMON_API const wxChar* const traceCrossProbeFlash;

/**
 * Flag to enable debug output of environment variable operations.
 *
 * Use "KICAD_ENV_VARS" to enable.
 */
extern KICOMMON_API const wxChar* const traceEnvVars;

/**
 * Flag to enable debug output of GAL performance profiling.
 *
 * Use "KICAD_GAL_PROFILE" to enable.
 */
extern KICOMMON_API const wxChar* const traceGalProfile;

/**
 * Flag to enable KiCad2Step debug tracing.
 *
 * Use "KICAD2STEP" to enable.
 */
extern KICOMMON_API const wxChar* const traceKiCad2Step;

/**
 * Flag to enable user interface profile tracing.
 *
 * Use "KICAD_UI_PROFILE" to enable.
 */
extern KICOMMON_API const wxChar* const traceUiProfile;

/**
 * Flag to enable Git debugging output.
 *
 * Use "KICAD_GIT" to enable.
 */
extern KICOMMON_API const wxChar* const traceGit;

/*
 * Flag to enable Eagle I/O debug tracing.
 *
 * Use "KICAD_EAGLE_IO" to enable.
 */
extern KICOMMON_API const wxChar* const traceEagleIo;

/*
 * Flag to enable Design Block O debug tracing.
 *
 * Use "KICAD_EAGLE_IO" to enable.
 */
extern KICOMMON_API const wxChar* const traceDesignBlocks;

/*
 * Flag to enable library field table debug tracing.
 *
 * Use "KICAD_LIB_FIELD_TABLE" to enable.
 */
extern KICOMMON_API const wxChar* const traceLibFieldTable;

/**
 * Flag to enable PDF plotter debug tracing.
 *
 * Use "KICAD_PDF_PLOTTER" to enable.
 */
extern KICOMMON_API const wxChar* const tracePdfPlotter;

/**
 * Flag to enable snap/grid helper debug tracing.
 *
 * Use "KICAD_SNAP" to enable.
 */
extern KICOMMON_API const wxChar* const traceSnap;

/**
 * Flag to enable debug output for stacked pins handling in symbol/pin code.
 *
 * Use "KICAD_STACKED_PINS" to enable.
 */
extern KICOMMON_API const wxChar* const traceStackedPins;

/**
 * Flag to enable debug output for library file watch refreshes.
 *
 * Use "KICAD_LIB_WATCH" to enable.
 */
extern KICOMMON_API const wxChar* const traceLibWatch;

/**
 * Flag to enable library table and library manager tracing.
 * use "KICAD_LIBRARIES" to enable.
 */
extern KICOMMON_API const wxChar* const traceLibraries;

/**
 * Flag to watch how schematic move tool actions are handled.
 *
 * Use "KICAD_SCH_MOVE" to enable.
 */
extern KICOMMON_API const wxChar* const traceSchMove;

/**
 * Flag to enable tracing of circular symbol inheritance detection.
 *
 * Use "KICAD_SYMBOL_INHERITANCE" to enable.
 */
extern KICOMMON_API const wxChar* const traceSymbolInheritance;

///@}

/**
 * Debug helper for printing wxKeyEvent information.
 *
 * @param aEvent is the wxKeyEvent to generate the print string from.
 * @return the wxKeyEvent information.
 */
extern KICOMMON_API wxString dump( const wxKeyEvent& aEvent );

/**
 * Debug helper for printing wxArrayString contents.
 *
 * @param aArray is the string array to output.
 * @return the wxArrayString contents in a formatted string for debugging output.
 */
extern KICOMMON_API wxString dump( const wxArrayString& aArray );

class KICOMMON_API TRACE_MANAGER
{
public:
    TRACE_MANAGER() :
        m_globalTraceEnabled( false ),
        m_printAllTraces (false )
    {};
    ~TRACE_MANAGER(){};

    static TRACE_MANAGER& Instance();

    WX_DEFINE_VARARG_FUNC_VOID( Trace, 2, (const wxString, const wxFormatString&), DoTrace,
                                DoTraceUtf8 )

    void DoTrace( const wxString aWhat, const wxChar* aFmt, ... )
    {
        va_list argptr;
        va_start( argptr, aFmt );
        traceV( aWhat, aFmt, argptr );
        va_end( argptr );
    }

#if wxUSE_UNICODE_UTF8
    void DoTraceUtf8( const wxString aWhat, const wxChar* aFmt, ... )
    {
        va_list argptr;
        va_start( argptr, aFmt );
        traceV( aWhat, aFmt, argptr );
        va_end( argptr );
    }
#endif

    bool IsTraceEnabled( const wxString& aWhat );

private:
    void traceV( const wxString& aWhat, const wxString& aFmt, va_list vargs );
    void init();

    std::map<wxString, bool> m_enabledTraces;
    bool                     m_globalTraceEnabled;
    bool                     m_printAllTraces;
};

#define KI_TRACE( aWhat, ... )                                                                     \
    if( TRACE_MANAGER::Instance().IsTraceEnabled( aWhat ) )                                        \
    {                                                                                              \
        TRACE_MANAGER::Instance().Trace( aWhat, __VA_ARGS__ );                                     \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
    }

#endif    // _TRACE_HELPERS_H_
