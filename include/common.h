/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * The common library
 * @file common.h
 */

#ifndef __INCLUDE__COMMON_H__
#define __INCLUDE__COMMON_H__ 1

#include "wx/wx.h"
#include "wx/confbase.h"
#include "wx/fileconf.h"


#ifdef KICAD_NANOMETRE
#include "length.h"
#endif

class wxAboutDialogInfo;
class BASE_SCREEN;
class EDA_DRAW_FRAME;
class EDA_DRAW_PANEL;

/* Flag for special keys */
#define GR_KB_RIGHTSHIFT 0x10000000                 /* Keybd states: right
                                                     * shift key depressed */
#define GR_KB_LEFTSHIFT  0x20000000                 /* left shift key depressed
                                                     */
#define GR_KB_CTRL       0x40000000                 /* CTRL depressed */
#define GR_KB_ALT        0x80000000                 /* ALT depressed */
#define GR_KB_SHIFT      (GR_KB_LEFTSHIFT | GR_KB_RIGHTSHIFT)
#define GR_KB_SHIFTCTRL  (GR_KB_SHIFT | GR_KB_CTRL)
#define MOUSE_MIDDLE     0x08000000                 /* Middle button mouse
                                                     * flag for block commands
                                                     */

// default name for nameless projects
#define NAMELESS_PROJECT wxT( "noname" )

#define NB_ITEMS 11

/* Pseudo key codes for command panning */
enum pseudokeys {
    EDA_PANNING_UP_KEY = 1,
    EDA_PANNING_DOWN_KEY,
    EDA_PANNING_LEFT_KEY,
    EDA_PANNING_RIGHT_KEY,
    EDA_ZOOM_IN_FROM_MOUSE,
    EDA_ZOOM_OUT_FROM_MOUSE,
    EDA_ZOOM_CENTER_FROM_MOUSE
};

#define ESC 27

/* TODO Executable names TODO*/
#ifdef __WINDOWS__
#define CVPCB_EXE           wxT( "cvpcb.exe" )
#define PCBNEW_EXE          wxT( "pcbnew.exe" )
#define EESCHEMA_EXE        wxT( "eeschema.exe" )
#define GERBVIEW_EXE        wxT( "gerbview.exe" )
#define BITMAPCONVERTER_EXE wxT( "bitmap2component.exe" )
#define PCB_CALCULATOR_EXE  wxT( "pcb_calculator.exe" )
#else
#ifndef __WXMAC__
#define CVPCB_EXE           wxT( "cvpcb" )
#define PCBNEW_EXE          wxT( "pcbnew" )
#define EESCHEMA_EXE        wxT( "eeschema" )
#define GERBVIEW_EXE        wxT( "gerbview" )
#define BITMAPCONVERTER_EXE wxT( "bitmap2component" )
#define PCB_CALCULATOR_EXE  wxT( "pcb_calculator" )
#else
#define CVPCB_EXE           wxT( "cvpcb.app/Contents/MacOS/cvpcb" )
#define PCBNEW_EXE          wxT( "pcbnew.app/Contents/MacOS/pcbnew" )
#define EESCHEMA_EXE        wxT( "eeschema.app/Contents/MacOS/eeschema" )
#define GERBVIEW_EXE        wxT( "gerbview.app/Contents/MacOS/gerbview" )
#define BITMAPCONVERTER_EXE wxT( "bitmap2component.app/Contents/MacOS/bitmap2component" )
#define PCB_CALCULATOR_EXE  wxT( "pcb_calculator.app/Contents/MacOS/pcb_calculator" )
# endif
#endif


/* Graphic Texts Orientation in 0.1 degree*/
#define TEXT_ORIENT_HORIZ 0
#define TEXT_ORIENT_VERT  900

#define ON  1
#define OFF 0


enum EDA_UNITS_T {
    INCHES = 0,
    MILLIMETRES = 1,
    UNSCALED_UNITS = 2
};

#if defined(KICAD_GOST)
#define GOST_LEFTMARGIN   800    /* 20mm */
#define GOST_RIGHTMARGIN  200    /* 5mm */
#define GOST_TOPMARGIN    200    /* 5mm */
#define GOST_BOTTOMMARGIN 200    /* 5mm */

#endif
/* forward declarations: */
class LibNameList;


/* Class to handle pages sizes:
 */
class Ki_PageDescr
{
// All sizes are in 1/1000 inch
public:
    wxSize   m_Size;    /* page size in 1/1000 inch */
    wxPoint  m_Offset;  /* plot offset in 1/1000 inch */
    wxString m_Name;
    int      m_LeftMargin;
    int      m_RightMargin;
    int      m_TopMargin;
    int      m_BottomMargin;

public:
    Ki_PageDescr( const wxSize& size, const wxPoint& offset, const wxString& name );
};


extern Ki_PageDescr   g_Sheet_A4;
extern Ki_PageDescr   g_Sheet_A3;
extern Ki_PageDescr   g_Sheet_A2;
extern Ki_PageDescr   g_Sheet_A1;
extern Ki_PageDescr   g_Sheet_A0;
extern Ki_PageDescr   g_Sheet_A;
extern Ki_PageDescr   g_Sheet_B;
extern Ki_PageDescr   g_Sheet_C;
extern Ki_PageDescr   g_Sheet_D;
extern Ki_PageDescr   g_Sheet_E;
extern Ki_PageDescr   g_Sheet_GERBER;
extern Ki_PageDescr   g_Sheet_user;
extern Ki_PageDescr*  g_SheetSizeList[];


extern wxString       g_ProductName;

/* Default user lib path can be left void, if the standard lib path is used */
extern wxString       g_UserLibDirBuffer;

extern bool           g_ShowPageLimits; // true to display the page limits

/**
 * File extension definitions.  Please do not changes these.  If a different
 * file extension is needed, create a new definition in the application.
 * Please note, just because they are defined as const doesn't guarantee
 * that they cannot be changed.
 */
extern const wxString ProjectFileExtension;
extern const wxString SchematicFileExtension;
extern const wxString NetlistFileExtension;
extern const wxString GerberFileExtension;
extern const wxString PcbFileExtension;
extern const wxString PdfFileExtension;
extern const wxString MacrosFileExtension;

/// Proper wxFileDialog wild card definitions.
extern const wxString ProjectFileWildcard;
extern const wxString SchematicFileWildcard;
extern const wxString BoardFileWildcard;
extern const wxString NetlistFileWildcard;
extern const wxString GerberFileWildcard;
extern const wxString PcbFileWildcard;
extern const wxString PdfFileWildcard;
extern const wxString MacrosFileWildcard;
extern const wxString AllFilesWildcard;


// Name of default configuration file. (kicad.pro)
extern wxString     g_Prj_Default_Config_FullFilename;

// Name of local configuration file. (<curr projet>.pro)
extern wxString     g_Prj_Config_LocalFilename;

extern EDA_UNITS_T  g_UserUnit;     ///< display units

/// Draw color for moving objects.
extern int          g_GhostColor;


/* COMMON.CPP */

/**
 * Function SetLocaleTo_C_standard
 *  because KiCad is internationalized, switch internalization to "C" standard
 *  i.e. uses the . (dot) as separator in print/read float numbers
 *  (some countries (France, Germany ..) use , (comma) as separator)
 *  This function must be called before read or write ascii files using float
 *  numbers in data the SetLocaleTo_C_standard function must be called after
 *  reading or writing the file
 *
 *  This is wrapper to the C setlocale( LC_NUMERIC, "C" ) function,
 *  but could make more easier an optional use of locale in KiCad
 */
void SetLocaleTo_C_standard( void );

/**
 * Function SetLocaleTo_Default
 *  because KiCad is internationalized, switch internalization to default
 *  to use the default separator in print/read float numbers
 *  (. (dot) but some countries (France, Germany ..) use , (comma) as
 *   separator)
 *  This function must be called after a call to SetLocaleTo_C_standard
 *
 *  This is wrapper to the C setlocale( LC_NUMERIC, "" ) function,
 *  but could make more easier an optional use of locale in KiCad
 */
void SetLocaleTo_Default( void );


/**
 * Function EnsureTextCtrlWidth
 * sets the minimum pixel width on a text control in order to make a text
 * string be fully visible within it. The current font within the text
 * control is considered.
 * The text can come either from the control or be given as an argument.
 * If the text control is larger than needed, then nothing is done.
 * @param aCtrl the text control to potentially make wider.
 * @param aString the text that is used in sizing the control's pixel width.
 * If NULL, then
 *   the text already within the control is used.
 * @return bool - true if the \a aCtrl had its size changed, else false.
 */
bool EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString = NULL );


/**
 * Operator << overload
 * outputs a point to the argument string in a format resembling
 * "@ (x,y)
 * @param aString Where to put the text describing the point value
 * @param aPoint  The point to output.
 * @return wxString& - the input string
 */
wxString& operator <<( wxString& aString, const wxPoint& aPoint );


/**
 * Function ProcessExecute
 * runs a child process.
 * @param aCommandLine The process and any arguments to it all in a single
 *                     string.
 * @param aFlags The same args as allowed for wxExecute()
 * @return bool - true if success, else false
 */
bool ProcessExecute( const wxString& aCommandLine, int aFlags = wxEXEC_ASYNC );


/*******************/
/* about_kicad.cpp */
/*******************/
void InitKiCadAbout( wxAboutDialogInfo& info );


/**************/
/* common.cpp */
/**************/

int GetTimeStamp();

int DisplayColorFrame( wxWindow* parent, int OldColor );
int GetCommandOptions( const int argc, const char** argv,
                       const char* stringtst, const char** optarg,
                       int* optind );


/* Returns to display the value of a parameter, by type of units selected
 * Input: value in mils, buffer text
 * Returns to buffer: text: value expressed in inches or millimeters
 * Followed by " or mm
 */
const wxString& valeur_param( int valeur, wxString& buf_texte );

/**
 * Function CoordinateToString
 * is a helper to convert the integer coordinate \a aValue to a string in inches,
 * millimeters, or unscaled units according to the current user units setting.
 *
 * @param aValue The coordinate to convert.
 * @param aInternalUnits The internal units of the application.  #EESCHEMA_INTERNAL_UNIT
 *                       and #PCB_INTERNAL_UNIT are the only valid value.
 * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
 *                       the current user unit is millimeters.
 * @return The converted string for display in user interface elements.
 */
wxString CoordinateToString( int aValue, int aInternalUnits, bool aConvertToMils = false );

/**
 * Returns the units symbol.
 *
 * @param aUnits - Units type, default is current units setting.
 * @param aFormatString - A formatting string to embed the units symbol into.  Note:
 *                        the format string must contain the %s format specifier.
 * @return The formatted units symbol.
 */
wxString ReturnUnitSymbol( EDA_UNITS_T aUnits = g_UserUnit,
                           const wxString& aFormatString = _( " (%s):" ) );

/**
 * Get a human readable units string.
 *
 * The strings returned are full text name and not abbreviations or symbolic
 * representations of the units.  Use ReturnUnitSymbol() for that.
 *
 * @param aUnits - The units text to return.
 * @return The human readable units string.
 */
wxString GetUnitsLabel( EDA_UNITS_T aUnits );
wxString GetAbbreviatedUnitsLabel( EDA_UNITS_T aUnit = g_UserUnit );

/**
 * Function ReturnValueFromeString
 * @return The string from Value, according to units (inch, mm ...) for display,
 *  and the initial unit for value
 *  Unit = display units (INCH, MM ..)
 *  Value = text
 *  Internal_Unit = units per inch for computed value
 */
int ReturnValueFromString( EDA_UNITS_T aUnit, const wxString& TextValue, int Internal_Unit );

/**
 * Function ReturnStringFromValue
 * Return the string from Value, according to units (inch, mm ...) for display,
 * and the initial unit for value
 * @param aUnit = display units (INCHES, MILLIMETRE ..)
 * @param aValue = value in Internal_Unit
 * @param aInternal_Unit = units per inch for Value
 * @param aAdd_unit_symbol = true to add symbol unit to the string value
 * @return a wxString what contains value and optionally the symbol unit (like
 *         2.000 mm)
 */
wxString ReturnStringFromValue( EDA_UNITS_T aUnit,
                                int  aValue,
                                int  aInternal_Unit,
                                bool aAdd_unit_symbol = false );


/**
 * Function AddUnitSymbol
 * adds string "  (mm):" or " ("):" to the static text Stext.
 * Used in dialog boxes for entering values depending on selected units
 */
void AddUnitSymbol( wxStaticText& Stext, EDA_UNITS_T aUnit = g_UserUnit );

/* Add string "  (mm):" or " ("):" to the static text Stext.
 *  Used in dialog boxes for entering values depending on selected units */
void PutValueInLocalUnits( wxTextCtrl& TextCtr, int Value, int Internal_Unit );

/**
 * Convert the number Value in a string according to the internal units
 *  and the selected unit (g_UserUnit) and put it in the wxTextCtrl TextCtrl
 */
int ReturnValueFromTextCtrl( const wxTextCtrl& TextCtr, int Internal_Unit );

#ifdef KICAD_NANOMETRE

struct LENGTH_UNIT_DESC
{
    LENGTH_DEF     m_Value;
    const wxString m_Symbol;
    int            m_Precision;
};

extern const LENGTH_UNIT_DESC g_MillimetreDesc, g_InchDesc, g_MilDesc;

const LENGTH_UNIT_DESC *UnitDescription( EDA_UNITS_T aUnit );

LENGTH_DEF       StringToLength( const LENGTH_UNIT_DESC *aUnit, const wxString& TextValue );
wxString         LengthToString( const LENGTH_UNIT_DESC *aUnit, LENGTH_DEF aValue,
                                 bool aAdd_unit_symbol = false );

void             LengthToTextCtrl( wxTextCtrl& TextCtr, LENGTH_DEF Value );
LENGTH_DEF       LengthFromTextCtrl( const wxTextCtrl& TextCtr );

#endif

/**
 * Function wxStringSplit
 * splita \a aString to a string list when split by \a aSplitter.
 * @return the list
 * @param aString : wxString : a String text
 * @param aSplitter : wxChar : the 'split' character
 */
wxArrayString* wxStringSplit( wxString aString, wxChar aSplitter );

/**
 * Function To_User_Unit
 * Convert in inch or mm the variable "val" (double)given in internal units
 * @return the converted value, in double
 * @param aUnit : user unit to be converted to
 * @param val : double : the given value
 * @param internal_unit_value = internal units per inch
 */
double To_User_Unit( EDA_UNITS_T aUnit, double val, int internal_unit_value );

/*
 * Return in internal units the value "val" given in inch or mm
 */
int From_User_Unit( EDA_UNITS_T aUnit, double val, int internal_unit_value );

/**
 * Function GenDate
 * @return A wsString object containg the date in the format "day month year" like
 *         "23 jun 2005".
 */
wxString GenDate();

#endif  // __INCLUDE__COMMON_H__
