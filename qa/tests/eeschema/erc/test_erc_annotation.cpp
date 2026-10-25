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
#include "eeschema_test_utils.h"

#include <erc/erc.h>
#include <erc/erc_item.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <wildcards_and_files_ext.h>


BOOST_FIXTURE_TEST_SUITE( ERCAnnotation, KI_TEST::SCHEMATIC_TEST_FIXTURE )


// kicad-cli runs ERC without the schematic editor, which used to be the only place annotation was checked
BOOST_AUTO_TEST_CASE( HeadlessRunFlagsDuplicateReferences )
{
    LoadSchematic( wxFileName( KI_TEST::GetEeschemaTestDataDir(), wxS( "issue13212" ),
                               FILEEXT::KiCadSchematicFileExtension ) );

    SCH_SHEET_LIST     hierarchy = m_schematic->Hierarchy();
    SCH_REFERENCE_LIST refs;

    hierarchy.GetSymbols( refs, SYMBOL_FILTER_NON_POWER );
    BOOST_REQUIRE_GE( refs.GetCount(), 2 );

    refs[1].GetSymbol()->SetRef( &refs[1].GetSheetPath(), refs[0].GetSymbol()->GetRef( &refs[0].GetSheetPath() ) );
    ERC_TESTER( m_schematic.get() ).RunTests( nullptr, nullptr, nullptr, &m_schematic->Project(), nullptr );

    int duplicates = 0;

    for( const SCH_SHEET_PATH& path : hierarchy )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            std::shared_ptr<RC_ITEM> rcItem = static_cast<SCH_MARKER*>( item )->GetRCItem();

            if( rcItem->GetErrorCode() != ERCE_DUPLICATE_REFERENCE )
                continue;

            // Exclusions saved from the editor dialog have no sheet path, so they must keep matching
            BOOST_CHECK( !std::static_pointer_cast<ERC_ITEM>( rcItem )->IsSheetSpecific() );
            duplicates++;
        }
    }

    BOOST_CHECK_GE( duplicates, 1 );
}


BOOST_AUTO_TEST_SUITE_END()
