/**
 * @file dialog_symbol_remap.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <macros.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <project.h>
#include <confirm.h>
#include <reporter.h>
#include <wx_html_report_panel.h>

#include <class_library.h>
#include <sch_io_mgr.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <class_sch_screen.h>
#include <symbol_lib_table.h>

#include <dialog_symbol_remap.h>


DIALOG_SYMBOL_REMAP::DIALOG_SYMBOL_REMAP( wxWindow* aParent ) :
    DIALOG_SYMBOL_REMAP_BASE( aParent )
{
}


void DIALOG_SYMBOL_REMAP::OnRemapSymbols( wxCommandEvent& aEvent )
{
    // The schematic is fully loaded, any legacy library symbols have been rescued.  Now
    // check to see if the schematic has not been converted to the symbol library table
    // method for looking up symbols.
    wxFileName prjSymLibTableFileName( Prj().GetProjectPath(),
                                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

    if( !prjSymLibTableFileName.FileExists() )
    {
        createProjectSymbolLibTable( m_messagePanel->Reporter() );
        Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
        Prj().SchSymbolLibTable();
    }

    remapSymbolsToLibTable( m_messagePanel->Reporter() );
}


size_t DIALOG_SYMBOL_REMAP::getLibsNotInGlobalSymbolLibTable( std::vector< PART_LIB* >& aLibs )
{
    PART_LIBS* libs = Prj().SchLibs();

    for( PART_LIBS_BASE::iterator it = libs->begin(); it != libs->end(); ++it )
    {
        // Ignore the cache library.
        if( it->IsCache() )
            continue;

        // Check for the obvious library name.
        wxString libFileName = it->GetFullFileName();

        if( !SYMBOL_LIB_TABLE::GetGlobalLibTable().FindRowByURI( libFileName ) )
            aLibs.push_back( &(*it) );
    }

    return aLibs.size();
}


void DIALOG_SYMBOL_REMAP::createProjectSymbolLibTable( REPORTER& aReporter )
{
    wxString msg;
    std::vector< PART_LIB* > libs;

    if( getLibsNotInGlobalSymbolLibTable( libs ) )
    {
        SYMBOL_LIB_TABLE prjLibTable;
        std::vector< wxString > libNames = SYMBOL_LIB_TABLE::GetGlobalLibTable().GetLogicalLibs();

        for( auto lib : libs )
        {
            wxString libName = lib->GetName();
            int libNameInc = 1;
            int libNameLen = libName.Length();

            // Don't create duplicate table entries.
            while( std::find( libNames.begin(), libNames.end(), libName ) != libNames.end() )
            {
                libName = libName.Left( libNameLen );
                libName << libNameInc;
                libNameInc++;
            }

            wxString tmp;
            wxString fullFileName;
            wxString pluginType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY );
            wxFileName fn = lib->GetFullFileName();

            // Use environment variable substitution where possible.  This is based solely
            // on the internal user environment variable list.  Checking against all of the
            // system wide environment variables is probably not a good idea.
            const ENV_VAR_MAP& envMap = Pgm().GetLocalEnvVariables();
            wxFileName envPath;

            for( auto& entry : envMap )
            {
                envPath.SetPath( entry.second.GetValue() );

                if( normalizeAbsolutePaths( envPath, fn, &tmp ) )
                {
                    fullFileName = "${" + entry.first + "}/";

                    if( !tmp.IsEmpty() )
                        fullFileName += tmp;

                    fullFileName += fn.GetFullName();
                    break;
                }
            }

            // Check the project path if no local environment variable paths where found.
            if( fullFileName.IsEmpty() )
            {
                envPath.SetPath( Prj().GetProjectPath() );

                if( normalizeAbsolutePaths( envPath, fn, &tmp ) )
                {
                    fullFileName = wxString( "${" ) + PROJECT_VAR_NAME + wxString( "}/" );

                    if( !tmp.IsEmpty() )
                        fullFileName += tmp;

                    fullFileName += fn.GetFullName();
                    break;
                }
            }

            // Fall back to the absolute library path.
            if( fullFileName.IsEmpty() )
                fullFileName = lib->GetFullFileName();

            msg.Printf( _( "Adding library '%s', file '%s' to project symbol library table." ),
                        libName, fullFileName );
            aReporter.Report( msg, REPORTER::RPT_INFO );

            prjLibTable.InsertRow( new SYMBOL_LIB_TABLE_ROW( libName, fullFileName, pluginType ) );
        }

        wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            prjLibTable.Format( &formatter, 0 );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Failed to write project symbol library table. Error:\n  %s" ),
                        ioe.What() );
            aReporter.Report( msg, REPORTER::RPT_ERROR );
        }
    }
}


void DIALOG_SYMBOL_REMAP::remapSymbolsToLibTable( REPORTER& aReporter )
{
    wxString msg;
    SCH_SCREENS schematic;
    SCH_COMPONENT* symbol;
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_SCREEN* screen;

    for( screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_COMPONENT_T )
                continue;

            symbol = dynamic_cast< SCH_COMPONENT* >( item );

            if( !remapSymbolToLibTable( symbol ) )
            {
                msg.Printf( _( "No symbol '%s' founded in symbol library table." ),
                            FROM_UTF8( symbol->GetLibId().GetLibItemName() ) );
                aReporter.Report( msg, REPORTER::RPT_WARNING );
            }
            else
            {
                msg.Printf( _( "Symbol '%s' mapped to symbol library '%s'." ),
                            FROM_UTF8( symbol->GetLibId().GetLibItemName() ),
                            FROM_UTF8( symbol->GetLibId().GetLibNickname() ) );
                aReporter.Report( msg, REPORTER::RPT_ACTION );
            }
        }
    }
}


bool DIALOG_SYMBOL_REMAP::remapSymbolToLibTable( SCH_COMPONENT* aSymbol )
{
    wxCHECK_MSG( aSymbol != NULL, false, "Null pointer passed to remapSymbolToLibTable." );
    wxCHECK_MSG( aSymbol->GetLibId().GetLibNickname().empty(), false,
                 "Cannot remap symbol that is already mapped." );
    wxCHECK_MSG( !aSymbol->GetLibId().GetLibItemName().empty(), false,
                 "The symbol LIB_ID name is empty." );

    PART_LIBS* libs = Prj().SchLibs();

    for( PART_LIBS_BASE::iterator it = libs->begin(); it != libs->end(); ++it )
    {
        // Ignore the cache library.
        if( it->IsCache() )
            continue;

        LIB_ALIAS* alias = it->FindAlias( aSymbol->GetLibId().GetLibItemName() );

        // Found in the same library as the old look up method assuming the user didn't
        // change the libraries or library ordering since the last time the schematic was
        // loaded.
        if( alias )
        {
            // Find the same library in the symbol library table using the full path and file name.
            wxString libFileName = it->GetFullFileName();

            const LIB_TABLE_ROW* row = Prj().SchSymbolLibTable()->FindRowByURI( libFileName );

            if( row )
            {
                LIB_ID id = aSymbol->GetLibId();

                id.SetLibNickname( row->GetNickName() );
                aSymbol->SetLibId( id, Prj().SchSymbolLibTable() );
                return true;
            }
        }
    }

    return false;
}


bool DIALOG_SYMBOL_REMAP::normalizeAbsolutePaths( const wxFileName& aPathA,
                                                  const wxFileName& aPathB,
                                                  wxString*         aResultPath )
{
    wxCHECK_MSG( aPathA.IsAbsolute(), false, "Arguement 'aPathA' must be an absolute path." );
    wxCHECK_MSG( aPathB.IsAbsolute(), false, "Arguement 'aPathB' must be an absolute path." );

    if( aPathA.GetPath() == aPathB.GetPath() )
        return true;

    if( ( aPathA.GetDirCount() > aPathB.GetDirCount() )
      || ( aPathA.HasVolume() && !aPathB.HasVolume() )
      || ( !aPathA.HasVolume() && aPathB.HasVolume() )
      || ( ( aPathA.HasVolume() && aPathB.HasVolume() )
         && ( aPathA.GetVolume() == aPathB.GetVolume() ) ) )
        return false;

    wxArrayString aDirs = aPathA.GetDirs();
    wxArrayString bDirs = aPathB.GetDirs();

    size_t i = 0;

    while( i < aDirs.GetCount() )
    {
        if( aDirs[i] != bDirs[i] )
            return false;

        i++;
    }

    if( aResultPath )
    {
        while( i < bDirs.GetCount() )
        {
            *aResultPath += bDirs[i] + wxT( "/" );
            i++;
        }
    }

    return true;
}
