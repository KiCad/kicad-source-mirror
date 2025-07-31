/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

// Date for KiCad build version
#include <wx/wx.h>
#include <config.h>
#include <boost/version.hpp>
#include <kiplatform/app.h>
#include <font/version_info.h>
#include <build_version.h>

#include <tuple>
#include <mutex>

// kicad_curl.h must be included before wx headers, to avoid
// conflicts for some defines, at least on Windows
// kicad_curl.h can create conflicts for some defines, at least on Windows
// so we are using here 2 proxy functions to know Curl version to avoid
// including kicad_curl.h to know Curl version
extern std::string GetKicadCurlVersion();
extern std::string GetCurlLibVersion();

#include <Standard_Version.hxx>

#include <ngspice/sharedspice.h>

// The include file version.h is always created even if the repo version cannot be
// determined.  In this case KICAD_VERSION_FULL will default to the KICAD_VERSION
// that is set in KiCadVersion.cmake.
#define INCLUDE_KICAD_VERSION
#include <kicad_build_version.h>
#undef INCLUDE_KICAD_VERSION

// Mutex for wxPlatformInfo
static std::recursive_mutex s_platformInfoMutex;

// Remember OpenGL info
static wxString s_glVendor;
static wxString s_glRenderer;
static wxString s_glVersion;

void SetOpenGLInfo( const char* aVendor, const char* aRenderer, const char* aVersion )
{
    s_glVendor = wxString::FromUTF8( aVendor );
    s_glRenderer = wxString::FromUTF8( aRenderer );
    s_glVersion = wxString::FromUTF8( aVersion );
}


wxString GetPlatformGetBitnessName()
{
    // wxPlatformInfo is not thread-safe, so protect it
    std::unique_lock lock(s_platformInfoMutex);

    return wxPlatformInfo().GetBitnessName();
}


bool IsNightlyVersion()
{
    return !!KICAD_IS_NIGHTLY;
}


wxString GetBuildVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_VERSION_FULL ) );
    return msg;
}


wxString GetBaseVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_VERSION ) );
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


wxString GetCommitHash()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_COMMIT_HASH ) );
    return msg;
}


wxString GetMajorMinorPatchVersion()
{
    wxString msg = wxString::Format( wxT( "%s" ), wxT( KICAD_MAJOR_MINOR_PATCH_VERSION ) );
    return msg;
}


const std::tuple<int,int,int>& GetMajorMinorPatchTuple()
{
    static std::tuple<int, int, int> retval = KICAD_MAJOR_MINOR_PATCH_TUPLE;

    return retval;
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
    version << ( KIPLATFORM::APP::IsOperatingSystemUnsupported() ? wxString( wxS( "(UNSUPPORTED)" ) )
                                                                 : GetBuildVersion() )
#ifdef DEBUG
            << ", debug"
#else
            << ", release"
#endif
            << " build";

    aMsg << "Application: " << aTitle;
    aMsg << " " << wxGetCpuArchitectureName() << " on " << wxGetNativeCpuArchitectureName();

    aMsg << eol << eol;


    aMsg << "Version: " << version << eol << eol;
    aMsg << "Libraries:" << eol;

    aMsg << indent4 << wxGetLibraryVersionInfo().GetVersionString() << eol;

    aMsg << indent4 << "FreeType " << KIFONT::VERSION_INFO::FreeType() << eol;
    aMsg << indent4 << "HarfBuzz " << KIFONT::VERSION_INFO::HarfBuzz() << eol;
    aMsg << indent4 << "FontConfig " << KIFONT::VERSION_INFO::FontConfig() << eol;

    if( !aBrief )
        aMsg << indent4 << GetKicadCurlVersion() << eol;

    aMsg << eol;

    wxString osDescription;

#if __LINUX__
    osDescription = wxGetLinuxDistributionInfo().Description;
#endif

    // Linux uses the lsb-release program to get the description of the OS, if lsb-release
    // isn't installed, then the string will be empty and we fallback to the method used on
    // the other platforms (to at least get the kernel/uname info).
    if( osDescription.empty() )
        osDescription = wxGetOsDescription();

    {
        // wxPlatformInfo is not thread-safe, so protect it
        std::unique_lock lock( s_platformInfoMutex );

        aMsg << "Platform: "
            << osDescription << ", "
            << GetPlatformGetBitnessName() << ", "
            << wxPlatformInfo().GetEndiannessName() << ", "
            << wxPlatformInfo().GetPortIdName();
    }

#ifdef __WXGTK__
    if( wxTheApp && wxTheApp->IsGUI() )
    {
        aMsg << ", ";

        switch( wxGetDisplayInfo().type )
        {
        case wxDisplayX11:     aMsg << "X11"; break;
        case wxDisplayWayland: aMsg << "Wayland"; break;
        case wxDisplayNone:    aMsg << "None"; break;
        default:               aMsg << "Unknown";
        }
    }

    aMsg << ", " << wxGetenv( "XDG_SESSION_DESKTOP" )
         << ", " << wxGetenv( "XDG_SESSION_TYPE" );
#endif

    if( !s_glVendor.empty() || !s_glRenderer.empty() || !s_glVersion.empty() )
    {
        aMsg << eol;
        aMsg << "OpenGL: " << s_glVendor << ", " << s_glRenderer << ", " << s_glVersion;
    }

    aMsg << eol << eol;

    if( !aBrief )
    {
        aMsg << "Build Info:" << eol;
        aMsg << indent4 << "Date: " << GetBuildDate() << eol;
    }

    aMsg << indent4 << "wxWidgets: " << wxVERSION_NUM_DOT_STRING << " (";
    aMsg << __WX_BO_UNICODE __WX_BO_STL;

// wx changed compatibility macros in 3.3, adding the 3.0 macro and removing the 2.8 macro
#if wxCHECK_VERSION( 3, 3, 0 )
    aMsg << __WX_BO_WXWIN_COMPAT_3_0 ")";
#else
    aMsg << __WX_BO_WXWIN_COMPAT_2_8 ")";
#endif

    // Get the GTK+ version where possible.
#ifdef __WXGTK__
    {
        // wxPlatformInfo is not thread-safe, so protect it
        std::unique_lock lock( s_platformInfoMutex );
        int              major, minor;

        major = wxPlatformInfo().GetToolkitMajorVersion();
        minor = wxPlatformInfo().GetToolkitMinorVersion();
        aMsg << " GTK+ " << major << "." << minor;
    }
#endif

    aMsg << eol;

    aMsg << indent4 << "Boost: " << ( BOOST_VERSION / 100000 ) << wxT( "." )
         << ( BOOST_VERSION / 100 % 1000 ) << wxT( "." )
         << ( BOOST_VERSION % 100 ) << eol;

    aMsg << indent4 << "OCC: " << OCC_VERSION_COMPLETE << eol;
    aMsg << indent4 << "Curl: " << GetCurlLibVersion() << eol;

#if defined( NGSPICE_BUILD_VERSION )
    aMsg << indent4 << "ngspice: " << NGSPICE_BUILD_VERSION << eol;
#elif defined( NGSPICE_HAVE_CONFIG_H )
    #undef HAVE_STRNCASECMP     /* is redefined in ngspice/config.h */
    #include <ngspice/config.h>
    aMsg << indent4 << "ngspice: " << PACKAGE_VERSION << eol;
#elif defined( NGSPICE_PACKAGE_VERSION )
    aMsg << indent4 << "ngspice: " << NGSPICE_PACKAGE_VERSION << eol;
#else
    aMsg << indent4 << "ngspice: " << "unknown" << eol;
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

    // Add build settings config (build options):
#if defined( KICAD_USE_EGL ) || ! defined( NDEBUG )
    aMsg << eol;
    aMsg << "Build settings:" << eol;
#endif

#ifdef KICAD_USE_EGL
    aMsg << indent4 << "KICAD_USE_EGL=" << ON;
#endif

#ifdef KICAD_IPC_API
    aMsg << indent4 << "KICAD_IPC_API=" << ON;
#else
    aMsg << indent4 << "KICAD_IPC_API=" << OFF;
#endif

    aMsg << indent4 << "KICAD_USE_PCH=";
#ifdef KICAD_USE_PCH
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

    aMsg << indent4 << "KICAD_SANITIZE_ADDRESS=";
#ifdef KICAD_SANITIZE_ADDRESS
    aMsg << ON;
#else
    aMsg << OFF;
#endif

    aMsg << indent4 << "KICAD_SANITIZE_THREADS=";
#ifdef KICAD_SANITIZE_THREADS
    aMsg << ON;
#else
    aMsg << OFF;
#endif
#endif

    wxLocale* locale = wxGetLocale();

    if( locale )
    {
        aMsg << eol;
        aMsg << "Locale: " << eol;
        aMsg << indent4 << "Lang: " << locale->GetCanonicalName() << eol;
        aMsg << indent4 << "Enc: " << locale->GetSystemEncodingName() << eol;
        aMsg << indent4 << "Num: "
             << wxString::Format( "%d%s%.1f", 1,
                                  locale->GetInfo( wxLocaleInfo::wxLOCALE_THOUSANDS_SEP ), 234.5 )
             << eol;

        wxString testStr( wxS( "кΩ丈" ) );
        wxString expectedUtf8Hex( wxS( "D0BACEA9E4B888" ) );
        wxString sysHex, utf8Hex;
        {
            const char* asChar = testStr.c_str().AsChar();
            size_t      length = strlen( asChar );

            for( size_t i = 0; i < length; i++ )
                sysHex << wxString::Format( "%02X", (unsigned int) (uint8_t) asChar[i] );
        }
        {
            const char* asChar = testStr.utf8_str().data();
            size_t      length = strlen( asChar );

            for( size_t i = 0; i < length; i++ )
                utf8Hex << wxString::Format( "%02X", (unsigned int) (uint8_t) asChar[i] );
        }

        aMsg << indent4 << "Encoded " << testStr << ": " << sysHex << " (sys), " << utf8Hex
             << " (utf8)" << eol;

        wxASSERT_MSG( utf8Hex == expectedUtf8Hex,
                      wxString::Format( "utf8_str string %s encoding bad result: %s, expected "
                                        "%s, system enc %s, lang %s",
                                        testStr, utf8Hex, expectedUtf8Hex,
                                        locale->GetSystemEncodingName(),
                                        locale->GetCanonicalName() ) );
    }

    return aMsg;
}
