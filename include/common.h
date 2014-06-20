/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <convert_to_biu.h>
#include <colors.h>


class wxAboutDialogInfo;
class SEARCH_STACK;


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
#define PL_EDITOR_EXE       wxT( "pl_editor.exe" )
#else
#ifndef __WXMAC__
#define CVPCB_EXE           wxT( "cvpcb" )
#define PCBNEW_EXE          wxT( "pcbnew" )
#define EESCHEMA_EXE        wxT( "eeschema" )
#define GERBVIEW_EXE        wxT( "gerbview" )
#define BITMAPCONVERTER_EXE wxT( "bitmap2component" )
#define PCB_CALCULATOR_EXE  wxT( "pcb_calculator" )
#define PL_EDITOR_EXE       wxT( "pl_editor" )
#else
#define CVPCB_EXE           wxT( "cvpcb.app/Contents/MacOS/cvpcb" )
#define PCBNEW_EXE          wxT( "pcbnew.app/Contents/MacOS/pcbnew" )
#define EESCHEMA_EXE        wxT( "eeschema.app/Contents/MacOS/eeschema" )
#define GERBVIEW_EXE        wxT( "gerbview.app/Contents/MacOS/gerbview" )
#define BITMAPCONVERTER_EXE wxT( "bitmap2component.app/Contents/MacOS/bitmap2component" )
#define PCB_CALCULATOR_EXE  wxT( "pcb_calculator.app/Contents/MacOS/pcb_calculator" )
#define PL_EDITOR_EXE       wxT( "pl_editor.app/Contents/MacOS/pl_editor" )
# endif
#endif


// Graphic Texts Orientation in 0.1 degree
#define TEXT_ORIENT_HORIZ 0
#define TEXT_ORIENT_VERT  900



//-----<KiROUND KIT>------------------------------------------------------------

/**
 * KiROUND
 * rounds a floating point number to an int using
 * "round halfway cases away from zero".
 * In Debug build an assert fires if will not fit into an int.
 */

#if !defined( DEBUG )

/// KiROUND: a function so v is not evaluated twice.  Unfortunately, compiler
/// is unable to pre-compute constants using this.
static inline int KiROUND( double v )
{
    return int( v < 0 ? v - 0.5 : v + 0.5 );
}

/// KIROUND: a macro so compiler can pre-compute constants.  Use this with compile
/// time constants rather than the inline function above.
#define KIROUND( v )    int( (v) < 0 ? (v) - 0.5 : (v) + 0.5 )

#else

// DEBUG: KiROUND() is a macro to capture line and file, then calls this inline

static inline int kiRound_( double v, int line, const char* filename )
{
    v = v < 0 ? v - 0.5 : v + 0.5;
    if( v > INT_MAX + 0.5 )
    {
        printf( "%s: in file %s on line %d, val: %.16g too ' > 0 ' for int\n", __FUNCTION__, filename, line, v );
    }
    else if( v < INT_MIN - 0.5 )
    {
        printf( "%s: in file %s on line %d, val: %.16g too ' < 0 ' for int\n", __FUNCTION__, filename, line, v );
    }
    return int( v );
}

#define KiROUND( v )    kiRound_( v, __LINE__, __FILE__ )

// in Debug build, use the overflow catcher since code size is immaterial
#define KIROUND( v )    KiROUND( v )

#endif

//-----</KiROUND KIT>-----------------------------------------------------------



/// Convert mm to mils.
inline int Mm2mils( double x ) { return KiROUND( x * 1000./25.4 ); }

/// Convert mils to mm.
inline int Mils2mm( double x ) { return KiROUND( x * 25.4 / 1000. ); }


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

    // these were once wxStrings, but it caused static construction sequence problems:
    static const wxChar A4[];
    static const wxChar A3[];
    static const wxChar A2[];
    static const wxChar A1[];
    static const wxChar A0[];
    static const wxChar A[];
    static const wxChar B[];
    static const wxChar C[];
    static const wxChar D[];
    static const wxChar E[];
    static const wxChar GERBER[];
    static const wxChar USLetter[];
    static const wxChar USLegal[];
    static const wxChar USLedger[];
    static const wxChar Custom[];     ///< "User" defined page type


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
#if defined(PCBNEW) || defined(EESCHEMA) || defined(GERBVIEW) || defined(PL_EDITOR)
    int GetWidthIU() const  { return IU_PER_MILS * GetWidthMils();  }
    int GetHeightIU() const { return IU_PER_MILS * GetHeightMils(); }
    const wxSize GetSizeIU() const  { return wxSize( GetWidthIU(), GetHeightIU() ); }
#endif

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

    bool        m_portrait;         ///< true if portrait, false if landscape

    wxPaperSize m_paper_id;         ///< wx' style paper id.

    static int s_user_height;
    static int s_user_width;

    void    updatePortrait();

    void    setMargins();
};

extern EDA_UNITS_T  g_UserUnit;     ///< display units

/// Draw color for moving objects.
extern EDA_COLOR_T  g_GhostColor;


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
    LOCALE_IO()
    {
        wxASSERT_MSG( C_count >= 0, wxT( "LOCALE_IO::C_count mismanaged." ) );

        // use thread safe, atomic operation
        if( __sync_fetch_and_add( &C_count, 1 ) == 0 )
        {
            // printf( "setting C locale.\n" );
            SetLocaleTo_C_standard();
        }
    }

    ~LOCALE_IO()
    {
        // use thread safe, atomic operation
        if( __sync_sub_and_fetch( &C_count, 1 ) == 0 )
        {
            // printf( "restoring default locale.\n" );
            SetLocaleTo_Default();
        }

        wxASSERT_MSG( C_count >= 0, wxT( "LOCALE_IO::C_count mismanaged." ) );
    }

private:
    static int  C_count;    // allow for nesting of LOCALE_IO instantiations
};


/**
 * Function GetTextSize
 * returns the size of @a aSingleLine of text when it is rendered in @a aWindow
 * using whatever font is currently set in that window.
 */
wxSize GetTextSize( const wxString& aSingleLine, wxWindow* aWindow );

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
 * Function ProcessExecute
 * runs a child process.
 * @param aCommandLine The process and any arguments to it all in a single
 *                     string.
 * @param aFlags The same args as allowed for wxExecute()
 * @param callback wxProcess implementing OnTerminate to be run when the
                   child process finishes
 * @return int - pid of process, 0 in case of error (like return values of
 *               wxExecute())
 */
int ProcessExecute( const wxString& aCommandLine, int aFlags = wxEXEC_ASYNC,
                     wxProcess *callback = NULL );


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
time_t GetNewTimeStamp();

EDA_COLOR_T DisplayColorFrame( wxWindow* parent, int OldColor );
int GetCommandOptions( const int argc, const char** argv,
                       const char* stringtst, const char** optarg,
                       int* optind );

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
 * Function GetRunningMicroSecs
 * returns an ever increasing indication of elapsed microseconds.  Use this
 * by computing differences between two calls.
 * @author Dick Hollenbeck
 */
unsigned GetRunningMicroSecs();

/**
 * Formats a wxDateTime using the long date format (on wx 2.9) or
 * an hardcoded format in wx 2.8; the idea is to avoid like the plague
 * the numeric-only date formats: it's difficult to discriminate between
 * dd/mm/yyyy and mm/dd/yyyy. The output is meant for user consumption
 * so no attempt to parse it should be done
 */
wxString FormatDateLong( const wxDateTime &aDate );

/**
 * Function SystemDirsAppend
 * appends system places to aSearchStack in a platform specific way, and pertinent
 * to KiCad programs.  It seems to be a place to collect bad ideas and keep them
 * out of view.
 */
void SystemDirsAppend( SEARCH_STACK* aSearchStack );


/**
 * Function SearchHelpFileFullPath
 * returns the help file's full path.
 * <p>
 * Return the KiCad help file with path.
 * If the help file for the current locale is not found, an attempt to find
 * the English version of the help file is made.
 * Help file is searched in directories in this order:
 *  help/\<canonical name\> like help/en_GB
 *  help/\<short name\> like help/en
 *  help/en
 * </p>
 * @param aSearchStack contains some possible base dirs that may be above the
 *  the one actually holding @a aBaseName.  These are starting points for nested searches.
 * @param aBaseName is the name of the help file to search for.
 * @return  wxEmptyString is returned if aBaseName is not found, else the full path & filename.
 */
wxString SearchHelpFileFullPath( const SEARCH_STACK& aSearchStack, const wxString& aBaseName );


/// Put aPriorityPath in front of all paths in the value of aEnvVar.
const wxString PrePendPath( const wxString& aEnvVar, const wxString& aPriorityPath );


#endif  // INCLUDE__COMMON_H_
