/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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


#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/wxcrtvararg.h>     //for wxPrintf

#include <kiway.h>
#include <libraries/library_manager.h>
#include <string_utils.h>
#include <paths.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>

#include <stdexcept>

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"

#include <build_version.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <locale_io.h>

#include "cli/command_jobset.h"
#include "cli/command_jobset_run.h"
#include "cli/command_pcb.h"
#include "cli/command_pcb_export.h"
#include "cli/command_pcb_drc.h"
#include "cli/command_pcb_render.h"
#include "cli/command_pcb_export_3d.h"
#include "cli/command_pcb_export_drill.h"
#include "cli/command_pcb_export_dxf.h"
#include "cli/command_pcb_export_gerber.h"
#include "cli/command_pcb_export_gerbers.h"
#include "cli/command_pcb_export_hpgl.h"
#include "cli/command_pcb_export_gencad.h"
#include "cli/command_pcb_export_ipc2581.h"
#include "cli/command_pcb_export_ipcd356.h"
#include "cli/command_pcb_export_odb.h"
#include "cli/command_pcb_export_pdf.h"
#include "cli/command_pcb_export_pos.h"
#include "cli/command_pcb_export_ps.h"
#include "cli/command_pcb_export_stats.h"
#include "cli/command_pcb_export_svg.h"
#include "cli/command_sch_export_bom.h"
#include "cli/command_sch_export_pythonbom.h"
#include "cli/command_sch_export_netlist.h"
#include "cli/command_sch_export_plot.h"
#include "cli/command_pcb_upgrade.h"
#include "cli/command_fp.h"
#include "cli/command_fp_export.h"
#include "cli/command_fp_export_svg.h"
#include "cli/command_fp_upgrade.h"
#include "cli/command_sch.h"
#include "cli/command_sch_erc.h"
#include "cli/command_sch_export.h"
#include "cli/command_sch_upgrade.h"
#include "cli/command_sym.h"
#include "cli/command_sym_export.h"
#include "cli/command_sym_export_svg.h"
#include "cli/command_sym_upgrade.h"
#include "cli/command_version.h"
#include "cli/exit_codes.h"

// Add this header after all others, to avoid a collision name in a Windows header
// on mingw.
#include <wx/app.h>

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_base.h>
#include <thread_pool.h>


KIFACE_BASE& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in kicad/kicad.cpp" );
}


struct COMMAND_ENTRY
{
    CLI::COMMAND* handler;

    std::vector<COMMAND_ENTRY> subCommands;

    COMMAND_ENTRY( CLI::COMMAND* aHandler ) : handler( aHandler ){};
    COMMAND_ENTRY( CLI::COMMAND* aHandler, std::vector<COMMAND_ENTRY> aSub ) :
            handler( aHandler ), subCommands( aSub ){};
};

static CLI::JOBSET_COMMAND               jobsetCmd{};
static CLI::JOBSET_RUN_COMMAND           jobsetRunCmd{};
static CLI::PCB_COMMAND                  pcbCmd{};
static CLI::PCB_DRC_COMMAND              pcbDrcCmd{};
static CLI::PCB_RENDER_COMMAND           pcbRenderCmd{};
static CLI::PCB_UPGRADE_COMMAND          pcbUpgradeCmd{};
static CLI::PCB_EXPORT_DRILL_COMMAND     exportPcbDrillCmd{};
static CLI::PCB_EXPORT_DXF_COMMAND       exportPcbDxfCmd{};
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbGlbCmd{ "glb", UTF8STDSTR( _( "Export GLB (binary GLTF)" ) ), JOB_EXPORT_PCB_3D::FORMAT::GLB };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbStepCmd{ "step", UTF8STDSTR( _( "Export STEP" ) ), JOB_EXPORT_PCB_3D::FORMAT::STEP };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbBrepCmd{ "brep", UTF8STDSTR( _( "Export BREP" ) ), JOB_EXPORT_PCB_3D::FORMAT::BREP };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbXaoCmd{ "xao", UTF8STDSTR( _( "Export XAO" ) ), JOB_EXPORT_PCB_3D::FORMAT::XAO };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbVrmlCmd{ "vrml", UTF8STDSTR( _( "Export VRML" ) ), JOB_EXPORT_PCB_3D::FORMAT::VRML };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbPlyCmd{ "ply", UTF8STDSTR( _( "Export PLY" ) ), JOB_EXPORT_PCB_3D::FORMAT::PLY };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbStlCmd{ "stl", UTF8STDSTR( _( "Export STL" ) ), JOB_EXPORT_PCB_3D::FORMAT::STL };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbStepzCmd{ "stpz", UTF8STDSTR( _( "Export STEPZ" ) ), JOB_EXPORT_PCB_3D::FORMAT::STEPZ };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcbU3DCmd{ "u3d", UTF8STDSTR( _( "Export U3D" ) ), JOB_EXPORT_PCB_3D::FORMAT::U3D };
static CLI::PCB_EXPORT_3D_COMMAND        exportPcb3DPDFCmd{ "3dpdf", UTF8STDSTR( _( "Export PDF" ) ),
                                                   JOB_EXPORT_PCB_3D::FORMAT::PDF };
static CLI::PCB_EXPORT_SVG_COMMAND       exportPcbSvgCmd{};
static CLI::PCB_EXPORT_PDF_COMMAND       exportPcbPdfCmd{};
static CLI::PCB_EXPORT_POS_COMMAND       exportPcbPosCmd{};
static CLI::PCB_EXPORT_PS_COMMAND        exportPcbPsCmd{};
static CLI::PCB_EXPORT_STATS_COMMAND     exportPcbStatsCmd{};
static CLI::PCB_EXPORT_GERBER_COMMAND    exportPcbGerberCmd{};
static CLI::PCB_EXPORT_GERBERS_COMMAND   exportPcbGerbersCmd{};
static CLI::PCB_EXPORT_HPGL_COMMAND      exportPcbHpglCmd{};
static CLI::PCB_EXPORT_GENCAD_COMMAND    exportPcbGencadCmd{};
static CLI::PCB_EXPORT_IPC2581_COMMAND   exportPcbIpc2581Cmd{};
static CLI::PCB_EXPORT_IPCD356_COMMAND   exportPcbIpcD356Cmd{};
static CLI::PCB_EXPORT_ODB_COMMAND       exportPcbOdbCmd{};
static CLI::PCB_EXPORT_COMMAND           exportPcbCmd{};
static CLI::SCH_EXPORT_COMMAND           exportSchCmd{};
static CLI::SCH_COMMAND                  schCmd{};
static CLI::SCH_ERC_COMMAND              schErcCmd{};
static CLI::SCH_UPGRADE_COMMAND          schUpgradeCmd{};
static CLI::SCH_EXPORT_BOM_COMMAND       exportSchBomCmd{};
static CLI::SCH_EXPORT_PYTHONBOM_COMMAND exportSchPythonBomCmd{};
static CLI::SCH_EXPORT_NETLIST_COMMAND   exportSchNetlistCmd{};
static CLI::SCH_EXPORT_PLOT_COMMAND      exportSchDxfCmd{ "dxf", UTF8STDSTR( _( "Export DXF" ) ), SCH_PLOT_FORMAT::DXF };
static CLI::SCH_EXPORT_PLOT_COMMAND      exportSchHpglCmd{ "hpgl", UTF8STDSTR( _( "Export HPGL" ) ), SCH_PLOT_FORMAT::HPGL };
static CLI::SCH_EXPORT_PLOT_COMMAND      exportSchPdfCmd{ "pdf", UTF8STDSTR( _( "Export PDF" ) ), SCH_PLOT_FORMAT::PDF, false };
static CLI::SCH_EXPORT_PLOT_COMMAND      exportSchPostscriptCmd{ "ps", UTF8STDSTR( _( "Export PS" ) ), SCH_PLOT_FORMAT::POST };
static CLI::SCH_EXPORT_PLOT_COMMAND      exportSchSvgCmd{ "svg", UTF8STDSTR( _( "Export SVG" ) ), SCH_PLOT_FORMAT::SVG };
static CLI::FP_COMMAND                   fpCmd{};
static CLI::FP_EXPORT_COMMAND            fpExportCmd{};
static CLI::FP_EXPORT_SVG_COMMAND        fpExportSvgCmd{};
static CLI::FP_UPGRADE_COMMAND           fpUpgradeCmd{};
static CLI::SYM_COMMAND                  symCmd{};
static CLI::SYM_EXPORT_COMMAND           symExportCmd{};
static CLI::SYM_EXPORT_SVG_COMMAND       symExportSvgCmd{};
static CLI::SYM_UPGRADE_COMMAND          symUpgradeCmd{};
static CLI::VERSION_COMMAND              versionCmd{};


// clang-format off
static std::vector<COMMAND_ENTRY> commandStack = {
    {
        &jobsetCmd,
        {
            {
                &jobsetRunCmd
            }
        }
    },
    {
        &fpCmd,
        {
            {
                &fpExportCmd,
                {
                    &fpExportSvgCmd
                }
            },
            {
                &fpUpgradeCmd
            }
        }
    },
    {
        &pcbCmd,
        {
            {
                &pcbDrcCmd
            },
            {
                &pcbRenderCmd
            },
            {
                &exportPcbCmd,
                {
                    &exportPcbBrepCmd,
                    &exportPcbDrillCmd,
                    &exportPcbDxfCmd,
                    &exportPcbGerberCmd,
                    &exportPcbGerbersCmd,
                    &exportPcbHpglCmd,
                    &exportPcbGencadCmd,
                    &exportPcbGlbCmd,
                    &exportPcbIpc2581Cmd,
                    &exportPcbIpcD356Cmd,
                    &exportPcbOdbCmd,
                    &exportPcbPdfCmd,
                    &exportPcbPosCmd,
                    &exportPcbPsCmd,
                    &exportPcbStatsCmd,
                    &exportPcbStepCmd,
                    &exportPcbSvgCmd,
                    &exportPcbVrmlCmd,
                    &exportPcbXaoCmd,
                    &exportPcbPlyCmd,
                    &exportPcbStlCmd,
                    &exportPcbStepzCmd,
                    &exportPcbU3DCmd,
                    &exportPcb3DPDFCmd
                }
            },
            {
                &pcbUpgradeCmd
            }
        }
    },
    {
        &schCmd,
        {
            {
                &schErcCmd
            },
            {
                &exportSchCmd,
                {
                    &exportSchDxfCmd,
                    &exportSchHpglCmd,
                    &exportSchNetlistCmd,
                    &exportSchPdfCmd,
                    &exportSchPostscriptCmd,
                    &exportSchBomCmd,
                    &exportSchPythonBomCmd,
                    &exportSchSvgCmd
                }
            },
            {
                &schUpgradeCmd
            }
        }
    },
    {
        &symCmd,
        {
            {
                &symExportCmd,
                {
                    &symExportSvgCmd
                }
            },
            {
                &symUpgradeCmd
            }
        }
    },
    {
            &versionCmd,
    }
};
// clang-format on


static void recurseArgParserBuild( argparse::ArgumentParser& aArgParser, COMMAND_ENTRY& aEntry )
{
    aArgParser.add_subparser( aEntry.handler->GetArgParser() );

    for( COMMAND_ENTRY& subEntry : aEntry.subCommands )
    {
        recurseArgParserBuild( aEntry.handler->GetArgParser(), subEntry );
    }
}


static COMMAND_ENTRY* recurseArgParserSubCommandUsed( argparse::ArgumentParser& aArgParser,
                                                      COMMAND_ENTRY&            aEntry )
{
    COMMAND_ENTRY* cliCmd = nullptr;

    if( aArgParser.is_subcommand_used( aEntry.handler->GetName() ) )
    {
        for( COMMAND_ENTRY& subentry : aEntry.subCommands )
        {
            cliCmd = recurseArgParserSubCommandUsed( aEntry.handler->GetArgParser(), subentry );
            if( cliCmd )
                break;
        }

        if(!cliCmd)
            cliCmd = &aEntry;
    }

    return cliCmd;
}


static void printHelp( argparse::ArgumentParser& argParser )
{
    std::stringstream ss;
    ss << argParser;
    wxPrintf( From_UTF8( ss.str().c_str() ) );
}


bool PGM_KICAD::OnPgmInit()
{
    PGM_BASE::BuildArgvUtf8();
    App().SetAppDisplayName( wxT( "kicad-cli" ) );

#if defined( DEBUG )
    wxString absoluteArgv0 = wxStandardPaths::Get().GetExecutablePath();

    if( !wxIsAbsolutePath( absoluteArgv0 ) )
    {
        wxLogError( wxT( "No meaningful argv[0]" ) );
        return false;
    }
#endif

    if( !InitPgm( true, true) )
        return false;

    m_bm.InitSettings( new KICAD_SETTINGS );
    GetSettingsManager().RegisterSettings( PgmSettings() );
    GetSettingsManager().SetKiway( &Kiway );
    m_bm.Init();

    GetLibraryManager().LoadGlobalTables();

    return true;
}


int PGM_KICAD::OnPgmRun()
{
    argparse::ArgumentParser argParser( std::string( "kicad-cli" ), GetMajorMinorVersion().ToStdString(),
                                        argparse::default_arguments::none );

    argParser.add_argument( "-v", ARG_VERSION )
            .help( UTF8STDSTR( _( "prints version information and exits" ) ) )
            .flag()
            .nargs( 0 );

    argParser.add_argument( ARG_HELP_SHORT, ARG_HELP )
            .help( UTF8STDSTR( ARG_HELP_DESC ) )
            .flag()
            .nargs( 0 );

    for( COMMAND_ENTRY& entry : commandStack )
    {
        recurseArgParserBuild( argParser, entry );
    }

    try
    {
        // Use the C locale to parse arguments
        // Otherwise the decimal separator for the locale will be applied
        LOCALE_IO dummy;
        argParser.parse_args( m_argcUtf8, m_argvUtf8 );
    }
    // std::runtime_error doesn't seem to be enough for the scan<>()
    catch( const std::exception& err )
    {
        bool requestedHelp = false;

        for( int i = 0; i < m_argcUtf8; ++i )
        {
            if( std::string arg( m_argvUtf8[i] ); arg == ARG_HELP_SHORT || arg == ARG_HELP )
            {
                requestedHelp = true;
                break;
            }
        }

        if( !requestedHelp )
            wxPrintf( "%s\n", err.what() );

        // find the correct argparser object to output the command usage info
        COMMAND_ENTRY* cliCmd = nullptr;
        for( COMMAND_ENTRY& entry : commandStack )
        {
            if( argParser.is_subcommand_used( entry.handler->GetName() ) )
            {
                cliCmd = recurseArgParserSubCommandUsed( argParser, entry );
            }
        }

        // arg parser uses a stream overload for printing the help
        // we want to intercept so we can wxString the utf8 contents
        // because on windows our terminal codepage might not be utf8
        if( cliCmd )
            cliCmd->handler->PrintHelp();
        else
        {
            printHelp( argParser );
        }

        return requestedHelp ? 0 : CLI::EXIT_CODES::ERR_ARGS;
    }

    if( argParser[ ARG_HELP ] == true )
    {
        std::stringstream ss;
        ss << argParser;
        wxPrintf( From_UTF8( ss.str().c_str() ) );

        return 0;
    }

    CLI::COMMAND* cliCmd = nullptr;

    // the version arg gets redirected to the version subcommand
    if( argParser[ARG_VERSION] == true )
    {
        cliCmd = &versionCmd;
    }

    if( !cliCmd )
    {
        for( COMMAND_ENTRY& entry : commandStack )
        {
            if( argParser.is_subcommand_used( entry.handler->GetName() ) )
            {
                COMMAND_ENTRY* cmdSubEntry = recurseArgParserSubCommandUsed( argParser, entry );
                if( cmdSubEntry != nullptr )
                {
                    cliCmd = cmdSubEntry->handler;
                    break;
                }
            }
        }
    }

    if( cliCmd )
    {
        int exitCode = cliCmd->Perform( Kiway );

        if( exitCode != CLI::EXIT_CODES::AVOID_CLOSING )
        {
            return exitCode;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        printHelp( argParser );

        return CLI::EXIT_CODES::ERR_ARGS;
    }
}


void PGM_KICAD::OnPgmExit()
{
    // Abort and wait on any background jobs
    GetKiCadThreadPool().purge();
    GetKiCadThreadPool().wait();

    Kiway.OnKiwayEnd();

    if( m_settings_manager && m_settings_manager->IsOK() )
    {
        SaveCommonSettings();
        m_settings_manager->Save();
    }

    // Destroy everything in PGM_KICAD,
    // especially wxSingleInstanceCheckerImpl earlier than wxApp and earlier
    // than static destruction would.
    Destroy();
}


void PGM_KICAD::MacOpenFile( const wxString& aFileName )
{
#if defined( __WXMAC__ )
    wxFAIL_MSG( "kicad-cli should not call MacOpenFile" );
#endif
}


void PGM_KICAD::Destroy()
{
    // unlike a normal destructor, this is designed to be called more
    // than once safely:

    m_bm.End();

    PGM_BASE::Destroy();
}


KIWAY Kiway( KFCTL_CPP_PROJECT_SUITE | KFCTL_CLI );

static PGM_KICAD program;

/**
 * Not publicly visible because most of the action is in #PGM_KICAD these days.
 */
struct APP_KICAD_CLI : public wxAppConsole
{
    APP_KICAD_CLI() : wxAppConsole()
    {
        SetPgm( &program );

        // Init the environment each platform wants
        KIPLATFORM::ENV::Init();
    }


    bool OnInit() override
    {
        // Perform platform-specific init tasks
        if( !KIPLATFORM::APP::Init() )
            return false;

#ifndef DEBUG
        // Enable logging traces to the console in release build.
        // This is usually disabled, but it can be useful for users to run to help
        // debug issues and other problems.
        if( wxGetEnv( wxS( "KICAD_ENABLE_WXTRACE" ), nullptr ) )
        {
            wxLog::EnableLogging( true );
            wxLog::SetLogLevel( wxLOG_Trace );
        }
#endif

        if( !program.OnPgmInit() )
        {
            program.OnPgmExit();
            return false;
        }

        return true;
    }

    int OnExit() override
    {
        program.OnPgmExit();

#if defined( __FreeBSD__ )
        // Avoid wxLog crashing when used in destructors.
        wxLog::EnableLogging( false );
#endif

        return wxAppConsole::OnExit();
    }

    int OnRun() override
    {
        try
        {
            return program.OnPgmRun();
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
        }

        return -1;
    }

    int FilterEvent( wxEvent& aEvent ) override
    {
        return Event_Skip;
    }

#if defined( DEBUG )
    /**
     * Process any unhandled events at the application level.
     */
    bool ProcessEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
        {
            wxKeyEvent* keyEvent = static_cast<wxKeyEvent*>( &aEvent );

            if( keyEvent )
            {
                wxLogTrace( kicadTraceKeyEvent, "APP_KICAD::ProcessEvent %s", dump( *keyEvent ) );
            }
        }

        aEvent.Skip();
        return false;
    }

    /**
     * Override main loop exception handling on debug builds.
     *
     * It can be painfully difficult to debug exceptions that happen in wxUpdateUIEvent
     * handlers.  The override provides a bit more useful information about the exception
     * and a breakpoint can be set to pin point the event where the exception was thrown.
     */
    bool OnExceptionInMainLoop() override
    {
        try
        {
            throw;
        }
        catch( ... )
        {
            Pgm().HandleException( std::current_exception() );
        }

        return false; // continue on. Return false to abort program
    }
#endif
};

IMPLEMENT_APP_CONSOLE( APP_KICAD_CLI )


// The C++ project manager supports one open PROJECT, so Prj() calls within
// this link image need this function.
PROJECT& Prj()
{
    return Kiway.Prj();
}
