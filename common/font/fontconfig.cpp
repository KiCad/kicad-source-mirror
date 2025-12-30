/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2023 CERN (www.cern.ch)
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mutex>
#include <font/fontconfig.h>
#include <wx/log.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <macros.h>
#include <cstdint>
#include <reporter.h>
#include <embedded_files.h>

#ifdef __WIN32__
#include <windows.h>
#endif

using namespace fontconfig;

static FONTCONFIG* g_config = nullptr;
static bool        g_fcInitSuccess = false;

REPORTER* FONTCONFIG::s_reporter = nullptr;
static std::mutex g_fontConfigMutex;

/**
 * A simple wrapper to avoid exporting fontconfig in the header
 */
struct fontconfig::FONTCONFIG_PAT
{
    FcPattern* pat;
};


wxString FONTCONFIG::Version()
{
    return wxString::Format( "%d.%d.%d", FC_MAJOR, FC_MINOR, FC_REVISION );
}


FONTCONFIG::FONTCONFIG()
{
};


void fontconfig::FONTCONFIG::SetReporter( REPORTER* aReporter )
{
    std::lock_guard lock( g_fontConfigMutex );
    s_reporter = aReporter;
}


REPORTER* fontconfig::FONTCONFIG::GetReporter()
{
    std::lock_guard lock( g_fontConfigMutex );
    return s_reporter;
}


/**
 * This is simply a wrapper to call FcInit() with SEH for Windows.
 *
 * SEH on Windows can only be used in functions without objects that might be unwound
 * (basically objects with destructors).
 * For example, new FONTCONFIG() in Fontconfig() is creating a object with a destructor
 * that *might* need to be unwound. MSVC catches this and throws a compile error.
 */
static void bootstrapFc()
{
#if defined( _MSC_VER )
    __try
    {
#endif
        FcInit();
        g_fcInitSuccess = true;
#if defined( _MSC_VER )
    }
    __except( GetExceptionCode() == STATUS_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER
                                                         : EXCEPTION_CONTINUE_SEARCH )
    {
        g_fcInitSuccess = false;

        // We have documented cases that fontconfig while trying to cache fonts
        // ends up using freetype to try and get font info
        // freetype itself reads fonts through memory mapping instead of normal file APIs
        // there are crashes reading fonts sometimes as a result that return STATUS_IN_PAGE_ERROR
    }
#endif
}


FONTCONFIG* Fontconfig()
{
    if( !g_config )
    {
        bootstrapFc();
        g_config = new FONTCONFIG();
    }

    return g_config;
}


bool FONTCONFIG::isLanguageMatch( const wxString& aSearchLang, const wxString& aSupportedLang )
{
    if( aSearchLang.Lower() == aSupportedLang.Lower() )
        return true;

    if( aSupportedLang.empty() )
        return false;

    if( aSearchLang.empty() )
        return false;

    wxArrayString supportedLangBits;
    wxStringSplit( aSupportedLang.Lower(), supportedLangBits, wxS( '-' ) );

    wxArrayString searhcLangBits;
    wxStringSplit( aSearchLang.Lower(), searhcLangBits, wxS( '-' ) );

    // if either side of the comparison have only one section, then its a broad match but fine
    // i.e. the haystack is declaring broad support or the search language is broad
    if( searhcLangBits.size() == 1 || supportedLangBits.size() == 1 )
    {
        return searhcLangBits[0] == supportedLangBits[0];
    }

    // the full two part comparison should have passed the initial shortcut

    return false;
}


std::string FONTCONFIG::getFcString( FONTCONFIG_PAT& aPat, const char* aObj, int aIdx )
{
    FcChar8*    str;
    std::string res;

    if( FcPatternGetString( aPat.pat, aObj, aIdx, &str ) == FcResultMatch )
        res = std::string( reinterpret_cast<char*>( str ) );

    return res;
}


void FONTCONFIG::getAllFamilyStrings( FONTCONFIG_PAT&                               aPat,
                                      std::unordered_map<std::string, std::string>& aFamStringMap )
{
    std::string famLang;
    std::string fam;

    int langIdx = 0;

    do
    {
        famLang = getFcString( aPat, FC_FAMILYLANG, langIdx );

        if( famLang.empty() && langIdx != 0 )
        {
            break;
        }
        else
        {
            fam = getFcString( aPat, FC_FAMILY, langIdx );
            aFamStringMap.insert_or_assign( famLang, fam );
        }
    } while( langIdx++ < std::numeric_limits<
                     int8_t>::max() ); //arbitrary to avoid getting stuck for any reason
}


std::string FONTCONFIG::getFamilyStringByLang( FONTCONFIG_PAT& aPat, const wxString& aDesiredLang )
{
    std::unordered_map<std::string, std::string> famStrings;
    getAllFamilyStrings( aPat, famStrings );

    if( famStrings.empty() )
        return "";

    for( auto const& [key, val] : famStrings )
    {
        if( isLanguageMatch( aDesiredLang, From_UTF8( key.c_str() ) ) )
        {
            return val;
        }
    }

    // fall back to the first and maybe only available name
    // most fonts by review don't even bother declaring more than one font family name
    // and they don't even bother declare the language tag either, they just leave it blank
    return famStrings.begin()->second;
}


FONTCONFIG::FF_RESULT FONTCONFIG::FindFont( const wxString& aFontName, wxString& aFontFile,
                                            int& aFaceIndex, bool aBold, bool aItalic,
                                            const std::vector<wxString>* aEmbeddedFiles )
{
    FF_RESULT retval = FF_RESULT::FF_ERROR;

    if( !g_fcInitSuccess )
        return retval;

    // If the original font name contains any of these, then it is bold, regardless
    // of whether we are looking for bold or not
    if( aFontName.Lower().Contains( wxS( "bold" ) )       // also catches ultrabold
        || aFontName.Lower().Contains( wxS( "heavy" ) )
        || aFontName.Lower().Contains( wxS( "black" ) )   // also catches extrablack
        || aFontName.Lower().Contains( wxS( "thick" ) )
        || aFontName.Lower().Contains( wxS( "dark" ) ) )
    {
        aBold = true;
    }

    FcConfig* config = FcConfigGetCurrent();

    if( aEmbeddedFiles )
    {
        for( const auto& file : *aEmbeddedFiles )
        {
            FcConfigAppFontAddFile( config, (const FcChar8*) file.c_str().AsChar() );
        }
    }

    wxString qualifiedFontName = aFontName;

    wxScopedCharBuffer const fcBuffer = qualifiedFontName.ToUTF8();

    FcPattern* pat = FcPatternCreate();

    if( aBold )
        FcPatternAddString( pat, FC_STYLE, (const FcChar8*) "Bold" );

    if( aItalic )
        FcPatternAddString( pat, FC_STYLE, (const FcChar8*) "Italic" );

    FcPatternAddString( pat, FC_FAMILY, (FcChar8*) fcBuffer.data() );

    FcConfigSubstitute( config, pat, FcMatchPattern );
    FcDefaultSubstitute( pat );

    FcResult   r = FcResultNoMatch;
    FcPattern* font = FcFontMatch( config, pat, &r );

    wxString fontName;

    if( font )
    {
        FcChar8* file = nullptr;

        if( FcPatternGetString( font, FC_FILE, 0, &file ) == FcResultMatch )
        {
            aFontFile = wxString::FromUTF8( (char*) file );
            aFaceIndex = 0;

            wxString styleStr;
            FcChar8* family = nullptr;
            FcChar8* style = nullptr;

            retval = FF_RESULT::FF_SUBSTITUTE;

            std::unordered_map<std::string, std::string> famStrings;
            FONTCONFIG_PAT                               patHolder{ font };

            getAllFamilyStrings( patHolder, famStrings );

            if( FcPatternGetString( font, FC_FAMILY, 0, &family ) == FcResultMatch )
            {
                FcPatternGetInteger( font, FC_INDEX, 0, &aFaceIndex );

                fontName = wxString::FromUTF8( (char*) family );

                if( FcPatternGetString( font, FC_STYLE, 0, &style ) == FcResultMatch )
                {
                    styleStr = wxString::FromUTF8( (char*) style );

                    if( !styleStr.IsEmpty() )
                    {
                        styleStr.Replace( ' ', ':' );
                        fontName += ':' + styleStr;
                    }
                }

                bool has_bold = false;
                bool has_ital = false;
                wxString lower_style = styleStr.Lower();

                if( lower_style.Contains( wxS( "thin" ) )
                         || lower_style.Contains( wxS( "light" ) )   // catches ultra & extra light
                         || lower_style.Contains( wxS( "regular" ) )
                         || lower_style.Contains( wxS( "roman" ) )
                         || lower_style.Contains( wxS( "book" ) ) )
                {
                    has_bold = false;
                }
                else if( lower_style.Contains( wxS( "medium" ) )
                         || lower_style.Contains( wxS( "semibold" ) )
                         || lower_style.Contains( wxS( "demibold" ) ) )
                {
                    has_bold = aBold;
                }
                else if( lower_style.Contains( wxS( "bold" ) )       // also catches ultrabold
                         || lower_style.Contains( wxS( "heavy" ) )
                         || lower_style.Contains( wxS( "black" ) )   // also catches extrablack
                         || lower_style.Contains( wxS( "thick" ) )
                         || lower_style.Contains( wxS( "dark" ) ) )
                {
                    has_bold = true;
                }

                if( lower_style.Contains( wxS( "italic" ) )
                        || lower_style.Contains( wxS( "oblique" ) )
                        || lower_style.Contains( wxS( "slant" ) ) )
                {
                    has_ital = true;
                }

                for( auto const& [key, val] : famStrings )
                {
                    wxString searchFont;
                    searchFont = wxString::FromUTF8( (char*) val.data() );

                    if( searchFont.Lower().StartsWith( aFontName.Lower() ) )
                    {
                        if( ( aBold && !has_bold ) && ( aItalic && !has_ital ) )
                            retval = FF_RESULT::FF_MISSING_BOLD_ITAL;
                        else if( aBold && !has_bold )
                            retval = FF_RESULT::FF_MISSING_BOLD;
                        else if( aItalic && !has_ital )
                            retval = FF_RESULT::FF_MISSING_ITAL;
                        else if( ( aBold != has_bold ) || ( aItalic != has_ital ) )
                            retval = FF_RESULT::FF_SUBSTITUTE;
                        else
                            retval = FF_RESULT::FF_OK;

                        break;
                    }
                }
            }
        }

        FcPatternDestroy( font );
    }

    if( retval == FF_RESULT::FF_ERROR )
    {
        if( s_reporter )
            s_reporter->Report( wxString::Format( _( "Error loading font '%s'." ),
                                                  qualifiedFontName ) );
    }
    else if( retval == FF_RESULT::FF_SUBSTITUTE )
    {
        fontName.Replace( ':', ' ' );

        // If we missed a case but the matching found the original font name, then we are
        // not substituting
        if( fontName.CmpNoCase( qualifiedFontName ) == 0 )
            retval = FF_RESULT::FF_OK;
        else if( s_reporter )
            s_reporter->Report( wxString::Format( _( "Font '%s' not found; substituting '%s'." ),
                                                  qualifiedFontName, fontName ) );
    }

    FcPatternDestroy( pat );
    return retval;
}


void FONTCONFIG::ListFonts( std::vector<std::string>& aFonts, const std::string& aDesiredLang,
                            const std::vector<wxString>* aEmbeddedFiles, bool aForce )
{
    if( !g_fcInitSuccess )
        return;

    // be sure to cache bust if the language changed
    if( m_fontInfoCache.empty() || m_fontCacheLastLang != aDesiredLang || aForce )
    {
        FcConfig* config = FcConfigGetCurrent();

        if( aEmbeddedFiles )
        {
            for( const auto& file : *aEmbeddedFiles )
            {
                FcConfigAppFontAddFile( config, (const FcChar8*) file.c_str().AsChar() );
            }
        }

        FcPattern*   pat = FcPatternCreate();
        FcObjectSet* os = FcObjectSetBuild( FC_FAMILY, FC_FAMILYLANG, FC_STYLE, FC_LANG, FC_FILE,
                                            FC_OUTLINE, nullptr );
        FcFontSet*   fs = FcFontList( config, pat, os );

        for( int i = 0; fs && i < fs->nfont; ++i )
        {
            FcPattern* font = fs->fonts[i];
            FcChar8*   file;
            FcChar8*   style;
            FcLangSet* langSet;
            FcBool     outline;

            if( FcPatternGetString( font, FC_FILE, 0, &file ) == FcResultMatch
                && FcPatternGetString( font, FC_STYLE, 0, &style ) == FcResultMatch
                && FcPatternGetLangSet( font, FC_LANG, 0, &langSet ) == FcResultMatch
                && FcPatternGetBool( font, FC_OUTLINE, 0, &outline ) == FcResultMatch )
            {
                if( !outline )
                    continue;

                FONTCONFIG_PAT patHolder{ font };
                std::string    theFamily =
                        getFamilyStringByLang( patHolder, From_UTF8( aDesiredLang.c_str() ) );

#ifdef __WXMAC__
                // On Mac (at least) some of the font names are in their own language.  If
                // the OS doesn't support this language then we get a bunch of garbage names
                // in the font menu.
                //
                // GTK, on the other hand, doesn't appear to support wxLocale::IsAvailable(),
                // so we can't run these checks.

                static std::map<wxString, bool> availableLanguages;

                FcStrSet*  langStrSet = FcLangSetGetLangs( langSet );
                FcStrList* langStrList = FcStrListCreate( langStrSet );
                FcChar8*   langStr = FcStrListNext( langStrList );
                bool       langSupported = false;

                if( !langStr )
                {
                    // Symbol fonts (Wingdings, etc.) have no language
                    langSupported = true;
                }
                else while( langStr )
                {
                    wxString langWxStr( reinterpret_cast<char *>( langStr ) );

                    if( availableLanguages.find( langWxStr ) == availableLanguages.end() )
                    {
                        const wxLanguageInfo* langInfo = wxLocale::FindLanguageInfo( langWxStr );
                        bool  available = langInfo && wxLocale::IsAvailable( langInfo->Language );

                        availableLanguages[ langWxStr ] = available;
                    }

                    if( availableLanguages[ langWxStr ] )
                    {
                        langSupported = true;
                        break;
                    }
                    else
                    {
                        wxLogTrace( traceFonts,
                                    wxS( "Font '%s' language '%s' not supported by OS." ),
                                    theFamily, langWxStr );
                    }

                    langStr = FcStrListNext( langStrList );
                }

                FcStrListDone( langStrList );
                FcStrSetDestroy( langStrSet );

                if( !langSupported )
                    continue;
#endif

                std::string theFile( reinterpret_cast<char *>( file ) );
                std::string theStyle( reinterpret_cast<char *>( style ) );
                FONTINFO    fontInfo( std::move( theFile ), std::move( theStyle ), theFamily );

                if( theFamily.length() > 0 && theFamily.front() == '.' )
                    continue;

                std::map<std::string, FONTINFO>::iterator it = m_fontInfoCache.find( theFamily );

                if( it == m_fontInfoCache.end() )
                    m_fontInfoCache.emplace( theFamily, fontInfo );
                else
                    it->second.Children().push_back( fontInfo );
            }
        }

        if( fs )
            FcFontSetDestroy( fs );

        m_fontCacheLastLang = aDesiredLang;
    }

    for( const std::pair<const std::string, FONTINFO>& entry : m_fontInfoCache )
        aFonts.push_back( entry.second.Family() );
}

