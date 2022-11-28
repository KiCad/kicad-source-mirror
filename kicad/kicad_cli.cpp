/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/app.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>

#include <kiway.h>
#include <macros.h>
#include <paths.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>

#include <stdexcept>

#include "pgm_kicad.h"
#include "kicad_manager_frame.h"

#include <kicad_build_version.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>

#include "cli/command_pcb.h"
#include "cli/command_pcb_export.h"
#include "cli/command_export_pcb_drill.h"
#include "cli/command_export_pcb_dxf.h"
#include "cli/command_export_pcb_gerber.h"
#include "cli/command_export_pcb_pdf.h"
#include "cli/command_export_pcb_pos.h"
#include "cli/command_export_pcb_svg.h"
#include "cli/command_export_pcb_step.h"
#include "cli/command_export_sch_bom.h"
#include "cli/command_export_sch_netlist.h"
#include "cli/command_export_sch_pdf.h"
#include "cli/command_export_sch_svg.h"
#include "cli/command_fp.h"
#include "cli/command_fp_upgrade.h"
#include "cli/command_sch.h"
#include "cli/command_sch_export.h"
#include "cli/exit_codes.h"

// a dummy to quiet linking with EDA_BASE_FRAME::config();
#include <kiface_base.h>
KIFACE_BASE& Kiface()
{
    // This function should never be called.  It is only referenced from
    // EDA_BASE_FRAME::config() and this is only provided to satisfy the linker,
    // not to be actually called.
    wxLogFatalError( wxT( "Unexpected call to Kiface() in kicad/kicad.cpp" ) );

    throw std::logic_error( "Unexpected call to Kiface() in kicad/kicad.cpp" );
}


static PGM_KICAD program;


PGM_BASE& Pgm()
{
    return program;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return &program;
}


PGM_KICAD& PgmTop()
{
    return program;
}

struct COMMAND_ENTRY
{
    CLI::COMMAND* handler;

    std::vector<COMMAND_ENTRY> subCommands;

    COMMAND_ENTRY( CLI::COMMAND* aHandler ) : handler( aHandler ){};
    COMMAND_ENTRY( CLI::COMMAND* aHandler, std::vector<COMMAND_ENTRY> aSub ) :
            handler( aHandler ), subCommands( aSub ){};
};

static CLI::EXPORT_PCB_DRILL_COMMAND   exportPcbDrillCmd{};
static CLI::EXPORT_PCB_DXF_COMMAND     exportPcbDxfCmd{};
static CLI::EXPORT_PCB_STEP_COMMAND    exportPcbStepCmd{};
static CLI::EXPORT_PCB_SVG_COMMAND     exportPcbSvgCmd{};
static CLI::EXPORT_PCB_PDF_COMMAND     exportPcbPdfCmd{};
static CLI::EXPORT_PCB_POS_COMMAND     exportPcbPosCmd{};
static CLI::EXPORT_PCB_GERBER_COMMAND  exportPcbGerberCmd{};
static CLI::EXPORT_PCB_COMMAND         exportPcbCmd{};
static CLI::PCB_COMMAND                pcbCmd{};
static CLI::EXPORT_SCH_COMMAND         exportSchCmd{};
static CLI::SCH_COMMAND                schCmd{};
static CLI::EXPORT_SCH_BOM_COMMAND     exportSchBomCmd{};
static CLI::EXPORT_SCH_NETLIST_COMMAND exportSchNetlistCmd{};
static CLI::EXPORT_SCH_PDF_COMMAND     exportSchPdfCmd{};
static CLI::EXPORT_SCH_SVG_COMMAND     exportSchSvgCmd{};
static CLI::FP_COMMAND                 fpCmd{};
static CLI::FP_UPGRADE_COMMAND         fpUpgradeCmd{};

static std::vector<COMMAND_ENTRY> commandStack = {
    {
        &fpCmd,
        {
            {
                &fpUpgradeCmd
            }
        }
    },
    {
        &pcbCmd,
        {
            { &exportPcbCmd,
                {
                    &exportPcbDrillCmd,
                    &exportPcbDxfCmd,
                    &exportPcbGerberCmd,
                    &exportPcbPdfCmd,
                    &exportPcbPosCmd,
                    &exportPcbStepCmd,
                    &exportPcbSvgCmd
                }
            }
        }
    },
    {
        &schCmd,
        {
            { &exportSchCmd,
                {
                    &exportSchBomCmd,
                    &exportSchNetlistCmd,
                    &exportSchPdfCmd,
                    &exportSchSvgCmd
                }
            }
        }
    },
};


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


bool PGM_KICAD::OnPgmInit()
{
    PGM_BASE::BuildArgvUtf8();
    App().SetAppDisplayName( wxT( "KiCad" ) );

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

    return true;
}


int PGM_KICAD::OnPgmRun()
{
    argparse::ArgumentParser argParser( std::string( "kicad-cli" ), KICAD_MAJOR_MINOR_VERSION );

    for( COMMAND_ENTRY& entry : commandStack )
    {
        recurseArgParserBuild( argParser, entry );
    }

    try
    {
        argParser.parse_args( m_argcUtf8, m_argvUtf8 );
    }
    catch( const std::runtime_error& err )
    {
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
        std::stringstream ss;
        if( cliCmd )
            ss << cliCmd->handler->GetArgParser();
        else
            ss << argParser;

        wxPrintf( FROM_UTF8( ss.str().c_str() ) );

        return CLI::EXIT_CODES::ERR_ARGS;
    }

    COMMAND_ENTRY* cliCmd = nullptr;
    for( COMMAND_ENTRY& entry : commandStack )
    {
        if( argParser.is_subcommand_used( entry.handler->GetName() ) )
        {
            cliCmd = recurseArgParserSubCommandUsed( argParser, entry );
        }
    }

    if( cliCmd )
    {
        int exitCode = cliCmd->handler->Perform( Kiway );

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
        std::stringstream ss;
        ss << argParser;
        wxPrintf( FROM_UTF8( ss.str().c_str() ) );

        return CLI::EXIT_CODES::ERR_ARGS;
    }
}


void PGM_KICAD::OnPgmExit()
{
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

    KICAD_MANAGER_FRAME* frame = (KICAD_MANAGER_FRAME*) App().GetTopWindow();

    if( !aFileName.empty() && wxFileExists( aFileName ) )
        frame->LoadProject( wxFileName( aFileName ) );

#endif
}


void PGM_KICAD::Destroy()
{
    // unlike a normal destructor, this is designed to be called more
    // than once safely:

    m_bm.End();

    PGM_BASE::Destroy();
}


KIWAY Kiway( &Pgm(), KFCTL_CPP_PROJECT_SUITE );


/**
 * Not publicly visible because most of the action is in #PGM_KICAD these days.
 */
struct APP_KICAD_CLI : public wxAppConsole
{
    APP_KICAD_CLI() : wxAppConsole()
    {
        // Init the environment each platform wants
        KIPLATFORM::ENV::Init();
    }


    bool OnInit() override
    {
        // Perform platform-specific init tasks
        if( !KIPLATFORM::APP::Init() )
            return false;

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
        catch( const std::exception& e )
        {
            wxLogError( wxT( "Unhandled exception class: %s  what: %s" ),
                        FROM_UTF8( typeid( e ).name() ), FROM_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch( ... )
        {
            wxLogError( wxT( "Unhandled exception of unknown type" ) );
        }

        return -1;
    }

    int FilterEvent( wxEvent& aEvent ) override
    {
        if( aEvent.GetEventType() == wxEVT_SHOW )
        {
            wxShowEvent& event = static_cast<wxShowEvent&>( aEvent );
            wxDialog*    dialog = dynamic_cast<wxDialog*>( event.GetEventObject() );

            if( dialog && dialog->IsModal() )
                Pgm().m_ModalDialogCount += event.IsShown() ? 1 : -1;
        }

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
        catch( const std::exception& e )
        {
            wxLogError( "Unhandled exception class: %s  what: %s", FROM_UTF8( typeid( e ).name() ),
                        FROM_UTF8( e.what() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( ioe.What() );
        }
        catch( ... )
        {
            wxLogError( "Unhandled exception of unknown type" );
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
