/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

// Date for KiCad build version
#include <wx/wx.h>
#include <config.h>
#include <boost/version.hpp>

// kicad_curl.h must be included before wx headers, to avoid
// conflicts for some defines, at least on Windows
// kicad_curl.h can create conflicts for some defines, at least on Windows
// so we are using here 2 proxy functions to know Curl version to avoid
// including kicad_curl.h to know Curl version
extern std::string GetKicadCurlVersion();
extern std::string GetCurlLibVersion();

#if defined( KICAD_USE_OCC ) | defined( KICAD_USE_OCE )
#include <Standard_Version.hxx>
#endif

// The include file version.h is always created even if the repo version cannot be
// determined.  In this case KICAD_VERSION_FULL will default to the KICAD_VERSION
// that is set in KiCadVersion.cmake.
#include <kicad_build_version.h>


wxString GetBuildVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_VERSION_FULL ) );
    return msg;
}


wxString GetBuildDate()
{
    wxString msg = wxString::Format( wxT( "%s %s" ), wxT( __DATE__ ), wxT( __TIME__ ) );
    return msg;
}


wxString GetSemanticVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_SEMANTIC_VERSION ) );
    return msg;
}


wxString GetMajorMinorVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_MAJOR_MINOR_VERSION ) );
    return msg;
}


wxString GetVersionInfoData( const wxString& aTitle, bool aHtml, bool aBrief )
{
    wxString aMsg;
    // DO NOT translate information in the msg_version string

    wxString eol = aHtml ? "<br>" : "\n";

    // Tabs instead of spaces for the plaintext version for shorter string length
    wxString indent4 = aHtml ? "&nbsp;&nbsp;&nbsp;&nbsp;" : "\t";

#define ON "ON" << eol
#define OFF "OFF" << eol

    wxString version;
    version << GetBuildVersion()
#ifdef DEBUG
            << ", debug"
#else
            << ", release"
#endif
            << " build";

    wxPlatformInfo platform;
    aMsg << "Application: " << aTitle << eol << eol;
    aMsg << "Version: " << version << eol << eol;
    aMsg << "Libraries:" << eol;

    aMsg << indent4 << wxGetLibraryVersionInfo().GetVersionString() << eol;

    if( !aBrief )
        aMsg << indent4 << GetKicadCurlVersion() << eol;

    aMsg << eol;

    aMsg << "Platform: " << wxGetOsDescription() << ", "
// TODO (ISM): Remove OSX conditional once our wx fork is running released 3.1.5
#if wxCHECK_VERSION( 3, 1, 5 ) && !defined( __WXOSX__  )
         << platform.GetBitnessName() << ", "
#else
         << platform.GetArchName() << ", "
#endif
         << platform.GetEndiannessName() << ", "
         << platform.GetPortIdName();

#ifdef __WXGTK__
    aMsg << ", " << wxGetenv( "XDG_SESSION_DESKTOP" )
         << ", " << wxGetenv( "XDG_SESSION_TYPE" );
#endif

    aMsg << eol << eol;

    if( !aBrief )
    {
        aMsg << "Build Info:" << eol;
        aMsg << indent4 << "Date: " << GetBuildDate() << eol;
    }

    aMsg << indent4 << "wxWidgets: " << wxVERSION_NUM_DOT_STRING << " (";
    aMsg << __WX_BO_UNICODE __WX_BO_STL __WX_BO_WXWIN_COMPAT_2_8 ")";

    // Get the GTK+ version where possible.
#ifdef __WXGTK__
    int major, minor;

    major = wxPlatformInfo().Get().GetToolkitMajorVersion();
    minor = wxPlatformInfo().Get().GetToolkitMinorVersion();
    aMsg << " GTK+ " <<  major << "." << minor;
#endif

    aMsg << eol;

    aMsg << indent4 << "Boost: " << ( BOOST_VERSION / 100000 ) << wxT( "." )
         << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." )
         << ( BOOST_VERSION % 100 ) << eol;

#ifdef KICAD_USE_OCC
    aMsg << indent4 << "OCC: " << OCC_VERSION_COMPLETE << eol;
#endif

#ifdef KICAD_USE_OCE
    aMsg << indent4 << "OCE: " << OCC_VERSION_COMPLETE << eol;
#endif

    aMsg << indent4 << "Curl: " << GetCurlLibVersion() << eol;

#if defined( KICAD_SPICE )
#if defined( NGSPICE_BUILD_VERSION )
    aMsg << indent4 << "ngspice: " << NGSPICE_BUILD_VERSION << eol;
#elif defined( NGSPICE_HAVE_CONFIG_H )
    #undef HAVE_STRNCASECMP     /* is redefined in ngspice/config.h */
    #include <ngspice/config.h>
    aMsg << indent4 << "ngspice: " << PACKAGE_VERSION << eol;
#else
    aMsg << indent4 << "ngspice: " << "unknown" << eol;
#endif
#endif

    aMsg << indent4 << "Compiler: ";
#if defined(__clang__)
    aMsg << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUG__)
    aMsg << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    aMsg << "Visual C++ " << _MSC_VER;
#elif defined(__INTEL_COMPILER)
    aMsg << "Intel C++ " << __INTEL_COMPILER;
#else
    aMsg << "Other Compiler ";
#endif

#if defined(__GXX_ABI_VERSION)
    aMsg << " with C++ ABI " << __GXX_ABI_VERSION << eol;
#else
    aMsg << " without C++ ABI" << eol;
#endif

    aMsg << eol;

    // Add build settings config (build options):
    aMsg << "Build settings:" << eol;

    aMsg << indent4 << "KICAD_SCRIPTING=";
#ifdef KICAD_SCRIPTING
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_MODULES=";
#ifdef KICAD_SCRIPTING_MODULES
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_PYTHON3=";
#ifdef KICAD_SCRIPTING_PYTHON3
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_WXPYTHON=";
#ifdef KICAD_SCRIPTING_WXPYTHON
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_WXPYTHON_PHOENIX=";
#ifdef KICAD_SCRIPTING_WXPYTHON_PHOENIX
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SCRIPTING_ACTION_MENU=";
#ifdef KICAD_SCRIPTING_ACTION_MENU
    aMsg << ON;
#else
    aMsg << OFF;
#endif

#ifdef KICAD_USE_OCE
    aMsg << indent4 << "KICAD_USE_OCE=" << ON;
#endif

#ifdef KICAD_USE_OCC
    aMsg << indent4 << "KICAD_USE_OCC=" << ON;
#endif

#ifdef KICAD_USE_EGL
    aMsg << indent4 << "KICAD_USE_EGL=" << ON;
#endif

    aMsg << indent4 << "KICAD_SPICE=";
#ifdef KICAD_SPICE
    aMsg << ON;
#else
    aMsg << OFF;
#endif

#ifndef NDEBUG
    aMsg << indent4 << "KICAD_STDLIB_DEBUG=";
#ifdef KICAD_STDLIB_DEBUG
    aMsg << ON;
#else
    aMsg << OFF;
    aMsg << indent4 << "KICAD_STDLIB_LIGHT_DEBUG=";
#ifdef KICAD_STDLIB_LIGHT_DEBUG
    aMsg << ON;
#else
    aMsg << OFF;
#endif
#endif

    aMsg << indent4 << "KICAD_SANITIZE=";
#ifdef KICAD_SANITIZE
    aMsg << ON;
#else
    aMsg << OFF;
#endif
#endif

    return aMsg;
}
