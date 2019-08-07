/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/dir.h>

#include <richio.h>
#include <gal/color4d.h>

#include <atomic>
#include <memory>

class wxAboutDialogInfo;
class SEARCH_STACK;
class REPORTER;


/**
 * timestamp_t is our type to represent unique IDs for all kinds of elements;
 * historically simply the timestamp when they were created.
 *
 * Long term, this type might be renamed to something like unique_id_t
 * (and then rename all the methods from {Get,Set}TimeStamp()
 * to {Get,Set}Id()) ?
 */
typedef uint32_t timestamp_t;


// Flag for special keys
// This type could be extended to 64 bits to add room for more flags.
// For compatibility with old code, keep flag bits out of the least
// significant nibble (0xF).
typedef uint32_t EDA_KEY;
#define EDA_KEY_C UINT32_C

#define GR_KB_RIGHTSHIFT    ( EDA_KEY_C( 0x01000000 ) )
#define GR_KB_LEFTSHIFT     ( EDA_KEY_C( 0x02000000 ) )
#define GR_KB_CTRL          ( EDA_KEY_C( 0x04000000 ) )
#define GR_KB_ALT           ( EDA_KEY_C( 0x08000000 ) )
#define GR_KB_SHIFT         ( GR_KB_LEFTSHIFT | GR_KB_RIGHTSHIFT )
#define GR_KB_SHIFTCTRL     ( GR_KB_SHIFT | GR_KB_CTRL )
#define MOUSE_MIDDLE        ( EDA_KEY_C( 0x10000000 ) )
#define GR_KEY_INVALID      ( EDA_KEY_C( 0x80000000 ) )
#define GR_KEY_NONE         ( EDA_KEY_C( 0 ) )

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

/// Frequent text rotations, used with {Set,Get}TextAngle(),
/// in 0.1 degrees for now, hoping to migrate to degrees eventually.
#define TEXT_ANGLE_HORIZ    0
#define TEXT_ANGLE_VERT     900

//-----<KiROUND KIT>------------------------------------------------------------

/**
 * Round a floating point number to an integer using "round halfway cases away from zero".
 *
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
        printf( "%s: in file %s on line %d, val: %.16g too ' > 0 ' for int\n",
                __FUNCTION__, filename, line, v );
    }
    else if( v < INT_MIN - 0.5 )
    {
        printf( "%s: in file %s on line %d, val: %.16g too ' < 0 ' for int\n",
                __FUNCTION__, filename, line, v );
    }
    return int( v );
}

#define KiROUND( v )    kiRound_( v, __LINE__, __FILE__ )

// in Debug build, use the overflow catcher since code size is immaterial
#define KIROUND( v )    KiROUND( v )

#endif

//-----</KiROUND KIT>-----------------------------------------------------------


enum EDA_UNITS_T {
    INCHES = 0,
    MILLIMETRES = 1,
    UNSCALED_UNITS = 2,
    DEGREES = 3,
    PERCENT = 4,
};


/// Draw color for moving objects.
extern KIGFX::COLOR4D  g_GhostColor;


/**
 * Instantiate the current locale within a scope in which you are expecting
 * exceptions to be thrown.
 *
 * The constructor sets a "C" language locale option, to read/print files with floating
 * point  numbers.  The destructor insures that the default locale is restored if an
 * exception is thrown or not.
 */
class LOCALE_IO
{
public:
    LOCALE_IO();
    ~LOCALE_IO();

private:
    // allow for nesting of LOCALE_IO instantiations
    static std::atomic<unsigned int> m_c_count;

    // The locale in use before switching to the "C" locale
    // (the locale can be set by user, and is not always the system locale)
    std::string m_user_locale;
};

/**
 * Return the size of @a aSingleLine of text when it is rendered in @a aWindow
 * using whatever font is currently set in that window.
 */
wxSize GetTextSize( const wxString& aSingleLine, wxWindow* aWindow );

/**
 * Set the minimum pixel width on a text control in order to make a text
 * string be fully visible within it.
 *
 * The current font within the text control is considered.  The text can come either from
 * the control or be given as an argument.  If the text control is larger than needed, then
 * nothing is done.
 *
 * @param aCtrl the text control to potentially make wider.
 * @param aString the text that is used in sizing the control's pixel width.
 * If NULL, then
 *   the text already within the control is used.
 * @return bool - true if the \a aCtrl had its size changed, else false.
 */
bool EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString = NULL );

/**
 * Select the number (or "?") in a reference for ease of editing.
 */
void SelectReferenceNumber( wxTextEntry* aTextEntry );

/**
 * Run a command in a child process.
 *
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

/**
 * @return an unique time stamp that changes after each call
 */
timestamp_t GetNewTimeStamp();

int GetCommandOptions( const int argc, const char** argv,
                       const char* stringtst, const char** optarg,
                       int* optind );

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
 * Split \a aString to a string list separated at \a aSplitter.
 *
 * @param aText is the text to split
 * @param aStrings will contain the splitted lines
 * @param aSplitter is the 'split' character
 */
void wxStringSplit( const wxString& aText, wxArrayString& aStrings, wxChar aSplitter );

/**
 * Return the help file's full path.
 * <p>
 * Return the KiCad help file with path and extension.
 * Help files can be html (.html ext) or pdf (.pdf ext) files.
 * A \<BaseName\>.html file is searched and if not found,
 * \<BaseName\>.pdf file is searched in the same path.
 * If the help file for the current locale is not found, an attempt to find
 * the English version of the help file is made.
 * Help file is searched in directories in this order:
 *  help/\<canonical name\> like help/en_GB
 *  help/\<short name\> like help/en
 *  help/en
 * </p>
 * @param aSearchStack contains some possible base dirs that may be above the
 *  the one actually holding @a aBaseName.  These are starting points for nested searches.
 * @param aBaseName is the name of the help file to search for, <p>without extension</p>.
 * @return  wxEmptyString is returned if aBaseName is not found, else the full path & filename.
 */
wxString SearchHelpFileFullPath( const SEARCH_STACK& aSearchStack, const wxString& aBaseName );

/**
 * Make \a aTargetFullFileName absolute and create the path of this file if it doesn't yet exist.
 *
 * @param aTargetFullFileName the wxFileName containing the full path and file name to modify.
 *                            The path may be absolute or relative to \a aBaseFilename .
 * @param aBaseFilename a full filename. Only its path is used to set the aTargetFullFileName path.
 * @param aReporter a point to a REPORTER object use to show messages (can be NULL)
 * @return true if \a aOutputDir already exists or was successfully created.
 */
bool EnsureFileDirectoryExists( wxFileName*     aTargetFullFileName,
                                const wxString& aBaseFilename,
                                REPORTER*       aReporter = NULL );

/// Put aPriorityPath in front of all paths in the value of aEnvVar.
const wxString PrePendPath( const wxString& aEnvVar, const wxString& aPriorityPath );

/**
 * Create a new wxConfig so we can put configuration files in a more proper place for each
 * platform.
 *
 * This is generally $HOME/.config/kicad/ in Linux according to the FreeDesktop specification at
 * http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
 * The config object created here should be destroyed by the caller.
 *
 * @param aProgName is the name of the program calling this function - can be obtained by
 *  calling Pgm().App().GetAppName().  This will be the actual file name of the config file.
 * @return A pointer to a new wxConfigBase derived object is returned.  The caller is in charge
 *  of deleting it.
 */
std::unique_ptr<wxConfigBase> GetNewConfig( const wxString& aProgName );

/**
 * Return the user configuration path used to store KiCad's configuration files.
 *
 * The configuration path order of precedence is determined by the following criteria:
 *
 * - The value of the KICAD_CONFIG_HOME environment variable
 * - The value of the XDG_CONFIG_HOME environment variable.
 * - The result of the call to wxStandardPaths::GetUserConfigDir() with ".config" appended
 *   as required on Linux builds.
 *
 * @return A wxString containing the config path for Kicad
 */
wxString GetKicadConfigPath();

/**
 * Replace any environment variable references with their values.
 *
 * @param aString = a string containing (perhaps) references to env var
 * @return a string where env var are replaced by their value
 */
const wxString ExpandEnvVarSubstitutions( const wxString& aString );

/**
 * Replace any environment variables in file-path uris (leaving network-path URIs alone).
 */
const wxString ResolveUriByEnvVars( const wxString& aUri );


#ifdef __WXMAC__
/**
 * OSX specific function GetOSXKicadUserDataDir
 * @return A wxString pointing to the user data directory for Kicad
 */
wxString GetOSXKicadUserDataDir();

/**
 * OSX specific function GetOSXMachineDataDir
 * @return A wxString pointing to the machine data directory for Kicad
 */
wxString GetOSXKicadMachineDataDir();

/**
 * OSX specific function GetOSXKicadDataDir
 * @return A wxString pointing to the bundle data directory for Kicad
 */
wxString GetOSXKicadDataDir();
#endif

// Some wxWidgets versions (for instance before 3.1.0) do not include
// this function, so add it if missing
#if !wxCHECK_VERSION( 3, 1, 0 )
#define USE_KICAD_WXSTRING_HASH     // for common.cpp
///> Template specialization to enable wxStrings for certain containers (e.g. unordered_map)
namespace std
{
    template<> struct hash<wxString>
    {
        size_t operator()( const wxString& s ) const;
    };
}
#endif

/// Required to use wxPoint as key type in maps
#define USE_KICAD_WXPOINT_LESS_AND_HASH // for common.cpp
namespace std
{
    template <> struct hash<wxPoint>
    {
        size_t operator() ( const wxPoint& k ) const;
    };
}

namespace std
{
    template<> struct less<wxPoint>
    {
        bool operator()( const wxPoint& aA, const wxPoint& aB ) const;
    };
}

/**
 * Helper function to print the given wxSize to a stream.
 *
 * Used for debugging functions like EDA_ITEM::Show and also in unit
 * testing fixtures.
 */
std::ostream& operator<<( std::ostream& out, const wxSize& size );

/**
 * Helper function to print the given wxPoint to a stream.
 *
 * Used for debugging functions like EDA_ITEM::Show and also in unit
 * testing fixtures.
 */
std::ostream& operator<<( std::ostream& out, const wxPoint& pt );


/**
 * A wrapper around a wxFileName which is much more performant with a subset of the API.
 */
class WX_FILENAME
{
public:
    WX_FILENAME( const wxString& aPath, const wxString& aFilename );

    void SetFullName( const wxString& aFileNameAndExtension );

    wxString GetName() const;
    wxString GetFullName() const;
    wxString GetPath() const;
    wxString GetFullPath() const;

    // Avoid multiple calls to stat() on POSIX kernels.
    long long GetTimestamp();

private:
    // Write cached values to the wrapped wxFileName.  MUST be called before using m_fn.
    void resolve();

    wxFileName m_fn;
    wxString   m_path;
    wxString   m_fullName;
};


long long TimestampDir( const wxString& aDirPath, const wxString& aFilespec );




#endif  // INCLUDE__COMMON_H_
