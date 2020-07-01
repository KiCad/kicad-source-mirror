/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plugins/eagle/eagle_plugin.h>
#include <plugins/geda/gpcb_plugin.h>
#include <io_mgr.h>
#include <plugins/kicad/kicad_plugin.h>
#include <plugins/legacy/legacy_plugin.h>
#include <plugins/pcad/pcad_plugin.h>
#include <plugins/altium/altium_circuit_maker_plugin.h>
#include <plugins/altium/altium_circuit_studio_plugin.h>
#include <plugins/altium/altium_designer_plugin.h>
#include <plugins/cadstar/cadstar_pcb_archive_plugin.h>
#include <plugins/fabmaster/fabmaster_plugin.h>
#include <wildcards_and_files_ext.h>

#define FMT_UNIMPLEMENTED   _( "Plugin \"%s\" does not implement the \"%s\" function." )
#define FMT_NOTFOUND        _( "Plugin type \"%s\" is not found." )


// Some day plugins might be in separate DLL/DSOs, simply because of numbers of them
// and code size.  Until then, use the simplest method:

// This implementation is one of two which could be done.
// The other one would cater to DLL/DSO's.  But since it would be nearly
// impossible to link a KICAD type DLL/DSO right now without pulling in all
// ::Draw() functions, I forgo that option temporarily.

// Some day it may be possible to have some built in AND some DLL/DSO
// plugins coexisting.


PLUGIN* IO_MGR::PluginFind( PCB_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public IO_MGR API is the only pertinent public information.

    return PLUGIN_REGISTRY::Instance()->Create( aFileType );
}


void IO_MGR::PluginRelease( PLUGIN* aPlugin )
{
    // This function is a place holder for a future point in time where
    // the plugin is a DLL/DSO.  It could do reference counting, and then
    // unload the DLL/DSO when count goes to zero.

    delete aPlugin;
}


const wxString IO_MGR::ShowType( PCB_FILE_T aType )
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


IO_MGR::PCB_FILE_T IO_MGR::EnumFromStr( const wxString& aType )
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


const wxString IO_MGR::GetFileExtension( PCB_FILE_T aFileType )
{
    wxString ext = wxEmptyString;
    PLUGIN* plugin = PluginFind( aFileType );

    if( plugin != NULL )
    {
        ext = plugin->GetFileExtension();
        PluginRelease( plugin );
    }

    return ext;
}


IO_MGR::PCB_FILE_T IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath )
{
    PCB_FILE_T  ret = KICAD_SEXP;        // default guess, unless detected otherwise.
    wxFileName  fn( aLibPath );

    if( fn.GetExt() == LegacyFootprintLibPathExtension )
    {
        ret = LEGACY;
    }
    else if( fn.GetExt() == GedaPcbFootprintLibFileExtension )
    {
        ret = GEDA_PCB;
    }
    else if( fn.GetExt() == EagleFootprintLibPathExtension )
    {
        ret = EAGLE;
    }

    // Test this one anyways, even though it's the default guess, to avoid
    // the wxURI instantiation below.
    // We default ret to KICAD above, because somebody might have
    // mistakenly put a pretty library into a directory other than
    // *.pretty/ with *.kicad_mod in there., and I don't want to return -1,
    // since we only claimed to be guessing.
    //
    else if( fn.GetExt() == KiCadFootprintLibPathExtension )
    {
        ret = KICAD_SEXP;
    }

    return ret;
}


BOARD* IO_MGR::Load( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aAppendToMe,
                     const PROPERTIES* aProperties, PROJECT* aProject )
{
    // release the PLUGIN even if an exception is thrown.
    PLUGIN::RELEASER pi( PluginFind( aFileType ) );

    if( (PLUGIN*) pi )  // test pi->plugin
    {
        return pi->Load( aFileName, aAppendToMe, aProperties, aProject );  // virtual
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


void IO_MGR::Save( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aBoard,
                   const PROPERTIES* aProperties )
{
    // release the PLUGIN even if an exception is thrown.
    PLUGIN::RELEASER pi( PluginFind( aFileType ) );

    if( (PLUGIN*) pi )  // test pi->plugin
    {
        pi->Save( aFileName, aBoard, aProperties );  // virtual
        return;
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}

// These text strings are "truth" for identifying the plugins.  If you change the spellings,
// you will obsolete library tables, so don't do it.  Additions are OK.
static IO_MGR::REGISTER_PLUGIN registerEaglePlugin( IO_MGR::EAGLE, wxT("Eagle"),
                                                    []() -> PLUGIN* { return new EAGLE_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerKicadPlugin( IO_MGR::KICAD_SEXP,
                                                    wxT("KiCad"), []() -> PLUGIN* { return new PCB_IO; } );
static IO_MGR::REGISTER_PLUGIN registerPcadPlugin( IO_MGR::PCAD, wxT("P-Cad"),
                                                   []() -> PLUGIN* { return new PCAD_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerFabmasterPlugin( IO_MGR::FABMASTER, wxT( "Fabmaster" ),
                                                   []() -> PLUGIN* { return new FABMASTER_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerAltiumDesignerPlugin( IO_MGR::ALTIUM_DESIGNER,
        wxT( "Altium Designer" ), []() -> PLUGIN* { return new ALTIUM_DESIGNER_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerAltiumCircuitStudioPlugin( IO_MGR::ALTIUM_CIRCUIT_STUDIO,
        wxT( "Altium Circuit Studio" ),
        []() -> PLUGIN* { return new ALTIUM_CIRCUIT_STUDIO_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerAltiumCircuitMakerPlugin( IO_MGR::ALTIUM_CIRCUIT_MAKER,
        wxT( "Altium Circuit Maker" ),
        []() -> PLUGIN* { return new ALTIUM_CIRCUIT_MAKER_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerCadstarArchivePlugin( IO_MGR::CADSTAR_PCB_ARCHIVE,
        wxT( "CADSTAR PCB Archive" ), []() -> PLUGIN* { return new CADSTAR_PCB_ARCHIVE_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerLegacyPlugin( IO_MGR::LEGACY, wxT("Legacy"),
                                                     []() -> PLUGIN* { return new LEGACY_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerGPCBPlugin( IO_MGR::GEDA_PCB, wxT("GEDA/Pcb"),
                                                   []() -> PLUGIN* { return new GPCB_PLUGIN; } );
