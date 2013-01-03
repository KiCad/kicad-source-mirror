/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

#include <io_mgr.h>
#include <legacy_plugin.h>
#include <kicad_plugin.h>
#include <eagle_plugin.h>
#include <pcad2kicadpcb_plugin/pcad_plugin.h>
#include <gpcb_plugin.h>
#include <wildcards_and_files_ext.h>

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


// static LEGACY_PLUGIN kicad_plugin;
// static EAGLE_PLUGIN eagle_plugin;

PLUGIN* IO_MGR::PluginFind( PCB_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public IO_MGR API is the only pertinent public information.

    switch( aFileType )
    {
    case LEGACY:
        return new LEGACY_PLUGIN();

    case KICAD:
        return new PCB_IO();

    case EAGLE:
        return new EAGLE_PLUGIN();

    case PCAD:
        return new PCAD_PLUGIN();

    case GEDA_PCB:
        return new GPCB_PLUGIN();
    }

    return NULL;
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
    // keep this function in sync with EnumFromStr() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    switch( aType )
    {
    default:
        return wxString::Format( _( "Unknown PCB_FILE_T value: %d" ), aType );

    case LEGACY:
        return wxString( wxT( "Legacy" ) );

    case KICAD:
        return wxString( wxT( "KiCad" ) );

    case EAGLE:
        return wxString( wxT( "Eagle" ) );

    case PCAD:
        return wxString( wxT( "P-Cad" ) );

    case GEDA_PCB:
        return wxString( wxT( "Geda-PCB" ) );
    }
}


IO_MGR::PCB_FILE_T IO_MGR::EnumFromStr( const wxString& aType )
{
    // keep this function in sync with ShowType() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    if( aType == wxT( "KiCad" ) )
        return KICAD;

    if( aType == wxT( "Legacy" ) )
        return LEGACY;

    if( aType == wxT( "Eagle" ) )
        return EAGLE;

    if( aType == wxT( "P-Cad" ) )
        return PCAD;

    if( aType == wxT( "Geda-PCB" ) )
        return GEDA_PCB;

    // wxASSERT( blow up here )

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
    wxFileName  fn = aLibPath;
    PCB_FILE_T  ret;

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
    else
    {
        // Although KICAD PLUGIN uses libpaths with fixed extension of
        // KiCadFootprintLibPathExtension, we don't make that assumption since
        // a default choice is needed.
        ret = KICAD;
    }

    return ret;
}


BOARD* IO_MGR::Load( PCB_FILE_T aFileType, const wxString& aFileName,
                     BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    // release the PLUGIN even if an exception is thrown.
    PLUGIN::RELEASER pi = PluginFind( aFileType );

    if( (PLUGIN*) pi )  // test pi->plugin
    {
        return pi->Load( aFileName, aAppendToMe, aProperties );  // virtual
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


void IO_MGR::Save( PCB_FILE_T aFileType, const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    // release the PLUGIN even if an exception is thrown.
    PLUGIN::RELEASER pi = PluginFind( aFileType );

    if( (PLUGIN*) pi )  // test pi->plugin
    {
        pi->Save( aFileName, aBoard, aProperties );  // virtual
        return;
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


BOARD* PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


void PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


wxArrayString PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


MODULE* PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                    PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}

