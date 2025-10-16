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
 * @file  trace_helpers.cpp
 * @brief wxLogTrace helper implementation.
 */

#include <trace_helpers.h>

#include <wx/tokenzr.h>

const wxChar* const traceFindReplace = wxT( "KICAD_FIND_REPLACE" );
const wxChar* const kicadTraceCoords = wxT( "KICAD_COORDS" );
const wxChar* const kicadTraceKeyEvent = wxT( "KICAD_KEY_EVENTS" );
const wxChar* const kicadTraceToolStack = wxT( "KICAD_TOOL_STACK" );
const wxChar* const kicadTraceCoroutineStack = wxT( "KICAD_COROUTINE_STACK" );
const wxChar* const traceSchLibMem = wxT( "KICAD_SCH_LIB_MEM" );
const wxChar* const traceFindItem = wxT( "KICAD_FIND_ITEM" );
const wxChar* const traceSchLegacyPlugin = wxT( "KICAD_SCH_LEGACY_PLUGIN" );
const wxChar* const traceSchPlugin = wxT( "KICAD_SCH_PLUGIN" );
const wxChar* const traceGedaPcbPlugin = wxT( "KICAD_GEDA_PLUGIN" );
const wxChar* const traceKicadPcbPlugin = wxT( "KICAD_PCB_PLUGIN" );
const wxChar* const tracePrinting = wxT( "KICAD_PRINT" );
const wxChar* const traceAutoSave = wxT( "KICAD_AUTOSAVE" );
const wxChar* const tracePathsAndFiles = wxT( "KICAD_PATHS_AND_FILES" );
const wxChar* const traceLocale = wxT( "KICAD_LOCALE" );
const wxChar* const traceFonts = wxT( "KICAD_FONTS" );
const wxChar* const traceScreen = wxT( "KICAD_SCREEN" );
const wxChar* const traceZoomScroll = wxT( "KICAD_ZOOM_SCROLL" );
const wxChar* const traceSymbolResolver = wxT( "KICAD_SYM_RESOLVE" );
const wxChar* const traceDisplayLocation = wxT( "KICAD_DISPLAY_LOCATION" );
const wxChar* const traceSchSheetPaths = wxT( "KICAD_SCH_SHEET_PATHS" );
const wxChar* const traceSchCurrentSheet = wxT( "KICAD_SCH_CURRENT_SHEET" );
const wxChar* const traceSchFieldRendering = wxT( "KICAD_SCH_FIELD_RENDERING" );
const wxChar* const traceSchPainter = wxT( "KICAD_SCH_PAINTER" );
const wxChar* const traceSchSymbolRef = wxT( "KICAD_SCH_SYMBOL_REF" );
const wxChar* const traceEnvVars = wxT( "KICAD_ENV_VARS" );
const wxChar* const traceSchDragNetCollision = wxT( "KICAD_SCH_DRAG_NET_COLLISION" );
const wxChar* const traceCrossProbeFlash = wxT( "CROSS_PROBE_FLASH" );
const wxChar* const traceGalProfile = wxT( "KICAD_GAL_PROFILE" );
const wxChar* const traceStackedPins = wxT( "KICAD_STACKED_PINS" );
const wxChar* const traceLibWatch = wxT( "KICAD_LIB_WATCH" );
const wxChar* const traceKiCad2Step = wxT( "KICAD2STEP" );
const wxChar* const traceUiProfile = wxT( "KICAD_UI_PROFILE" );
const wxChar* const traceGit = wxT( "KICAD_GIT" );
const wxChar* const traceEagleIo = wxT( "KICAD_EAGLE_IO" );
const wxChar* const traceDesignBlocks = wxT( "KICAD_DESIGN_BLOCK" );
const wxChar* const traceLibFieldTable = wxT( "KICAD_LIB_FIELD_TABLE" );
const wxChar* const tracePdfPlotter = wxT( "KICAD_PDF_PLOTTER" );
const wxChar* const traceSnap = wxT( "KICAD_SNAP" );
const wxChar* const traceLibraries = wxT( "KICAD_LIBRARIES" );


wxString dump( const wxArrayString& aArray )
{
    wxString tmp;

    for( unsigned i = 0;  i < aArray.GetCount();  i++ )
    {
        if( aArray[i].IsEmpty() )
            tmp << wxT( "\"\" " );
        else
            tmp << aArray[i] << wxT( " " );
    }

    return tmp;
}


// The following code was shamelessly copied from the wxWidgets keyboard sample
// at https://github.com/wxWidgets/wxWidgets/blob/master/samples/keyboard/keyboard.cpp.

/////////////////////////////////////////////////////////////////////////////
// Author:      Vadim Zeitlin
// Modified by: Marcin Wojdyr
// Created:     07.04.02
// Copyright:   (c) 2002 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// helper function that returns textual description of wx virtual keycode
const char* GetVirtualKeyCodeName(int keycode)
{
    switch ( keycode )
    {
#define WXK_(x) \
        case WXK_##x: return #x;

        WXK_(BACK)
        WXK_(TAB)
        WXK_(RETURN)
        WXK_(ESCAPE)
        WXK_(SPACE)
        WXK_(DELETE)
        WXK_(START)
        WXK_(LBUTTON)
        WXK_(RBUTTON)
        WXK_(CANCEL)
        WXK_(MBUTTON)
        WXK_(CLEAR)
        WXK_(SHIFT)
        WXK_(ALT)
        WXK_(CONTROL)
        WXK_(MENU)
        WXK_(PAUSE)
        WXK_(CAPITAL)
        WXK_(END)
        WXK_(HOME)
        WXK_(LEFT)
        WXK_(UP)
        WXK_(RIGHT)
        WXK_(DOWN)
        WXK_(SELECT)
        WXK_(PRINT)
        WXK_(EXECUTE)
        WXK_(SNAPSHOT)
        WXK_(INSERT)
        WXK_(HELP)
        WXK_(NUMPAD0)
        WXK_(NUMPAD1)
        WXK_(NUMPAD2)
        WXK_(NUMPAD3)
        WXK_(NUMPAD4)
        WXK_(NUMPAD5)
        WXK_(NUMPAD6)
        WXK_(NUMPAD7)
        WXK_(NUMPAD8)
        WXK_(NUMPAD9)
        WXK_(MULTIPLY)
        WXK_(ADD)
        WXK_(SEPARATOR)
        WXK_(SUBTRACT)
        WXK_(DECIMAL)
        WXK_(DIVIDE)
        WXK_(F1)
        WXK_(F2)
        WXK_(F3)
        WXK_(F4)
        WXK_(F5)
        WXK_(F6)
        WXK_(F7)
        WXK_(F8)
        WXK_(F9)
        WXK_(F10)
        WXK_(F11)
        WXK_(F12)
        WXK_(F13)
        WXK_(F14)
        WXK_(F15)
        WXK_(F16)
        WXK_(F17)
        WXK_(F18)
        WXK_(F19)
        WXK_(F20)
        WXK_(F21)
        WXK_(F22)
        WXK_(F23)
        WXK_(F24)
        WXK_(NUMLOCK)
        WXK_(SCROLL)
        WXK_(PAGEUP)
        WXK_(PAGEDOWN)
        WXK_(NUMPAD_SPACE)
        WXK_(NUMPAD_TAB)
        WXK_(NUMPAD_ENTER)
        WXK_(NUMPAD_F1)
        WXK_(NUMPAD_F2)
        WXK_(NUMPAD_F3)
        WXK_(NUMPAD_F4)
        WXK_(NUMPAD_HOME)
        WXK_(NUMPAD_LEFT)
        WXK_(NUMPAD_UP)
        WXK_(NUMPAD_RIGHT)
        WXK_(NUMPAD_DOWN)
        WXK_(NUMPAD_PAGEUP)
        WXK_(NUMPAD_PAGEDOWN)
        WXK_(NUMPAD_END)
        WXK_(NUMPAD_BEGIN)
        WXK_(NUMPAD_INSERT)
        WXK_(NUMPAD_DELETE)
        WXK_(NUMPAD_EQUAL)
        WXK_(NUMPAD_MULTIPLY)
        WXK_(NUMPAD_ADD)
        WXK_(NUMPAD_SEPARATOR)
        WXK_(NUMPAD_SUBTRACT)
        WXK_(NUMPAD_DECIMAL)
        WXK_(NUMPAD_DIVIDE)

        WXK_(WINDOWS_LEFT)
        WXK_(WINDOWS_RIGHT)
#ifdef __WXOSX__
        WXK_(RAW_CONTROL)
#endif
#undef WXK_

    default:
        return nullptr;
    }
}


// helper function that returns textual description of key in the event
wxString GetKeyName( const wxKeyEvent &aEvent )
{
    int keycode = aEvent.GetKeyCode();
    const char* virt = GetVirtualKeyCodeName( keycode );

    if( virt )
        return virt;

    if( keycode > 0 && keycode < 32 )
        return wxString::Format( "Ctrl-%c", (unsigned char)('A' + keycode - 1) );

    if( keycode >= 32 && keycode < 128 )
        return wxString::Format( "'%c'", (unsigned char)keycode );

#if wxUSE_UNICODE
    int uc = aEvent.GetUnicodeKey();

    if( uc != WXK_NONE )
        return wxString::Format( "'%c'", uc );
#endif

    return "unknown";
}


wxString dump( const wxKeyEvent& aEvent )
{
    wxString msg;
    wxString eventType = wxS( "unknown" );

    if( aEvent.GetEventType() == wxEVT_KEY_DOWN )
        eventType = wxS( "KeyDown" );
    else if( aEvent.GetEventType() == wxEVT_KEY_UP )
        eventType = wxS( "KeyUp" );
    else if( aEvent.GetEventType() == wxEVT_CHAR )
        eventType = wxS( "Char" );
    else if( aEvent.GetEventType() == wxEVT_CHAR_HOOK )
        eventType = wxS( "Hook" );

    // event  key_name  KeyCode  modifiers  Unicode  raw_code raw_flags pos
    msg.Printf( "%7s %15s %5d   %c%c%c%c"
#if wxUSE_UNICODE
                "%5d (U+%04x)"
#else
                "    none   "
#endif
#ifdef wxHAS_RAW_KEY_CODES
                "  %7lu    0x%08lx"
#else
                "  not-set    not-set"
#endif
                "  (%5d,%5d)",
                eventType,
                GetKeyName( aEvent ),
                aEvent.GetKeyCode(),
                aEvent.ControlDown() ? 'C' : '-',
                aEvent.AltDown()     ? 'A' : '-',
                aEvent.ShiftDown()   ? 'S' : '-',
                aEvent.MetaDown()    ? 'M' : '-'
#if wxUSE_UNICODE
                , aEvent.GetUnicodeKey()
                , aEvent.GetUnicodeKey()
#endif
#ifdef wxHAS_RAW_KEY_CODES
                , (unsigned long) aEvent.GetRawKeyCode()
                , (unsigned long) aEvent.GetRawKeyFlags()
#endif
                , aEvent.GetX()
                , aEvent.GetY()
        );

    return msg;
}


TRACE_MANAGER& TRACE_MANAGER::Instance()
{
    static TRACE_MANAGER* self = nullptr;

    if( !self )
    {
        self = new TRACE_MANAGER;
        self->init();
    }

    return *self;
}


bool TRACE_MANAGER::IsTraceEnabled( const wxString& aWhat )
{
    if( !m_printAllTraces )
    {
        if( !m_globalTraceEnabled )
            return false;

        if( m_enabledTraces.find( aWhat ) == m_enabledTraces.end() )
            return false;
    }

    return true;
}


void TRACE_MANAGER::traceV( const wxString& aWhat, const wxString& aFmt, va_list vargs )
{
    if( !IsTraceEnabled( aWhat ) )
        return;

    wxString str;
    str.PrintfV( aFmt, vargs );

#if defined( __UNIX__ ) || defined( _WIN32 )
    fprintf( stderr, " %-30s | %s", aWhat.c_str().AsChar(), str.c_str().AsChar() );
#endif
}


void TRACE_MANAGER::init()
{
    wxString traceVars;
    m_globalTraceEnabled = wxGetEnv( wxT( "KICAD_TRACE" ), &traceVars );
    m_printAllTraces = false;

    if( !m_globalTraceEnabled )
        return;

    wxStringTokenizer tokenizer( traceVars, "," );

    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        m_enabledTraces[token] = true;

        if( token.Lower() == wxT( "all" ) )
            m_printAllTraces = true;
    }
}
