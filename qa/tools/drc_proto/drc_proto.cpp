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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string>

#include <common.h>
#include <core/profile.h>

#include <wx/cmdline.h>

#include <properties/property_mgr.h>

#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew/drc/drc_engine.h>
#include <pcbnew/board.h>
#include <pcbnew/drc/drc_rule_parser.h>
#include <pcbnew/drc/drc_test_provider.h>
#include <pcbnew/pcbexpr_evaluator.h>
#include <board_design_settings.h>

#include <string_utils.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>

#include <reporter.h>
#include <widgets/progress_reporter_base.h>

#include <project.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>

#include <kiway.h>
#include <kiface_ids.h>

#include "drc_proto.h"

PROJECT_CONTEXT loadKicadProject( const wxString& filename, std::optional<wxString> rulesFilePath )
{
   PROJECT_CONTEXT rv;

    auto &manager = Pgm().GetSettingsManager();

    wxFileName pro( filename );
    wxFileName brdName ( filename );
    wxFileName schName ( filename );
    wxFileName ruleFileName ( filename );

    pro.SetExt( FILEEXT::ProjectFileExtension );
    brdName.SetExt( FILEEXT::KiCadPcbFileExtension );
    schName.SetExt( FILEEXT::KiCadSchematicFileExtension );
    ruleFileName.SetExt( FILEEXT::DesignRulesFileExtension );

    brdName.MakeAbsolute();
    schName.MakeAbsolute();
    ruleFileName.MakeAbsolute();
    pro.MakeAbsolute();

    manager.LoadProject( pro.GetFullPath() );

    rv.project = &manager.Prj();
    rv.board.reset( KI_TEST::ReadBoardFromFileOrStream(
                            std::string( brdName.GetFullPath().ToUTF8() ) ).release() );
    rv.board->SetProject( rv.project );

    if( rulesFilePath )
        rv.rulesFilePath = *rulesFilePath;
    else
        rv.rulesFilePath = ruleFileName.GetFullPath();

    if( wxFileExists( schName.GetFullPath() ) )
    {
        //printf("Generating SCH netlist for '%s'\n", (const char*) schName.GetFullPath() );
        //rv.netlist.reset( new NETLIST );
        //generateSchematicNetlist( schName.GetFullPath(), *rv.netlist.get() );
    }

    return rv;
}


int runDRCProto( PROJECT_CONTEXT project, std::shared_ptr<KIGFX::VIEW_OVERLAY> aDebugOverlay )
{
    std::shared_ptr<DRC_ENGINE> drcEngine( new DRC_ENGINE );

    CONSOLE_LOG consoleLog;

    project.board->GetDesignSettings().m_DRCEngine = drcEngine;

    drcEngine->SetBoard( project.board.get() );
    drcEngine->SetDesignSettings( &project.board->GetDesignSettings() );
    drcEngine->SetLogReporter( new CONSOLE_MSG_REPORTER ( &consoleLog ) );
    drcEngine->SetProgressReporter( new CONSOLE_PROGRESS_REPORTER ( &consoleLog ) );

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                // fixme
            } );


    drcEngine->InitEngine( project.rulesFilePath );
    drcEngine->SetDebugOverlay( aDebugOverlay );

    for( auto provider : drcEngine->GetTestProviders() )
    {
        //if( provider->GetName() == "diff_pair_coupling" )
          //  provider->Enable(true);
        //else
          //  provider->Enable(false);
    }

    try
    {
        drcEngine->RunTests( EDA_UNITS::MM, true, false );
    }
    catch( const Clipper2Lib::Clipper2Exception& e )
    {
        consoleLog.Print( wxString::Format( "Clipper exception %s occurred.", e.what() ) );
    }

    return 0;
}
