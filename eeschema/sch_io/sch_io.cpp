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

#include <unordered_set>

#include <ki_exception.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <wx/translation.h>
#include <wx/filename.h>
#include <wx/dir.h>

#define FMT_UNIMPLEMENTED wxT( "Plugin \"%s\" does not implement the \"%s\" function." )
#define NOT_IMPLEMENTED( aCaller )                                                   \
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED,                             \
                                      GetName().GetData(),                           \
                                      wxString::FromUTF8( aCaller ).GetData() ) );


const IO_BASE::IO_FILE_DESC SCH_IO::GetSchematicFileDesc() const
{
    return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
}


bool SCH_IO::CanReadSchematicFile( const wxString& aFileName ) const
{
    const std::vector<std::string>& exts = GetSchematicFileDesc().m_FileExtensions;

    wxString fileExt = wxFileName( aFileName ).GetExt().MakeLower();

    for( const std::string& ext : exts )
    {
        if( fileExt == wxString( ext ).Lower() )
            return true;
    }

    return false;
}


void SCH_IO::SaveLibrary( const wxString& aFileName, const std::map<std::string, UTF8>* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


SCH_SHEET* SCH_IO::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                      SCH_SHEET* aAppendToMe, const std::map<std::string, UTF8>* aProperties )
{
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
                                const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                 const wxString&   aLibraryPath,
                                 const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                 const wxString&   aLibraryPath,
                                 const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


LIB_SYMBOL* SCH_IO::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                         const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                           const std::map<std::string, UTF8>* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}


void SCH_IO::GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const
{
    // Get base options first
    IO_BASE::GetLibraryOptions( aListToAppendTo );

    // Empty for most plugins
    //
    // To add a new option override and use example code below:
    //
    //(*aListToAppendTo)["new_option_name"] = UTF8( _(
    //    "A nice descrtiption with possibility for <b>bold</b> and other formatting."
    //    ) );
}


const wxString& SCH_IO::GetError() const
{
    // not pure virtual so that plugins only have to implement subset of the SCH_IO interface.
    NOT_IMPLEMENTED( __FUNCTION__ );
}
