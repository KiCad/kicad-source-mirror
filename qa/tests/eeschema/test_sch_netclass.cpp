/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

class TEST_SCH_NETCLASS_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{};

BOOST_FIXTURE_TEST_SUITE( SchNetclass, TEST_SCH_NETCLASS_FIXTURE )


BOOST_AUTO_TEST_CASE( TestSubsheetNetclass )
{
    LoadSchematic( "issue14494" );

    SCH_SHEET_LIST sheets = m_schematic.GetSheets();
    SCH_SHEET_PATH path = sheets.at( 1 );
    SCH_SCREEN *screen = path.GetSheet( 1 )->GetScreen();

    for( SCH_ITEM* item : screen->Items().OfType( SCH_HIER_LABEL_T ) )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

        wxString name = label->GetText();

        if( name == wxT( "B" ) || name == wxT( "D" ) )
            BOOST_CHECK_EQUAL( label->GetEffectiveNetClass( &path )->GetName(), wxT( "net_02" ) );
        else
            BOOST_CHECK_EQUAL( label->GetEffectiveNetClass( &path )->GetName(), wxT( "net_01" ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
