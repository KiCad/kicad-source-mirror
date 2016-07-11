/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016 KiCad Developers, see change_log.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/filename.h>
#include <wx/uri.h>

#include <sch_io_mgr.h>
#include <sch_legacy_plugin.h>

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


SCH_PLUGIN* SCH_IO_MGR::FindPlugin( SCH_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public SCH_IO_MGR API is the only pertinent public information.

    switch( aFileType )
    {
    case SCH_LEGACY:
        return new SCH_LEGACY_PLUGIN();
    case SCH_KICAD:
        return NULL;
    }

    return NULL;
}


void SCH_IO_MGR::ReleasePlugin( SCH_PLUGIN* aPlugin )
{
    // This function is a place holder for a future point in time where
    // the plugin is a DLL/DSO.  It could do reference counting, and then
    // unload the DLL/DSO when count goes to zero.

    delete aPlugin;
}


const wxString SCH_IO_MGR::ShowType( SCH_FILE_T aType )
{
    // keep this function in sync with EnumFromStr() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    switch( aType )
    {
    default:
        return wxString::Format( _( "Unknown SCH_FILE_T value: %d" ), aType );

    case SCH_LEGACY:
        return wxString( wxT( "Legacy" ) );
    }
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::EnumFromStr( const wxString& aType )
{
    // keep this function in sync with ShowType() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    if( aType == wxT( "Legacy" ) )
        return SCH_LEGACY;

    // wxASSERT( blow up here )

    return SCH_FILE_T( -1 );
}


const wxString SCH_IO_MGR::GetFileExtension( SCH_FILE_T aFileType )
{
    wxString ext = wxEmptyString;
    SCH_PLUGIN* plugin = FindPlugin( aFileType );

    if( plugin != NULL )
    {
        ext = plugin->GetFileExtension();
        ReleasePlugin( plugin );
    }

    return ext;
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath )
{
    SCH_FILE_T  ret = SCH_LEGACY;        // default guess, unless detected otherwise.
    wxFileName  fn( aLibPath );

    if( fn.GetExt() == SchematicFileWildcard )
    {
        ret = SCH_LEGACY;
    }

    return ret;
}


SCH_SHEET* SCH_IO_MGR::Load( SCH_FILE_T aFileType, const wxString& aFileName, KIWAY* aKiway,
                             SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    // release the SCH_PLUGIN even if an exception is thrown.
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( FindPlugin( aFileType ) );

    if( (SCH_PLUGIN*) pi )  // test pi->plugin
    {
        return pi->Load( aFileName, aKiway, aAppendToMe, aProperties );  // virtual
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}


void SCH_IO_MGR::Save( SCH_FILE_T aFileType, const wxString& aFileName,
                       SCH_SCREEN* aSchematic, KIWAY* aKiway, const PROPERTIES* aProperties )
{
    // release the SCH_PLUGIN even if an exception is thrown.
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( FindPlugin( aFileType ) );

    if( (SCH_PLUGIN*) pi )  // test pi->plugin
    {
        pi->Save( aFileName, aSchematic, aKiway, aProperties );  // virtual
        return;
    }

    THROW_IO_ERROR( wxString::Format( FMT_NOTFOUND, ShowType( aFileType ).GetData() ) );
}
