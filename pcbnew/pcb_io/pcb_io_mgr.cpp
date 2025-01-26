/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <wx/uri.h>

#include <config.h>
#include <kiway_player.h>
#include <wildcards_and_files_ext.h>
#include <pcb_io/pcb_io_mgr.h>

#include <pcb_io/eagle/pcb_io_eagle.h>
#include <pcb_io/geda/pcb_io_geda.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_legacy/pcb_io_kicad_legacy.h>
#include <pcb_io/pcad/pcb_io_pcad.h>
#include <pcb_io/altium/pcb_io_altium_circuit_maker.h>
#include <pcb_io/altium/pcb_io_altium_circuit_studio.h>
#include <pcb_io/altium/pcb_io_altium_designer.h>
#include <pcb_io/altium/pcb_io_solidworks.h>
#include <pcb_io/cadstar/pcb_io_cadstar_archive.h>
#include <pcb_io/fabmaster/pcb_io_fabmaster.h>
#include <pcb_io/easyeda/pcb_io_easyeda_plugin.h>
#include <pcb_io/easyedapro/pcb_io_easyedapro.h>
#include <pcb_io/ipc2581/pcb_io_ipc2581.h>
#include <pcb_io/odbpp/pcb_io_odbpp.h>
#include <reporter.h>



#define FMT_UNIMPLEMENTED   _( "Plugin '%s' does not implement the '%s' function." )
#define FMT_NOTFOUND        _( "Plugin type '%s' is not found." )


// Some day plugins might be in separate DLL/DSOs, simply because of numbers of them
// and code size.  Until then, use the simplest method:

// This implementation is one of two which could be done.
// The other one would cater to DLL/DSO's.  But since it would be nearly
// impossible to link a KICAD type DLL/DSO right now without pulling in all
// ::Draw() functions, I forgo that option temporarily.

// Some day it may be possible to have some built in AND some DLL/DSO
// plugins coexisting.


PCB_IO* PCB_IO_MGR::PluginFind( PCB_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public IO_MGR API is the only pertinent public information.

    return PLUGIN_REGISTRY::Instance()->Create( aFileType );
}


const wxString PCB_IO_MGR::ShowType( PCB_FILE_T aType )
{
    const auto& plugins = PLUGIN_REGISTRY::Instance()->AllPlugins();

    for( const auto& plugin : plugins )
    {
        if ( plugin.m_type == aType )
        {
            return plugin.m_name;
        }
    }

    return wxString::Format( _( "UNKNOWN (%d)" ), aType );
}


PCB_IO_MGR::PCB_FILE_T PCB_IO_MGR::EnumFromStr( const wxString& aType )
{
    const auto& plugins = PLUGIN_REGISTRY::Instance()->AllPlugins();

    for( const auto& plugin : plugins )
    {
        if ( plugin.m_name == aType )
        {
            return plugin.m_type;
        }
    }

    return PCB_FILE_T( -1 );
}


// The KIWAY_PLAYER::OpenProjectFiles() API knows nothing about plugins, so
// determine how to load the BOARD here
PCB_IO_MGR::PCB_FILE_T PCB_IO_MGR::FindPluginTypeFromBoardPath( const wxString& aFileName, int aCtl )
{
    const auto& plugins = PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins();

    for( const auto& plugin : plugins )
    {
        bool isKiCad = plugin.m_type == PCB_IO_MGR::KICAD_SEXP || plugin.m_type == PCB_IO_MGR::LEGACY;

        if( ( aCtl & KICTL_KICAD_ONLY ) && !isKiCad )
            continue;

        if( ( aCtl & KICTL_NONKICAD_ONLY ) && isKiCad )
            continue;

        IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );

        if( pi->CanReadBoard( aFileName ) )
            return plugin.m_type;
    }

    return PCB_IO_MGR::FILE_TYPE_NONE;
}


PCB_IO_MGR::PCB_FILE_T PCB_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl )
{
    const auto& plugins = PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins();

    for( const auto& plugin : plugins )
    {
        bool isKiCad = plugin.m_type == PCB_IO_MGR::KICAD_SEXP || plugin.m_type == PCB_IO_MGR::LEGACY;

        if( ( aCtl & KICTL_KICAD_ONLY ) && !isKiCad )
            continue;

        if( ( aCtl & KICTL_NONKICAD_ONLY ) && isKiCad )
            continue;

        IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );

        if( pi->CanReadLibrary( aLibPath ) )
            return plugin.m_type;
    }

    return PCB_IO_MGR::FILE_TYPE_NONE;
}


BOARD* PCB_IO_MGR::Load( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aAppendToMe,
                     const std::map<std::string, UTF8>* aProperties, PROJECT* aProject,
                     PROGRESS_REPORTER* aProgressReporter )
{
    IO_RELEASER<PCB_IO> pi( PluginFind( aFileType ) );

    if( pi )  // test pi->plugin
    {
        pi->SetProgressReporter( aProgressReporter );
        return pi->LoadBoard( aFileName, aAppendToMe, aProperties, aProject );
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


void PCB_IO_MGR::Save( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aBoard,
                   const std::map<std::string, UTF8>* aProperties )
{
    IO_RELEASER<PCB_IO> pi( PluginFind( aFileType ) );

    if( pi )
    {
        pi->SaveBoard( aFileName, aBoard, aProperties );  // virtual
        return;
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


bool PCB_IO_MGR::ConvertLibrary( const std::map<std::string, UTF8>& aOldFileProps,
                                 const wxString& aOldFilePath, const wxString& aNewFilePath,
                                 REPORTER* aReporter )
{
    PCB_IO_MGR::PCB_FILE_T oldFileType = PCB_IO_MGR::GuessPluginTypeFromLibPath( aOldFilePath );

    if( oldFileType == PCB_IO_MGR::FILE_TYPE_NONE )
        return false;

    IO_RELEASER<PCB_IO> oldFilePI( PCB_IO_MGR::PluginFind( oldFileType ) );
    IO_RELEASER<PCB_IO> kicadPI( PCB_IO_MGR::PluginFind( PCB_IO_MGR::KICAD_SEXP ) );
    wxArrayString fpNames;
    wxFileName newFileName( aNewFilePath );

    if( newFileName.HasExt() )
    {
        wxString extraDir = newFileName.GetFullName();
        newFileName.ClearExt();
        newFileName.SetName( "" );
        newFileName.AppendDir( extraDir );
    }

    if( !newFileName.DirExists() && !wxFileName::Mkdir( aNewFilePath, wxS_DIR_DEFAULT ) )
        return false;

    try
    {
        bool bestEfforts = false; // throw on first error
        oldFilePI->FootprintEnumerate( fpNames, aOldFilePath, bestEfforts, &aOldFileProps );
        std::map<std::string, UTF8> props { { "skip_cache_validation", "" } };

        for ( const wxString& fpName : fpNames )
        {
            std::unique_ptr<const FOOTPRINT> fp( oldFilePI->GetEnumeratedFootprint( aOldFilePath, fpName,
                                                                                    &aOldFileProps ) );

            try
            {
                kicadPI->FootprintSave( aNewFilePath, fp.get(), &props );
            }
            catch( ... )
            {
                // Footprints that cannot be saved are just skipped. This is not see
                // as a fatal error.
                // this can be just a illegal filename used for the footprint
                if( aReporter )
                    aReporter->Report( wxString::Format( "Footprint \"%s\" can't be saved. Skipped",
                                                         fpName ),
                                       SEVERITY::RPT_SEVERITY_WARNING );
            }
        }
    }
    catch( IO_ERROR& io_err )
    {
        if( aReporter )
            aReporter->Report( wxString::Format( "Library '%s' Convert err: \"%s\"",
                                             aOldFilePath, io_err.What() ),
                                SEVERITY::RPT_SEVERITY_ERROR );
        return false;
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


// These text strings are "truth" for identifying the plugins.  If you change the spellings,
// you will obsolete library tables, so don't do it.  Additions are OK.

// clang-format off
static PCB_IO_MGR::REGISTER_PLUGIN registerKicadPlugin(
        PCB_IO_MGR::KICAD_SEXP,
        wxT( "KiCad" ),
        []() -> PCB_IO* { return new PCB_IO_KICAD_SEXPR; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerLegacyPlugin(
        PCB_IO_MGR::LEGACY,
        wxT( "Legacy" ),
        []() -> PCB_IO* { return new PCB_IO_KICAD_LEGACY; } );

// Keep non-KiCad plugins in alphabetical order

static PCB_IO_MGR::REGISTER_PLUGIN registerAltiumCircuitMakerPlugin(
        PCB_IO_MGR::ALTIUM_CIRCUIT_MAKER,
        wxT( "Altium Circuit Maker" ),
        []() -> PCB_IO* { return new PCB_IO_ALTIUM_CIRCUIT_MAKER; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerAltiumCircuitStudioPlugin(
        PCB_IO_MGR::ALTIUM_CIRCUIT_STUDIO,
        wxT( "Altium Circuit Studio" ),
        []() -> PCB_IO* { return new PCB_IO_ALTIUM_CIRCUIT_STUDIO; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerAltiumDesignerPlugin(
        PCB_IO_MGR::ALTIUM_DESIGNER,
        wxT( "Altium Designer" ),
        []() -> PCB_IO* { return new PCB_IO_ALTIUM_DESIGNER; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerCadstarArchivePlugin(
        PCB_IO_MGR::CADSTAR_PCB_ARCHIVE,
        wxT( "CADSTAR PCB Archive" ),
        []() -> PCB_IO* { return new PCB_IO_CADSTAR_ARCHIVE; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerEaglePlugin(
        PCB_IO_MGR::EAGLE,
        wxT( "Eagle" ),
        []() -> PCB_IO* { return new PCB_IO_EAGLE; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerEasyEDAPlugin(
        PCB_IO_MGR::EASYEDA,
        wxT( "EasyEDA / JLCEDA Std" ),
	    []() -> PCB_IO* { return new PCB_IO_EASYEDA; });

static PCB_IO_MGR::REGISTER_PLUGIN registerEasyEDAProPlugin(
        PCB_IO_MGR::EASYEDAPRO,
        wxT( "EasyEDA / JLCEDA Pro" ),
        []() -> PCB_IO* { return new PCB_IO_EASYEDAPRO; });

static PCB_IO_MGR::REGISTER_PLUGIN registerFabmasterPlugin(
        PCB_IO_MGR::FABMASTER,
        wxT( "Fabmaster" ),
        []() -> PCB_IO* { return new PCB_IO_FABMASTER; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerGPCBPlugin(
        PCB_IO_MGR::GEDA_PCB,
        wxT( "GEDA/Pcb" ),
        []() -> PCB_IO* { return new PCB_IO_GEDA; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerPcadPlugin(
        PCB_IO_MGR::PCAD,
        wxT( "P-Cad" ),
        []() -> PCB_IO* { return new PCB_IO_PCAD; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerSolidworksPCBPlugin(
        PCB_IO_MGR::SOLIDWORKS_PCB,
        wxT( "Solidworks PCB" ),
        []() -> PCB_IO* { return new PCB_IO_SOLIDWORKS; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerIPC2581Plugin(
        PCB_IO_MGR::IPC2581,
        wxT( "IPC-2581" ),
        []() -> PCB_IO* { return new PCB_IO_IPC2581; } );

static PCB_IO_MGR::REGISTER_PLUGIN registerODBPPPlugin(
        PCB_IO_MGR::ODBPP,
        wxT( "ODB++" ),
        []() -> PCB_IO* { return new PCB_IO_ODBPP; } );
// clang-format on
