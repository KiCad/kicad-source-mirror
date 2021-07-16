/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_plugins/eagle/sch_eagle_plugin.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>

#include <sch_plugins/altium/sch_altium_plugin.h>
#include <sch_plugins/cadstar/cadstar_sch_archive_plugin.h>
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


SCH_PLUGIN* SCH_IO_MGR::FindPlugin( SCH_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public SCH_IO_MGR API is the only pertinent public information.

    switch( aFileType )
    {
    case SCH_LEGACY:          return new SCH_LEGACY_PLUGIN();
    case SCH_KICAD:           return new SCH_SEXPR_PLUGIN();
    case SCH_ALTIUM:          return new SCH_ALTIUM_PLUGIN();
    case SCH_CADSTAR_ARCHIVE: return new CADSTAR_SCH_ARCHIVE_PLUGIN();
    case SCH_EAGLE:           return new SCH_EAGLE_PLUGIN();
    default:                  return nullptr;
    }
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
    case SCH_LEGACY:          return wxString( wxT( "Legacy" ) );
    case SCH_KICAD:           return wxString( wxT( "KiCad" ) );
    case SCH_ALTIUM:          return wxString( wxT( "Altium" ) );
    case SCH_CADSTAR_ARCHIVE: return wxString( wxT( "CADSTAR Schematic Archive" ) );
    case SCH_EAGLE:           return wxString( wxT( "EAGLE" ) );
    default:                  return wxString::Format( _( "Unknown SCH_FILE_T value: %d" ),
                                                       aType );
    }
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::EnumFromStr( const wxString& aType )
{
    // keep this function in sync with ShowType() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    if( aType == wxT( "Legacy" ) )
        return SCH_LEGACY;
    else if( aType == wxT( "KiCad" ) )
        return SCH_KICAD;
    else if( aType == wxT( "Altium" ) )
        return SCH_ALTIUM;
    else if( aType == wxT( "CADSTAR Schematic Archive" ) )
        return SCH_CADSTAR_ARCHIVE;
    else if( aType == wxT( "EAGLE" ) )
        return SCH_EAGLE;

    // wxASSERT( blow up here )

    return SCH_FILE_T( -1 );
}


const wxString SCH_IO_MGR::GetFileExtension( SCH_FILE_T aFileType )
{
    wxString ext = wxEmptyString;
    SCH_PLUGIN* plugin = FindPlugin( aFileType );

    if( plugin != nullptr )
    {
        ext = plugin->GetFileExtension();
        ReleasePlugin( plugin );
    }

    return ext;
}


const wxString SCH_IO_MGR::GetLibraryFileExtension( SCH_FILE_T aFileType )
{
    wxString ext = wxEmptyString;
    SCH_PLUGIN* plugin = FindPlugin( aFileType );

    if( plugin != nullptr )
    {
        ext = plugin->GetLibraryFileExtension();
        ReleasePlugin( plugin );
    }

    return ext;
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath )
{
    SCH_FILE_T  ret = SCH_KICAD;        // default guess, unless detected otherwise.
    wxFileName  fn( aLibPath );

    if( fn.GetExt() == LegacySymbolLibFileExtension )
    {
        ret = SCH_LEGACY;
    }
    else if( fn.GetExt() == KiCadSymbolLibFileExtension )
    {
        ret = SCH_KICAD;
    }

    return ret;
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::GuessPluginTypeFromSchPath( const wxString& aSchematicPath )
{
    SCH_FILE_T  ret = SCH_KICAD;        // default guess, unless detected otherwise.
    wxFileName  fn( aSchematicPath );

    if( fn.GetExt() == LegacySchematicFileExtension )
    {
        ret = SCH_LEGACY;
    }
    else if( fn.GetExt() == KiCadSchematicFileExtension )
    {
        ret = SCH_KICAD;
    }

    return ret;
}


DECLARE_ENUM_VECTOR( SCH_IO_MGR, SCH_FILE_T )
