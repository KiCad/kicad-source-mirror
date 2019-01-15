/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <io_mgr.h>
#include <legacy_plugin.h>
#include <kicad_plugin.h>
#include <eagle_plugin.h>
#include <pcad2kicadpcb_plugin/pcad_plugin.h>
#include <gpcb_plugin.h>
#include <config.h>

#if defined(BUILD_GITHUB_PLUGIN)
 #include <github/github_plugin.h>
#endif

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
    // However libraries on GitHub have names ending by .pretty
    // so test also this is not a name starting by http (including https).
    else if( fn.GetExt() == KiCadFootprintLibPathExtension &&
             !aLibPath.StartsWith( wxT( "http" ) ) )
    {
        ret = KICAD_SEXP;
    }
    else
    {
#if defined(BUILD_GITHUB_PLUGIN)
        // There is no extension for a remote repo, so test the server name.
        wxURI   uri( aLibPath );

        if( uri.HasServer() && uri.GetServer() == wxT( "github.com" ) )
        {
            ret = GITHUB;
        }
#endif
    }

    return ret;
}


BOARD* IO_MGR::Load( PCB_FILE_T aFileType, const wxString& aFileName,
                     BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    // release the PLUGIN even if an exception is thrown.
    PLUGIN::RELEASER pi( PluginFind( aFileType ) );

    if( (PLUGIN*) pi )  // test pi->plugin
    {
        return pi->Load( aFileName, aAppendToMe, aProperties );  // virtual
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


void IO_MGR::Save( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties )
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
static IO_MGR::REGISTER_PLUGIN registerEaglePlugin( IO_MGR::EAGLE, wxT("Eagle"), []() -> PLUGIN* { return new EAGLE_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerKicadPlugin( IO_MGR::KICAD_SEXP, wxT("KiCad"), []() -> PLUGIN* { return new PCB_IO; } );
static IO_MGR::REGISTER_PLUGIN registerPcadPlugin( IO_MGR::PCAD, wxT("P-Cad"), []() -> PLUGIN* { return new PCAD_PLUGIN; } );
#ifdef BUILD_GITHUB_PLUGIN
static IO_MGR::REGISTER_PLUGIN registerGithubPlugin( IO_MGR::GITHUB, wxT("Github"), []() -> PLUGIN* { return new GITHUB_PLUGIN; } );
#endif /* BUILD_GITHUB_PLUGIN */
static IO_MGR::REGISTER_PLUGIN registerLegacyPlugin( IO_MGR::LEGACY, wxT("Legacy"), []() -> PLUGIN* { return new LEGACY_PLUGIN; } );
static IO_MGR::REGISTER_PLUGIN registerGPCBPlugin( IO_MGR::GEDA_PCB, wxT("GEDA/Pcb"), []() -> PLUGIN* { return new GPCB_PLUGIN; } );
