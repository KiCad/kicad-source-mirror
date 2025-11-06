/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_io/sch_io_mgr.h>
#include <sch_io/eagle/sch_io_eagle.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>

#include <sch_io/altium/sch_io_altium.h>
#include <sch_io/cadstar/sch_io_cadstar_archive.h>
#include <sch_io/easyeda/sch_io_easyeda.h>
#include <sch_io/easyedapro/sch_io_easyedapro.h>
#include <sch_io/database/sch_io_database.h>
#include <sch_io/ltspice/sch_io_ltspice.h>
#include <sch_io/http_lib/sch_io_http_lib.h>
#include <common.h>     // for ExpandEnvVarSubstitutions

#include <wildcards_and_files_ext.h>
#include <kiway_player.h>
#include <string_utils.h>
#include <libraries/library_table_parser.h>

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


SCH_IO* SCH_IO_MGR::FindPlugin( SCH_FILE_T aFileType )
{
    // This implementation is subject to change, any magic is allowed here.
    // The public SCH_IO_MGR API is the only pertinent public information.

    switch( aFileType )
    {
    case SCH_KICAD:           return new SCH_IO_KICAD_SEXPR();
    case SCH_LEGACY:          return new SCH_IO_KICAD_LEGACY();
    case SCH_ALTIUM:          return new SCH_IO_ALTIUM();
    case SCH_CADSTAR_ARCHIVE: return new SCH_IO_CADSTAR_ARCHIVE();
    case SCH_DATABASE:        return new SCH_IO_DATABASE();
    case SCH_EAGLE:           return new SCH_IO_EAGLE();
    case SCH_EASYEDA:         return new SCH_IO_EASYEDA();
    case SCH_EASYEDAPRO:      return new SCH_IO_EASYEDAPRO();
    case SCH_LTSPICE:         return new SCH_IO_LTSPICE();
    case SCH_HTTP:            return new SCH_IO_HTTP_LIB();
    default:                  return nullptr;
    }
}


const wxString SCH_IO_MGR::ShowType( SCH_FILE_T aType )
{
    // keep this function in sync with EnumFromStr() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    switch( aType )
    {
    case SCH_KICAD:           return wxString( wxT( "KiCad" ) );
    case SCH_LEGACY:          return wxString( wxT( "Legacy" ) );
    case SCH_ALTIUM:          return wxString( wxT( "Altium" ) );
    case SCH_CADSTAR_ARCHIVE: return wxString( wxT( "CADSTAR Schematic Archive" ) );
    case SCH_DATABASE:        return wxString( wxT( "Database" ) );
    case SCH_EAGLE:           return wxString( wxT( "EAGLE" ) );
    case SCH_EASYEDA:         return wxString( wxT( "EasyEDA (JLCEDA) Std" ) );
    case SCH_EASYEDAPRO:      return wxString( wxT( "EasyEDA (JLCEDA) Pro" ) );
    case SCH_LTSPICE:         return wxString( wxT( "LTspice" ) );
    case SCH_HTTP:            return wxString( wxT( "HTTP" ) );
    case SCH_NESTED_TABLE:    return LIBRARY_TABLE_ROW::TABLE_TYPE_NAME;
    default:                  return wxString::Format( _( "Unknown SCH_FILE_T value: %d" ), aType );
    }
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::EnumFromStr( const wxString& aType )
{
    // keep this function in sync with ShowType() relative to the
    // text spellings.  If you change the spellings, you will obsolete
    // library tables, so don't do change, only additions are ok.

    if( aType == wxT( "KiCad" ) )
        return SCH_KICAD;
    else if( aType == wxT( "Legacy" ) )
        return SCH_LEGACY;
    else if( aType == wxT( "Altium" ) )
        return SCH_ALTIUM;
    else if( aType == wxT( "CADSTAR Schematic Archive" ) )
        return SCH_CADSTAR_ARCHIVE;
    else if( aType == wxT( "Database" ) )
        return SCH_DATABASE;
    else if( aType == wxT( "EAGLE" ) )
        return SCH_EAGLE;
    else if( aType == wxT( "EasyEDA (JLCEDA) Std" ) )
        return SCH_EASYEDA;
    else if( aType == wxT( "EasyEDA (JLCEDA) Pro" ) )
        return SCH_EASYEDAPRO;
    else if( aType == wxT( "LTspice" ) )
        return SCH_LTSPICE;
    else if( aType == wxT( "HTTP" ) )
        return SCH_HTTP;
    else if( aType == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        return SCH_NESTED_TABLE;

    return SCH_FILE_UNKNOWN;
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::GuessPluginTypeFromLibPath( const wxString& aLibPath, int aCtl )
{
    LIBRARY_TABLE_PARSER parser;

    if( parser.Parse( aLibPath.ToStdString() ).has_value() )
        return SCH_NESTED_TABLE;

    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        bool isKiCad = fileType == SCH_IO_MGR::SCH_KICAD || fileType == SCH_IO_MGR::SCH_LEGACY;

        if( ( aCtl & KICTL_KICAD_ONLY ) && !isKiCad )
            continue;

        if( ( aCtl & KICTL_NONKICAD_ONLY ) && isKiCad )
            continue;

        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        // For SCH_IO_MGR::SCH_KICAD and KICTL_CREATE option is set, use SCH_IO::CanReadLibrary()
        // here instead  of SCH_IO_KICAD_SEXPR::CanReadLibrary because aLibPath perhaps
        // does notexist, and we need to use the version that does not test the existence
        // of the file, just know if aLibPath file type can be handled.
        if( fileType == SCH_IO_MGR::SCH_KICAD && ( aCtl & KICTL_CREATE ) )
        {
            if( pi->SCH_IO::CanReadLibrary( aLibPath ) )    // Test only the file ext
                return fileType;
        }
        else
        {
            // Other lib types must be tested using the specific CanReadLibrary() and
            // in some cases need to read the file
            if( pi->CanReadLibrary( aLibPath ) )
                return fileType;
        }
    }

    return SCH_IO_MGR::SCH_FILE_UNKNOWN;
}


SCH_IO_MGR::SCH_FILE_T SCH_IO_MGR::GuessPluginTypeFromSchPath( const wxString& aSchematicPath,
                                                               int             aCtl )
{
    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        bool isKiCad = fileType == SCH_IO_MGR::SCH_KICAD || fileType == SCH_IO_MGR::SCH_LEGACY;

        if( ( aCtl & KICTL_KICAD_ONLY ) && !isKiCad )
            continue;

        if( ( aCtl & KICTL_NONKICAD_ONLY ) && isKiCad )
            continue;

        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        if( pi->CanReadSchematicFile( aSchematicPath ) )
            return fileType;
    }

    return SCH_IO_MGR::SCH_FILE_UNKNOWN;
}


bool SCH_IO_MGR::ConvertLibrary( std::map<std::string, UTF8>* aOldFileProps, const wxString& aOldFilePath,
                                 const wxString& aNewFilepath )
{
    SCH_IO_MGR::SCH_FILE_T oldFileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( aOldFilePath );

    if( oldFileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        return false;

    IO_RELEASER<SCH_IO>                oldFilePI( SCH_IO_MGR::FindPlugin( oldFileType ) );
    IO_RELEASER<SCH_IO>                kicadPI( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    std::vector<LIB_SYMBOL*>           symbols;
    std::vector<LIB_SYMBOL*>           newSymbols;
    std::map<LIB_SYMBOL*, LIB_SYMBOL*> symbolMap;

    try
    {
        oldFilePI->EnumerateSymbolLib( symbols, aOldFilePath, aOldFileProps );

        // Copy non-derived symbols first, so we can build a map from symbols to newSymbols
        for( LIB_SYMBOL* symbol : symbols )
        {
            if( symbol->IsDerived() )
                continue;

            symbol->SetName( EscapeString( symbol->GetName(), CTX_LIBID ) );

            newSymbols.push_back( new LIB_SYMBOL( *symbol ) );
            symbolMap[symbol] = newSymbols.back();
        }

        // Now do the derived symbols using the map to hook them up to their newSymbol parents
        for( LIB_SYMBOL* symbol : symbols )
        {
            if( !symbol->IsDerived() )
                continue;

            symbol->SetName( EscapeString( symbol->GetName(), CTX_LIBID ) );

            newSymbols.push_back( new LIB_SYMBOL( *symbol ) );
            newSymbols.back()->SetParent( symbolMap[ symbol->GetParent().lock().get() ] );
        }

        // Create a blank library
        kicadPI->SaveLibrary( aNewFilepath );

        // Finally write out newSymbols
        for( LIB_SYMBOL* symbol : newSymbols )
        {
            kicadPI->SaveSymbol( aNewFilepath, symbol );
        }
    }
    catch( ... )
    {
        return false;
    }

    return true;
}
