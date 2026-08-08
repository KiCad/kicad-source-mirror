/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_variant_field_text_vars.cpp
 * A field value stored on a design variant must resolve text variables the same way the
 * symbol's own field text does. The resolved flag is cached from the field's own text, so a
 * variable that appears only in the variant override was left unresolved and reached the
 * canvas, the BOM and the netlist verbatim.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct VARIANT_TEXT_VARS_FIXTURE
{
    VARIANT_TEXT_VARS_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_variant_text_vars.kicad_pro" );

        m_projectFile = projectPath;
        m_settingsManager.LoadProject( projectPath.ToStdString() );

        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();

        m_sheet = m_schematic->GetTopLevelSheets()[0];
        m_sheet->GetScreen()->SetFileName( wxT( "variant_text_vars.kicad_sch" ) );

        m_path.clear();
        m_path.push_back( m_sheet );

        m_libSym = std::make_unique<LIB_SYMBOL>( wxT( "R" ), nullptr );

        SCH_PIN* pin = new SCH_PIN( m_libSym.get() );
        pin->SetNumber( wxT( "1" ) );
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        m_libSym->AddDrawItem( pin );

        m_symbol = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &m_path, 0, 0, VECTOR2I( 0, 0 ) );
        m_symbol->UpdatePins();
        m_sheet->GetScreen()->Append( m_symbol );

        // The variable the variant override will refer to.
        m_symbol->AddField( SCH_FIELD( m_symbol, FIELD_T::USER, wxT( "MPN" ) ) );
        m_symbol->GetField( wxT( "MPN" ) )->SetText( wxT( "ABC123" ) );
    }

    ~VARIANT_TEXT_VARS_FIXTURE()
    {
        m_schematic.reset();

        if( wxFileExists( m_projectFile ) )
            wxRemoveFile( m_projectFile );
    }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
    SCH_SHEET*                  m_sheet = nullptr;
    SCH_SHEET_PATH              m_path;
    SCH_SYMBOL*                 m_symbol = nullptr;
    wxString                    m_projectFile;
};


BOOST_FIXTURE_TEST_SUITE( VariantFieldTextVars, VARIANT_TEXT_VARS_FIXTURE )


// The symbol's own Value carries no variable, so the cached "has text vars" flag is false.
// The variant override does carry one and must still be resolved.
BOOST_AUTO_TEST_CASE( VariantOverrideResolvesTextVars )
{
    BOOST_REQUIRE( !m_symbol->GetField( FIELD_T::VALUE )->GetText().Contains( wxT( "${" ) ) );

    m_symbol->SetFieldText( wxT( "Value" ), wxT( "${MPN}-ALT" ), &m_path, wxT( "TextVars" ) );

    BOOST_CHECK_EQUAL( m_symbol->GetValue( true, &m_path, false, wxT( "TextVars" ) ), wxT( "ABC123-ALT" ) );
}


// The canvas draws through GetShownText, which took the same shortcut.
BOOST_AUTO_TEST_CASE( VariantOverrideResolvesInShownText )
{
    m_symbol->SetFieldText( wxT( "Description" ), wxT( "${MPN}-DESC" ), &m_path, wxT( "TextVars" ) );

    SCH_FIELD* description = m_symbol->GetField( wxT( "Description" ) );
    BOOST_REQUIRE( description );

    BOOST_CHECK_EQUAL( description->GetShownText( &m_path, false, 0, wxT( "TextVars" ) ), wxT( "ABC123-DESC" ) );
}


// Without a variant nothing changes: the symbol's own text is resolved as it always was.
BOOST_AUTO_TEST_CASE( SymbolOwnTextStillResolves )
{
    m_symbol->GetField( FIELD_T::VALUE )->SetText( wxT( "${MPN}-OWN" ) );

    BOOST_CHECK_EQUAL( m_symbol->GetValue( true, &m_path, false, wxEmptyString ), wxT( "ABC123-OWN" ) );
}


BOOST_AUTO_TEST_SUITE_END()
