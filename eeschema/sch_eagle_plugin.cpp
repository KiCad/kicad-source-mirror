/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2017 CERN
*
* @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <properties.h>

#include <wx/filename.h>

// #include <richio.h>
#include <sch_eagle_plugin.h>


SCH_EAGLE_PLUGIN::SCH_EAGLE_PLUGIN()
{
}

SCH_EAGLE_PLUGIN::~SCH_EAGLE_PLUGIN()
{
}

const wxString SCH_EAGLE_PLUGIN::GetName() const
{
    return wxT( "EAGLE" );
}


const wxString SCH_EAGLE_PLUGIN::GetFileExtension() const
{
    return wxT( "sch" );
}



int SCH_EAGLE_PLUGIN::GetModifyHash() const
{
    return 0;
}


void SCH_EAGLE_PLUGIN::SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties )
{
}


SCH_SHEET* SCH_EAGLE_PLUGIN::Load( const wxString& aFileName, KIWAY* aKiway,
                                   SCH_SHEET* aAppendToMe, const PROPERTIES* aProperties )
{
    wxASSERT( !aFileName || aKiway != NULL );

    SCH_SHEET*  sheet = nullptr;

    wxFileName fn = aFileName;

    if( !m_xmlTree.Load( fn.GetFullPath() ) )
        THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ), fn.GetFullPath() ) );

    return sheet;
}


void SCH_EAGLE_PLUGIN::Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
                             const PROPERTIES* aProperties )
{
}


size_t SCH_EAGLE_PLUGIN::GetSymbolLibCount( const wxString&   aLibraryPath,
                                            const PROPERTIES* aProperties )
{
    return 0;
}


void SCH_EAGLE_PLUGIN::EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                                           const wxString&   aLibraryPath,
                                           const PROPERTIES* aProperties )
{
}


LIB_ALIAS* SCH_EAGLE_PLUGIN::LoadSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                                         const PROPERTIES* aProperties )
{
    return nullptr;
}


void SCH_EAGLE_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                                   const PROPERTIES* aProperties )
{
}


void SCH_EAGLE_PLUGIN::DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                                    const PROPERTIES* aProperties )
{
}


void SCH_EAGLE_PLUGIN::DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                     const PROPERTIES* aProperties )
{
}


void SCH_EAGLE_PLUGIN::CreateSymbolLib( const wxString&   aLibraryPath,
                                        const PROPERTIES* aProperties )
{
}


bool SCH_EAGLE_PLUGIN::DeleteSymbolLib( const wxString&   aLibraryPath,
                                        const PROPERTIES* aProperties )
{
    return false;
}


bool SCH_EAGLE_PLUGIN::IsSymbolLibWritable( const wxString& aLibraryPath )
{
    return false;
}

void SCH_EAGLE_PLUGIN::SymbolLibOptions( PROPERTIES* aListToAppendTo ) const
{
}
