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
#include <schematic_utils/schematic_file_util.h>

#include <locale_io.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <settings/settings_manager.h>


struct ISSUE_25508_FIXTURE
{
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// A global label embedding ${INTERSHEET_REFS} in its own text resolves through itself; the
// expansion has no fixed point, so all it can do is stop on the unknown-page placeholder
BOOST_FIXTURE_TEST_CASE( Issue25508, ISSUE_25508_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue25508/issue25508", m_schematic );

    SCH_SHEET_PATH   sheet = m_schematic->BuildSheetListSortedByPageNumbers().at( 0 );
    SCH_GLOBALLABEL* label = nullptr;

    for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        label = static_cast<SCH_GLOBALLABEL*>( item );

    BOOST_REQUIRE( label );

    SCHEMATIC_SETTINGS& settings = m_schematic->Settings();
    wxString            expected = wxString::Format( wxS( "foo (%s?%s)" ),
                                                     settings.m_IntersheetRefsPrefix,
                                                     settings.m_IntersheetRefsSuffix );

    BOOST_CHECK_EQUAL( label->GetShownText( &sheet, false ), expected );
}
