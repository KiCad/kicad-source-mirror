/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_SUITE( ERCEmptyLabel )


struct EMPTY_LABEL_ERC_FIXTURE
{
    EMPTY_LABEL_ERC_FIXTURE()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->AddTopLevelSheet( m_sheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    void AddWire()
    {
        SCH_LINE* wire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
        wire->SetEndPoint( VECTOR2I( 5000000, 0 ) );
        m_screen->Append( wire );
    }

    int RunCheck()
    {
        ERC_TESTER tester( m_schematic.get() );
        return tester.TestEmptyLabelNames();
    }

    int CountMarkers()
    {
        int count = 0;

        for( SCH_ITEM* item : m_screen->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == ERCE_EMPTY_LABEL_NAME )
                count++;
        }

        return count;
    }

    SETTINGS_MANAGER           m_mgr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SCREEN*                m_screen;
    SCH_SHEET*                 m_sheet;
};


BOOST_FIXTURE_TEST_CASE( EmptyLocalLabelFlagged, EMPTY_LABEL_ERC_FIXTURE )
{
    AddWire();

    // Named and empty label on the same wire, as in the issue repro
    m_screen->Append( new SCH_LABEL( VECTOR2I( 1000000, 0 ), wxT( "PCIe_CLK_N" ) ) );
    m_screen->Append( new SCH_LABEL( VECTOR2I( 3000000, 0 ), wxEmptyString ) );

    BOOST_CHECK_EQUAL( RunCheck(), 1 );
    BOOST_CHECK_EQUAL( CountMarkers(), 1 );
}


BOOST_FIXTURE_TEST_CASE( WhitespaceOnlyLabelsFlagged, EMPTY_LABEL_ERC_FIXTURE )
{
    AddWire();

    m_screen->Append( new SCH_LABEL( VECTOR2I( 1000000, 0 ), wxT( " " ) ) );
    m_screen->Append( new SCH_GLOBALLABEL( VECTOR2I( 3000000, 0 ), wxEmptyString ) );
    m_screen->Append( new SCH_HIERLABEL( VECTOR2I( 4000000, 0 ), wxEmptyString ) );

    BOOST_CHECK_EQUAL( RunCheck(), 3 );
    BOOST_CHECK_EQUAL( CountMarkers(), 3 );
}


BOOST_FIXTURE_TEST_CASE( ValidAndDirectiveLabelsPass, EMPTY_LABEL_ERC_FIXTURE )
{
    AddWire();

    m_screen->Append( new SCH_LABEL( VECTOR2I( 1000000, 0 ), wxT( "PCIe_CLK_N" ) ) );
    m_screen->Append( new SCH_GLOBALLABEL( VECTOR2I( 3000000, 0 ), wxT( "VBUS" ) ) );

    m_screen->Append( new SCH_DIRECTIVE_LABEL( VECTOR2I( 4000000, 0 ) ) );

    BOOST_CHECK_EQUAL( RunCheck(), 0 );
    BOOST_CHECK_EQUAL( CountMarkers(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
