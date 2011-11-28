/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <io_mgr.h>
#include <kicad_plugin.h>


// some day plugins could be in separate DLL/DSOs, until then, use the simplest method:

// This implementation is one of two which could be done.
// the other one would cater to DLL/DSO's.  But since it would be nearly
// impossible to link a KICAD type DLL/DSO right now without pulling in all
// ::Draw() functions, I forgo that option.

// Some day it may be possible to have some built in AND some DLL/DSO, but
// only when we can keep things clean enough to link a DLL/DSO without
// pulling in the world.

static KICAD_PLUGIN kicad_plugin;
//static EAGLE_PLUGIN eagle_plugin;

PLUGIN* IO_MGR::PluginFind( PCB_FILE_T aFileType )
{
    switch( aFileType )
    {
    case KICAD:     return &kicad_plugin;

//    case EAGLE:     return &eagle_plugin;
    }

    return NULL;
}


void IO_MGR::PluginRelease( PLUGIN* aPlugin )
{
    // This function is a place holder for a future point in time where
    // the plugin is a DLL/DSO.  It could do reference counting, and then
    // unload the DLL/DSO when count goes to zero.
}


const wxString& IO_MGR::ShowType( PCB_FILE_T aFileType )
{
    static const wxString kicad   = wxT( "KiCad" );
    static const wxString unknown = _( "Unknown" );

    switch( aFileType )
    {
    case KICAD:
        return kicad;
    default:
        return unknown;     // could Printf() the numeric value of aFileType
    }
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

    wxString msg;

    msg.Printf( _( "Plugin type '%s' is not found.\n" ), ShowType( aFileType ).GetData() );

    THROW_IO_ERROR( msg );
}


BOARD* PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface,
    // e.g. Load() or Save() but not both.

    wxString msg;

    msg.Printf( _( "Plugin %s does not implement the BOARD Load() function.\n" ),
            Name().GetData() );

    THROW_IO_ERROR( msg );
}


void PLUGIN::Save( const wxString* aFileName, BOARD* aBoard, PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface,
    // e.g. Load() or Save() but not both.

    wxString msg;

    msg.Printf( _( "Plugin %s does not implement the BOARD Save() function.\n" ),
            Name().GetData() );

    THROW_IO_ERROR( msg );
}

