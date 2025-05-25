/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.TXT for contributors.
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

#pragma once
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pgm_base.h>
#include <config.h>

// Suppress a warning that the mock methods don't override the base class methods because
// turtlemocks doesn't seem to provide a way to actually annotate the methods with override.
#ifdef HAVE_WINCONSISTENT_MISSING_OVERRIDE
    _Pragma( "GCC diagnostic push" ) \
    _Pragma( "GCC diagnostic ignored \"-Winconsistent-missing-override\"" )
#endif

MOCK_BASE_CLASS( MOCK_PGM_BASE, PGM_BASE )
{
    MOCK_PGM_BASE() : PGM_BASE() {};
    virtual ~MOCK_PGM_BASE() {};

    MOCK_METHOD( MacOpenFile, 1, void( const wxString& ) );
    MOCK_METHOD( SetTextEditor, 1, void( const wxString& ) );
    MOCK_METHOD( GetTextEditor, 1, wxString&( bool ) );
    MOCK_METHOD( AskUserForPreferredEditor, 1, const wxString( const wxString& ) );
    MOCK_CONST_METHOD( IsKicadEnvVariableDefined, 0, bool() );
    MOCK_CONST_METHOD( GetKicadEnvVariable, 0, const wxString&() );
    MOCK_CONST_METHOD( GetPdfBrowserName, 0, const wxString&() );
    MOCK_METHOD( SetPdfBrowserName, 1, void( const wxString& ) );
    MOCK_CONST_METHOD( UseSystemPdfBrowser, 0, bool() );
    MOCK_METHOD( ForceSystemPdfBrowser, 1, void( bool ) );
    MOCK_METHOD( SetLanguage, 2, bool( wxString&, bool ) );
    MOCK_METHOD( SetLanguageIdentifier, 1, void( int ) );
    MOCK_METHOD( ReadPdfBrowserInfos, 0, void() );
    MOCK_METHOD( WritePdfBrowserInfos, 0, void() );
    MOCK_METHOD( SetLocalEnvVariable, 2, bool( const wxString&, const wxString& ) );
    MOCK_METHOD( SetLocalEnvVariables, 0, void() );

    int GetSelectedLanguageIdentifier() const override
    {
        return 0;
    }

    // following functions will not be mocked in order to mimic old qa behavior
//    MOCK_METHOD( App, 0, wxApp&() );
//    MOCK_METHOD( GetLocale, 0, wxLocale*() );
//    MOCK_METHOD( GetSettingsManager, 0, SETTINGS_MANAGER&() );
//    MOCK_METHOD( SetLanguagePath, 0, void() );
//    MOCK_CONST_METHOD( GetExecutablePath, 0, const wxString&() );
//    MOCK_METHOD( GetSettingsManager, 0, SETTINGS_MANAGER&() );
};

#ifdef HAVE_WINCONSISTENT_MISSING_OVERRIDE
    _Pragma( "GCC diagnostic pop" )
#endif
