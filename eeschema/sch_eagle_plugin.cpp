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
#include <memory>

#include <sch_sheet.h>
#include <sch_eagle_plugin.h>

using namespace std;


SCH_EAGLE_PLUGIN::SCH_EAGLE_PLUGIN()
{
    m_rootSheet = nullptr;
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
    // TODO: Handle Kiway adn uncomment next line.
    // wxASSERT( !aFileName || aKiway != NULL );

    // Load the document
    wxXmlDocument xmlDocument;
    wxFileName fn = aFileName;

    if( !xmlDocument.Load( fn.GetFullPath() ) )
        THROW_IO_ERROR( wxString::Format( _( "Unable to read file '%s'" ), fn.GetFullPath() ) );

    // Delete on exception, if I own m_rootSheet, according to aAppendToMe
    unique_ptr<SCH_SHEET> deleter( aAppendToMe ? nullptr : m_rootSheet  );

    if( aAppendToMe )
    {
        m_rootSheet = aAppendToMe->GetRootSheet();
        // deleter = make_unique<SCH_SHEET>( nullptr );
    }
    else
    {
        m_rootSheet = new SCH_SHEET();
        m_rootSheet->SetFileName( aFileName );
        // deleter = make_unique<SCH_SHEET>( m_rootSheet );
    }

    // Retrieve the root as current node
    wxXmlNode* currentNode = xmlDocument.GetRoot();

    // If the attribute is found, store the Eagle version;
    // otherwise, store the dummy "0.0" version.
    m_version = currentNode->GetAttribute("version", "0.0");

    // Loop through all children
    currentNode = currentNode->GetChildren();
    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == "compatibility" )
        {
            // TODO: handle compatibility nodes
        }
        else if( nodeName == "drawing")
        {
            loadDrawing( currentNode );
        }
        else // DEFAULT
        {
            THROW_IO_ERROR( wxString::Format( _( "XML node '%s' unknown" ), nodeName ) );
        }

        currentNode = currentNode->GetNext();
    }

    deleter.release();
    return m_rootSheet;
}

void SCH_EAGLE_PLUGIN::loadDrawing( wxXmlNode* currentNode )
{
    currentNode = currentNode->GetChildren();

    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == "board" )
        {
            // TODO: handle board nodes
        }
        else if( nodeName == "grid" )
        {
            // TODO: handle grid nodes
        }
        else if( nodeName == "layers" )
        {
            // TODO: handle layers nodes
        }
        else if( nodeName == "library" )
        {
            // TODO: handle library nodes
        }
        else if( nodeName == "schematic" )
        {
            loadSchematic( currentNode );
        }
        else if( nodeName == "settings" )
        {
            // TODO: handle settings nodes
        }
        else // DEFAULT
        {
            THROW_IO_ERROR( wxString::Format( _( "XML node '%s' unknown" ), nodeName ) );
        }

        currentNode = currentNode->GetNext();
    }
}

void SCH_EAGLE_PLUGIN::loadSchematic( wxXmlNode* currentNode )
{
    currentNode = currentNode->GetChildren();

    while( currentNode )
    {
        wxString nodeName = currentNode->GetName();

        if( nodeName == "attributes" )
        {
            wxXmlNode* attributeNode = currentNode->GetChildren();
            while( attributeNode )
            {
                // TODO: handle attributes
                // Possible attributes: constant, display, font, layer, name,
                // ratio, rot, size, value, x, y.
                attributeNode = attributeNode->GetNext();
            }
        }
        else if( nodeName == "classes" )
        {
            // TODO : handle classes nodes
        }
        else if( nodeName == "description" )
        {
            // TODO : handle description nodes
        }
        else if( nodeName == "errors" )
        {
            // TODO : handle errors nodes
        }
        else if( nodeName == "libraries" )
        {
            // TODO : handle libraries nodes
        }
        else if( nodeName == "modules" )
        {
            // TODO : handle modules nodes
        }
        else if( nodeName == "parts" )
        {
            // TODO : handle parts nodes
        }
        else if( nodeName == "sheets" )
        {
            wxXmlNode* sheetNode = currentNode->GetChildren();
            while( sheetNode )
            {
                loadSheet( sheetNode );
                sheetNode = sheetNode->GetNext();
            }
        }
        else if( nodeName == "variantdefs" )
        {
            // TODO : handle variantdefs nodes
        }
        else // DEFAULT
        {
            THROW_IO_ERROR( wxString::Format( _( "XML node '%s' unknown" ), nodeName ) );
        }

        currentNode = currentNode->GetNext();
    }
}

void SCH_EAGLE_PLUGIN::loadSheet( wxXmlNode* currentNode )
{

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
