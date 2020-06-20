/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <functional>

#include <wx/wx.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/dir.h>

#include <richio.h>
#include <gal/color4d.h>

#include <atomic>
#include <limits>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <boost/uuid/uuid.hpp>
#include <macros.h>

class PROJECT;
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

class KIID
{
public:
    KIID();
    KIID( int null );
    KIID( const wxString& aString );
    KIID( timestamp_t aTimestamp );

    void Clone( const KIID& aUUID );

    size_t Hash() const;

    bool IsLegacyTimestamp() const;
    timestamp_t AsLegacyTimestamp() const;

    wxString AsString() const;
    wxString AsLegacyTimestampString() const;

    /**
     * Change an existing time stamp based UUID into a true UUID.
     *
     * If this is not a time stamp based UUID, then no change is made.
     */
    void ConvertTimestampToUuid();

    bool operator==( KIID const& rhs) const
    {
        return m_uuid == rhs.m_uuid;
    }

    bool operator!=( KIID const& rhs) const
    {
        return m_uuid != rhs.m_uuid;
    }

    bool operator<( KIID const& rhs) const
    {
        return m_uuid < rhs.m_uuid;
    }

private:
    boost::uuids::uuid m_uuid;

    timestamp_t        m_cached_timestamp;
};


extern KIID niluuid;

KIID& NilUuid();

// declare KIID_VECT_LIST as std::vector<KIID> both for c++ and swig:
DECL_VEC_FOR_SWIG( KIID_VECT_LIST, KIID )

class KIID_PATH : public KIID_VECT_LIST
{
public:
    KIID_PATH()
    {}

    KIID_PATH( const wxString& aString )
    {
        for( const wxString& pathStep : wxSplit( aString, '/' ) )
        {
            if( !pathStep.empty() )
                emplace_back( KIID( pathStep ) );
        }
    }

    wxString AsString() const
    {
        wxString path;

        for( const KIID& pathStep : *this )
            path += '/' + pathStep.AsString();

        return path;
    }

    bool operator==( KIID_PATH const& rhs) const
    {
        if( size() != rhs.size() )
            return false;

        for( size_t i = 0; i < size(); ++i )
        {
            if( at( i ) != rhs.at( i ) )
                return false;
        }

        return true;
    }

    bool operator<( KIID_PATH const& rhs) const
    {
        if( size() != rhs.size() )
            return size() < rhs.size();

        for( size_t i = 0; i < size(); ++i )
        {
            if( at( i ) < rhs.at( i ) )
                return true;

            if( at( i ) != rhs.at( i ) )
                return false;
        }

        return false;
    }
};


/// default name for nameless projects
#define NAMELESS_PROJECT wxT( "noname" )

/// Frequent text rotations, used with {Set,Get}TextAngle(),
/// in 0.1 degrees for now, hoping to migrate to degrees eventually.
#define TEXT_ANGLE_HORIZ    0
#define TEXT_ANGLE_VERT     900

/**
 * The type of unit.
 */
enum class EDA_DATA_TYPE
{
    DISTANCE = 0,
    AREA     = 1,
    VOLUME   = 2
};

enum class EDA_UNITS
{
    INCHES = 0,
    MILLIMETRES = 1,
    UNSCALED = 2,
    DEGREES = 3,
    PERCENT = 4,
};


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
    wxLocale*   m_wxLocale;
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
 * Replace any environment variable & text variable references with their values.
 *
 * @param aString = a string containing (perhaps) references to env var
 * @return a string where env var are replaced by their value
 */
const wxString ExpandEnvVarSubstitutions( const wxString& aString, PROJECT* aProject );

/**
 * Expand '${var-name}' templates in text.  The LocalResolver is given first crack at it,
 * after which the PROJECT's resolver is called.
 */
wxString ExpandTextVars( const wxString& aSource,
                         const std::function<bool( wxString* )>* aLocalResolver,
                         const PROJECT* aProject );

/**
 * Replace any environment and/or text variables in file-path uris (leaving network-path URIs
 * alone).
 */
const wxString ResolveUriByEnvVars( const wxString& aUri, PROJECT* aProject );


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
