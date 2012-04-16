/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef INCLUDE__COMMON_H_
#define INCLUDE__COMMON_H_

#include <vector>

#include <wx/wx.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>

#include <richio.h>

#if !wxUSE_PRINTING_ARCHITECTURE
#   error "You must use '--enable-printarch' in your wx library configuration."
#endif

#if defined( __WXGTK__ )
#   if !wxUSE_LIBGNOMEPRINT && !wxUSE_GTKPRINT
#       error "You must use '--with-gnomeprint' or '--with-gtkprint' in your wx library configuration."
#   endif
#endif


class wxAboutDialogInfo;

// Flag for special keys
#define GR_KB_RIGHTSHIFT 0x10000000                 /* Keybd states: right
                                                     * shift key depressed */
#define GR_KB_LEFTSHIFT  0x20000000                 /* left shift key depressed
                                                     */
#define GR_KB_CTRL       0x40000000                 // CTRL depressed
#define GR_KB_ALT        0x80000000                 // ALT depressed
#define GR_KB_SHIFT      (GR_KB_LEFTSHIFT | GR_KB_RIGHTSHIFT)
#define GR_KB_SHIFTCTRL  (GR_KB_SHIFT | GR_KB_CTRL)
#define MOUSE_MIDDLE     0x08000000                 /* Middle button mouse
                                                     * flag for block commands
                                                     */

/// default name for nameless projects
#define NAMELESS_PROJECT wxT( "noname" )


/// Pseudo key codes for command panning
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

// TODO Executable names TODO
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


// Graphic Texts Orientation in 0.1 degree
#define TEXT_ORIENT_HORIZ 0
#define TEXT_ORIENT_VERT  900

#define ON  1
#define OFF 0


/// Convert mm to mils.
inline int Mm2mils( double x ) { return wxRound( x * 1000./25.4 ); }

/// Convert mils to mm.
inline int Mils2mm( double x ) { return wxRound( x * 25.4 / 1000. ); }


/// Return whether GOST is in play
bool IsGOST();


enum EDA_UNITS_T {
    INCHES = 0,
    MILLIMETRES = 1,
    UNSCALED_UNITS = 2
};


// forward declarations:
class LibNameList;


/**
 * Class PAGE_INFO
 * describes the page size and margins of a paper page on which to
 * eventually print or plot.  Paper sizes are often described in inches.
 * Here paper is described in 1/1000th of an inch (mils).  For convenience
 * there are some read only accessors for internal units (IU), which is a compile
 * time calculation, not runtime.
 *
 * @author Dick Hollenbeck
 */
class PAGE_INFO
{
public:

    PAGE_INFO( const wxString& aType = PAGE_INFO::A3, bool IsPortrait = false );

    // paper size names which are part of the public API, pass to SetType() or
    // above constructor.

    static const wxString A4;
    static const wxString A3;
    static const wxString A2;
    static const wxString A1;
    static const wxString A0;
    static const wxString A;
    static const wxString B;
    static const wxString C;
    static const wxString D;
    static const wxString E;
    static const wxString GERBER;
    static const wxString USLetter;
    static const wxString USLegal;
    static const wxString USLedger;
    static const wxString Custom;     ///< "User" defined page type


    /**
     * Function SetType
     * sets the name of the page type and also the sizes and margins
     * commonly associated with that type name.
     *
     * @param aStandardPageDescriptionName is a wxString constant giving one of:
     * "A4" "A3" "A2" "A1" "A0" "A" "B" "C" "D" "E" "GERBER", "USLetter", "USLegal",
     * "USLedger", or "User".  If "User" then the width and height are custom,
     * and will be set according to <b>previous</b> calls to
     * static PAGE_INFO::SetUserWidthMils() and
     * static PAGE_INFO::SetUserHeightMils();
     * @param IsPortrait Set to true to set page orientation to portrait mode.
     *
     * @return bool - true if @a aStandarePageDescription was a recognized type.
     */
    bool SetType( const wxString& aStandardPageDescriptionName, bool IsPortrait = false );
    const wxString& GetType() const { return m_type; }

    /**
     * Function IsDefault
     * @return True if the object has the default page settings which are A3, landscape.
     */
    bool IsDefault() const { return m_type == PAGE_INFO::A3 && !m_portrait; }

    /**
     * Function IsCustom
     * returns true if the type is Custom
     */
    bool IsCustom() const;

    /**
     * Function SetPortrait
     * will rotate the paper page 90 degrees.  This PAGE_INFO may either be in
     * portrait or landscape mode.  Use this function to change from one to the
     * other mode.
     * @param isPortrait if true and not already in portrait mode, will change
     *  this PAGE_INFO to portrait mode.  Or if false and not already in landscape mode,
     *  will change this PAGE_INFO to landscape mode.
     */
    void SetPortrait( bool isPortrait );
    bool IsPortrait() const { return m_portrait; }

    /**
     * Function GetWxOrientation.
     * @return ws' style printing orientation (wxPORTRAIT or wxLANDSCAPE).
     */
#if wxCHECK_VERSION( 2, 9, 0  )
    wxPrintOrientation  GetWxOrientation() const { return IsPortrait() ? wxPORTRAIT : wxLANDSCAPE; }
#else
    int  GetWxOrientation() const { return IsPortrait() ? wxPORTRAIT : wxLANDSCAPE; }
#endif

    /**
     * Function GetPaperId
     * @return wxPaperSize - wxPrintData's style paper id associated with
     * page type name.
     */
    wxPaperSize GetPaperId() const { return m_paper_id; }

    void SetWidthMils(  int aWidthInMils );
    int GetWidthMils() const { return m_size.x; }

    void SetHeightMils( int aHeightInMils );
    int GetHeightMils() const { return m_size.y; }

    const wxSize& GetSizeMils() const { return m_size; }

    // Accessors returning "Internal Units (IU)".  IUs are mils in EESCHEMA,
    // and either deci-mils or nanometers in PCBNew.
#if defined(PCBNEW)
# if defined(KICAD_NANOMETRE)
    int GetWidthIU() const  { return 25400 * GetWidthMils();  }
    int GetHeightIU() const { return 25400 * GetHeightMils(); }
# else
    int GetWidthIU() const  { return 10 * GetWidthMils();  }
    int GetHeightIU() const { return 10 * GetHeightMils(); }
# endif
    const wxSize GetSizeIU() const  { return wxSize( GetWidthIU(), GetHeightIU() ); }
#elif defined(EESCHEMA)
    int GetWidthIU() const  { return GetWidthMils();  }
    int GetHeightIU() const { return GetHeightMils(); }
    const wxSize GetSizeIU() const  { return wxSize( GetWidthIU(), GetHeightIU() ); }
#endif

    /**
     * Function GetLeftMarginMils.
     * @return int - logical page left margin in mils.
     */
    int GetLeftMarginMils() const           { return m_left_margin; }

    /**
     * Function GetLeftMarginMils.
     * @return int - logical page right margin in mils.
     */
    int GetRightMarginMils() const          { return m_right_margin; }

    /**
     * Function GetLeftMarginMils.
     * @return int - logical page top margin in mils.
     */
    int GetTopMarginMils() const            { return m_top_margin; }

    /**
     * Function GetBottomMarginMils.
     * @return int - logical page bottom margin in mils.
     */
    int GetBottomMarginMils() const         { return m_bottom_margin; }

    /**
     * Function SetLeftMarginMils
     * sets left page margin to @a aMargin in mils.
     */
    void SetLeftMarginMils( int aMargin )   { m_left_margin = aMargin; }

    /**
     * Function SetRightMarginMils
     * sets right page margin to @a aMargin in mils.
     */
    void SetRightMarginMils( int aMargin )  { m_right_margin = aMargin; }

    /**
     * Function SetTopMarginMils
     * sets top page margin to @a aMargin in mils.
     */
    void SetTopMarginMils( int aMargin )    { m_top_margin = aMargin; }

    /**
     * Function SetBottomMarginMils
     * sets bottom page margin to @a aMargin in mils.
     */
    void SetBottomMarginMils( int aMargin ) { m_bottom_margin = aMargin; }

    /**
     * Function SetCustomWidthMils
     * sets the width of Custom page in mils, for any custom page
     * constructed or made via SetType() after making this call.
     */
    static void SetCustomWidthMils( int aWidthInMils );

    /**
     * Function SetCustomHeightMils
     * sets the height of Custom page in mils, for any custom page
     * constructed or made via SetType() after making this call.
     */
    static void SetCustomHeightMils( int aHeightInMils );

    /**
     * Function GetCustomWidthMils.
     * @return int - custom paper width in mils.
     */
    static int GetCustomWidthMils() { return s_user_width; }

    /**
     * Function GetCustomHeightMils.
     * @return int - custom paper height in mils.
     */
    static int GetCustomHeightMils() { return s_user_height; }

    /**
     * Function GetStandardSizes
     * returns the standard page types, such as "A4", "A3", etc.
    static wxArrayString GetStandardSizes();
     */

    /**
     * Function Format
     * outputs the page class to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
        throw( IO_ERROR );

protected:
    // only the class implementation(s) may use this constructor
    PAGE_INFO( const wxSize& aSizeMils, const wxString& aName, wxPaperSize aPaperId );


private:

    // standard pre-defined sizes
    static const PAGE_INFO pageA4;
    static const PAGE_INFO pageA3;
    static const PAGE_INFO pageA2;
    static const PAGE_INFO pageA1;
    static const PAGE_INFO pageA0;
    static const PAGE_INFO pageA;
    static const PAGE_INFO pageB;
    static const PAGE_INFO pageC;
    static const PAGE_INFO pageD;
    static const PAGE_INFO pageE;
    static const PAGE_INFO pageGERBER;

    static const PAGE_INFO pageUSLetter;
    static const PAGE_INFO pageUSLegal;
    static const PAGE_INFO pageUSLedger;

    static const PAGE_INFO pageUser;

    // all dimensions here are in mils

    wxString    m_type;             ///< paper type: A4, A3, etc.
    wxSize      m_size;             ///< mils

/// Min and max page sizes for clamping.
#define MIN_PAGE_SIZE   4000
#define MAX_PAGE_SIZE   48000


    int         m_left_margin;
    int         m_right_margin;
    int         m_top_margin;
    int         m_bottom_margin;

    bool        m_portrait;         ///< true if portrait, false if landscape

    wxPaperSize m_paper_id;         ///< wx' style paper id.

    static int s_user_height;
    static int s_user_width;

    void    updatePortrait();

    void    setMargins();
};


extern wxString     g_ProductName;

/// Default user lib path can be left void, if the standard lib path is used
extern wxString     g_UserLibDirBuffer;

extern bool         g_ShowPageLimits;       ///< true to display the page limits

/// Name of default configuration file. (kicad.pro)
extern wxString     g_Prj_Default_Config_FullFilename;

/// Name of local configuration file. (\<curr projet\>.pro)
extern wxString     g_Prj_Config_LocalFilename;

extern EDA_UNITS_T  g_UserUnit;     ///< display units

/// Draw color for moving objects.
extern int          g_GhostColor;


// COMMON.CPP

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
void SetLocaleTo_C_standard();

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
void SetLocaleTo_Default();

/**
 * Class LOCALE_IO
 * is a class that can be instantiated within a scope in which you are expecting
 * exceptions to be thrown.  Its constructor calls SetLocaleTo_C_Standard().
 * Its destructor insures that the default locale is restored if an exception
 * is thrown, or not.
 */
class LOCALE_IO
{
public:
    LOCALE_IO()     { SetLocaleTo_C_standard(); }
    ~LOCALE_IO()    { SetLocaleTo_Default(); }
};

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

/**
 * @return an unique time stamp that changes after each call
 */
unsigned long GetNewTimeStamp();

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

void AddUnitSymbol( wxStaticText& Stext, EDA_UNITS_T aUnit = g_UserUnit );

/**
 * Round to the nearest precision.
 *
 * Try to approximate a coordinate using a given precision to prevent
 * rounding errors when converting from inches to mm.
 *
 * ie round the unit value to 0 if unit is 1 or 2, or 8 or 9
 */
double RoundTo0( double x, double precision );

/**
 * Function wxStringSplit
 * splits \a aString to a string list separated at \a aSplitter.
 * @return the list
 * @param aString is the text to split
 * @param aSplitter is the 'split' character
 */
wxArrayString* wxStringSplit( wxString aString, wxChar aSplitter );

/**
 * Function GenDate
 * @return A wxString object containing the date in the format "day month year" like
 *         "23 jun 2005".
 */
wxString GenDate();

/**
 * Function GetRunningMicroSecs
 * returns an ever increasing indication of elapsed microseconds.  Use this
 * by computing differences between two calls.
 * @author Dick Hollenbeck
 */
unsigned GetRunningMicroSecs();

#endif  // INCLUDE__COMMON_H_
