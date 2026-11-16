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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <ki_exception.h>
#include <locale_io.h>
#include <schematic.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <settings/settings_manager.h>
#include <widgets/progress_reporter_base.h>

#include <functional>
#include <memory>


namespace
{

// Cancels once a second file starts to load, which is the first subsheet
class CANCEL_AT_SUBSHEET : public PROGRESS_REPORTER_BASE
{
public:
    CANCEL_AT_SUBSHEET() :
            PROGRESS_REPORTER_BASE( 1 )
    {
    }

    void Report( const wxString& aMessage ) override
    {
        PROGRESS_REPORTER_BASE::Report( aMessage );
        m_files++;
    }

protected:
    bool updateUI() override { return m_files < 2; }

private:
    int m_files = 0;
};

} // namespace


BOOST_AUTO_TEST_SUITE( SchIoCancel )


// A cancel in a subsheet must not be kept as that sheet's error while the rest loads
BOOST_AUTO_TEST_CASE( CancelInASubsheetStopsTheWholeLoad )
{
    const std::vector<std::pair<std::string, std::function<SCH_IO*()>>> loaders = {
        { "netlists/hierarchy_aliases/hierarchy_aliases.kicad_sch",
          []() -> SCH_IO*
          {
              return new SCH_IO_KICAD_SEXPR();
          } },
        { "netlists/complex_hierarchy/complex_hierarchy.sch",
          []() -> SCH_IO*
          {
              return new SCH_IO_KICAD_LEGACY();
          } }
    };

    for( const auto& [file, make] : loaders )
    {
        BOOST_TEST_CONTEXT( file )
        {
            LOCALE_IO               locale;
            SETTINGS_MANAGER        settings;
            SCHEMATIC               schematic( nullptr );
            std::unique_ptr<SCH_IO> io( make() );
            CANCEL_AT_SUBSHEET      reporter;

            settings.LoadProject( "" );
            schematic.SetProject( &settings.Prj() );
            schematic.Reset();
            io->SetProgressReporter( &reporter );

            BOOST_CHECK_THROW( io->LoadSchematicFile( KI_TEST::GetEeschemaTestDataDir() + file, &schematic ),
                               IO_CANCELLED );

            schematic.Reset();
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
