/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/**
 * Regression tests for issue 24943, design variant edits on multi-unit symbols.
 *
 * With a variant active, an edit to one unit is stored as a variant differential
 * on that unit while the cross-unit sync wrote plain base field text to the
 * sibling units.  The sibling must instead receive the same variant differential
 * and keep its base value untouched.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <lib_symbol.h>
#include <sch_commit.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct ISSUE24943_FIXTURE
{
    ISSUE24943_FIXTURE()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT( "test_issue24943.kicad_pro" );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
        m_schematic->CreateDefaultScreens();

        m_sheet = m_schematic->GetTopLevelSheets()[0];
        SCH_SCREEN* screen = m_sheet->GetScreen();
        screen->SetFileName( "issue24943.kicad_sch" );

        m_path.clear();
        m_path.push_back( m_sheet );

        m_libSym = std::make_unique<LIB_SYMBOL>( "OPAMP", nullptr );

        SCH_PIN* pin = new SCH_PIN( m_libSym.get() );
        pin->SetNumber( "1" );
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        m_libSym->AddDrawItem( pin );

        m_libSym->SetUnitCount( 2, true );

        m_unitA = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &m_path, 1, 0, VECTOR2I( 0, 0 ) );
        m_unitA->UpdatePins();
        screen->Append( m_unitA );

        m_unitB = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &m_path, 2, 0, VECTOR2I( 2540000, 0 ) );
        m_unitB->UpdatePins();
        screen->Append( m_unitB );

        m_unitA->SetRef( &m_path, wxS( "U1" ) );
        m_unitB->SetRef( &m_path, wxS( "U1" ) );

        m_baseValue = wxS( "OPAMP" );
        m_unitA->GetField( FIELD_T::VALUE )->SetText( m_baseValue );
        m_unitB->GetField( FIELD_T::VALUE )->SetText( m_baseValue );
    }

    void sync( SCH_SYMBOL* aSource, const wxString& aVariant )
    {
        TOOL_MANAGER mgr;
        SCH_COMMIT   commit( &mgr );

        aSource->SyncOtherUnits( m_path, commit, nullptr, aVariant );
    }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
    SCH_SHEET*                  m_sheet = nullptr;
    SCH_SHEET_PATH              m_path;
    SCH_SYMBOL*                 m_unitA = nullptr;
    SCH_SYMBOL*                 m_unitB = nullptr;
    wxString                    m_baseValue;
};


BOOST_FIXTURE_TEST_SUITE( Issue24943VariantUnitSync, ISSUE24943_FIXTURE )


/**
 * A value assigned under a variant must reach the sibling unit as the same
 * variant differential, and no base value may change.
 */
BOOST_AUTO_TEST_CASE( ValueSyncUnderVariant )
{
    m_unitA->SetValueFieldText( wxS( "AAA" ), &m_path, wxS( "V1" ) );
    sync( m_unitA, wxS( "V1" ) );

    BOOST_CHECK_EQUAL( m_unitB->GetValue( false, &m_path, false, wxS( "V1" ) ), wxS( "AAA" ) );

    BOOST_CHECK_EQUAL( m_unitA->GetValue( false, &m_path, false ), m_baseValue );
    BOOST_CHECK_EQUAL( m_unitB->GetValue( false, &m_path, false ), m_baseValue );
}


/**
 * Same contract for user fields, which take the updateOtherFields branch.
 */
BOOST_AUTO_TEST_CASE( UserFieldSyncUnderVariant )
{
    for( SCH_SYMBOL* unit : { m_unitA, m_unitB } )
    {
        SCH_FIELD mpn( unit, FIELD_T::USER, wxS( "MPN" ) );
        mpn.SetText( wxS( "111" ) );
        unit->AddField( mpn );
    }

    m_unitA->SetFieldText( wxS( "MPN" ), wxS( "222" ), &m_path, wxS( "V1" ) );
    sync( m_unitA, wxS( "V1" ) );

    BOOST_CHECK_EQUAL( m_unitB->GetFieldText( wxS( "MPN" ), &m_path, wxS( "V1" ) ), wxS( "222" ) );
    BOOST_CHECK_EQUAL( m_unitB->GetField( wxS( "MPN" ) )->GetText(), wxS( "111" ) );
}


/**
 * Exclude flags set under a variant must sync as variant overrides, base
 * flags stay untouched.
 */
BOOST_AUTO_TEST_CASE( ExcludeFlagSyncUnderVariant )
{
    m_unitA->SetExcludedFromBOM( true, &m_path, wxS( "V1" ) );
    sync( m_unitA, wxS( "V1" ) );

    BOOST_CHECK( m_unitB->GetExcludedFromBOM( &m_path, wxS( "V1" ) ) );
    BOOST_CHECK( !m_unitB->GetExcludedFromBOM( &m_path ) );
}


/**
 * Positive control, DNP sync was already variant aware and must stay so.
 */
BOOST_AUTO_TEST_CASE( DnpSyncUnderVariantControl )
{
    m_unitA->SetDNP( true, &m_path, wxS( "V1" ) );
    sync( m_unitA, wxS( "V1" ) );

    BOOST_CHECK( m_unitB->GetDNP( &m_path, wxS( "V1" ) ) );
    BOOST_CHECK( !m_unitB->GetDNP( &m_path ) );
}


/**
 * Negative control, the no-variant sync path keeps writing base values.
 */
BOOST_AUTO_TEST_CASE( ValueSyncWithoutVariant )
{
    m_unitA->SetValueFieldText( wxS( "BBB" ), &m_path, wxEmptyString );
    sync( m_unitA, wxEmptyString );

    BOOST_CHECK_EQUAL( m_unitB->GetValue( false, &m_path, false ), wxS( "BBB" ) );
}


BOOST_AUTO_TEST_SUITE_END()
