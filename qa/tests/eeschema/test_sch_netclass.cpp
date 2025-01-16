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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <sch_sheet_path.h>
#include <sch_label.h>
#include <sch_io/sch_io.h>
#include <project/net_settings.h>
#include <project/project_file.h>

class TEST_SCH_NETCLASS_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{};

BOOST_FIXTURE_TEST_SUITE( SchNetclass, TEST_SCH_NETCLASS_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSubsheetNetclass )
{
    LoadSchematic( SchematicQAPath( "issue14494" ) );

    SCH_SHEET_PATH path = m_schematic->BuildSheetListSortedByPageNumbers().at( 1 );
    SCH_SCREEN*    screen = path.GetSheet( 1 )->GetScreen();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

        wxString name = label->GetText();

        if( name == wxT( "B" ) || name == wxT( "D" ) )
            BOOST_CHECK_EQUAL( label->GetEffectiveNetClass( &path )->GetName(), "net_02" );
        else
            BOOST_CHECK_EQUAL( label->GetEffectiveNetClass( &path )->GetName(), "net_01" );
    }
}

BOOST_AUTO_TEST_CASE( TestMultiNetclasses )
{
    LoadSchematic( SchematicQAPath( "multinetclasses" ) );

    std::shared_ptr<NET_SETTINGS>& netSettings = m_schematic->Project().GetProjectFile().m_NetSettings;

    std::shared_ptr<NETCLASS> nc = netSettings->GetEffectiveNetClass( "/BUS.SIGNAL" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS2,CLASS1,Default" );

    nc = netSettings->GetEffectiveNetClass( "/BUS.A0" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS1,CLASS3,Default" );

    nc = netSettings->GetEffectiveNetClass( "/BUS.A1" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS1,Default" );

    nc = netSettings->GetEffectiveNetClass( "/BUS.A2" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS1,CLASS4,Default" );

    nc = netSettings->GetEffectiveNetClass( "/NET_1" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS2,CLASS3,Default" );

    nc = netSettings->GetEffectiveNetClass( "/NET_2" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS_COMPLETE" );

    nc = netSettings->GetEffectiveNetClass( "/NET_3" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS_COMPLETE,CLASS3,CLASS4" );

    nc = netSettings->GetEffectiveNetClass( "/NET_4" );
    BOOST_CHECK_EQUAL( nc->GetName(), "CLASS_COMPLETE,CLASS3,CLASS4" );
}

BOOST_AUTO_TEST_SUITE_END()
