/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fontconfig/fontconfig.h>

#include <font/fontconfig.h>
#include <pgm_base.h>
#include <wx/log.h>
#include <trace_helpers.h>

using namespace fontconfig;

static FONTCONFIG* g_config = nullptr;


inline static FcChar8* wxStringToFcChar8( const wxString& str )
{
    wxScopedCharBuffer const fcBuffer = str.ToUTF8();
    return (FcChar8*) fcBuffer.data();
}


FONTCONFIG::FONTCONFIG()
{
    (void) FcInitLoadConfigAndFonts();
};


FONTCONFIG* Fontconfig()
{
    if( !g_config )
    {
        FcInit();
        g_config = new FONTCONFIG();
    }

    return g_config;
}


bool FONTCONFIG::FindFont( const wxString& aFontName, wxString& aFontFile )
{
    FcPattern* pat = FcNameParse( wxStringToFcChar8( aFontName ) );
    FcConfigSubstitute( nullptr, pat, FcMatchPattern );
    FcDefaultSubstitute( pat );

    FcResult   r = FcResultNoMatch;
    FcPattern* font = FcFontMatch( nullptr, pat, &r );

    wxString fontName;
    bool     ok = false;
    bool     substituted = false;

    if( font )
    {
        FcChar8* file = nullptr;

        if( FcPatternGetString( font, FC_FILE, 0, &file ) == FcResultMatch )
        {
            aFontFile = wxString::FromUTF8( (char*) file );

            FcChar8* family = nullptr;
            FcChar8* style = nullptr;

            if( FcPatternGetString( font, FC_FAMILY, 0, &family ) == FcResultMatch )
            {
                fontName = wxString::FromUTF8( (char*) family );

                if( FcPatternGetString( font, FC_STYLE, 0, &style ) == FcResultMatch )
                {
                    wxString styleStr = wxString::FromUTF8( (char*) style );

                    if( !styleStr.IsEmpty() )
                    {
                        styleStr.Replace( " ", ":" );
                        fontName += ":" + styleStr;
                    }
                }

                // TODO: report Regular vs Book, Italic vs Oblique, etc. or filter them out?

                if( aFontName.Contains( ":" ) )
                    substituted = aFontName.CmpNoCase( fontName ) != 0;
                else
                    substituted = !fontName.StartsWith( aFontName );
            }

            ok = true;
        }

        FcPatternDestroy( font );
    }

    if( !ok )
        wxLogWarning( _( "Error loading font '%s'." ), aFontName );
    else if( substituted )
        wxLogWarning( _( "Font '%s' not found; substituting '%s'." ), aFontName, fontName );

    FcPatternDestroy( pat );
    return ok;
}


void FONTCONFIG::ListFonts( std::vector<std::string>& aFonts )
{
    if( m_fonts.empty() )
    {
        FcPattern*   pat = FcPatternCreate();
        FcObjectSet* os = FcObjectSetBuild( FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, FC_OUTLINE,
                                            nullptr );
        FcFontSet*   fs = FcFontList( nullptr, pat, os );

        for( int i = 0; fs && i < fs->nfont; ++i )
        {
            FcPattern* font = fs->fonts[i];
            FcChar8*   file;
            FcChar8*   style;
            FcChar8*   family;
            FcLangSet* langSet;
            FcBool     outline;

            if( FcPatternGetString( font, FC_FILE, 0, &file ) == FcResultMatch
                && FcPatternGetString( font, FC_FAMILY, 0, &family ) == FcResultMatch
                && FcPatternGetString( font, FC_STYLE, 0, &style ) == FcResultMatch
                && FcPatternGetLangSet( font, FC_LANG, 0, &langSet ) == FcResultMatch
                && FcPatternGetBool( font, FC_OUTLINE, 0, &outline ) == FcResultMatch )
            {
                if( !outline )
                    continue;

                std::string theFamily( reinterpret_cast<char *>( family ) );

#ifdef __WXMAC__
                // On Mac (at least) some of the font names are in their own language.  If
                // the OS doesn't support this language then we get a bunch of garbage names
                // in the font menu.
                //
                // GTK, on the other hand, doesn't appear to support wxLocale::IsAvailable(),
                // so we can't run these checks.

                FcStrSet*  langStrSet = FcLangSetGetLangs( langSet );
                FcStrList* langStrList = FcStrListCreate( langStrSet );
                FcChar8*   langStr = FcStrListNext( langStrList );
                bool langSupported = false;

                if( !langStr )
                {
                    // Symbol fonts (Wingdings, etc.) have no language
                    langSupported = true;
                }
                else while( langStr )
                {
                    wxString langWxStr( reinterpret_cast<char *>( langStr ) );
                    const wxLanguageInfo* langInfo = wxLocale::FindLanguageInfo( langWxStr );

                    if( langInfo && wxLocale::IsAvailable( langInfo->Language ) )
                    {
                        langSupported = true;
                        break;
                    }
                    else
                    {
                        wxLogTrace( traceFonts, "Font '%s' language '%s' not supported by OS.",
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
                FONTINFO    fontInfo( theFile, theStyle, theFamily );

                if( theFamily.length() > 0 && theFamily.front() == '.' )
                    continue;

                std::map<std::string, FONTINFO>::iterator it = m_fonts.find( theFamily );

                if( it == m_fonts.end() )
                    m_fonts.emplace( theFamily, fontInfo );
                else
                    it->second.Children().push_back( fontInfo );
            }
        }

        if( fs )
            FcFontSetDestroy( fs );
    }

    for( const std::pair<const std::string, FONTINFO>& entry : m_fonts )
        aFonts.push_back( entry.second.Family() );
}
