/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bus_alias.h>
#include <connection_graph.h>
#include <erc_settings.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <schematic.h>
#include <sch_screen.h>


SCHEMATIC::SCHEMATIC( PROJECT* aPrj ) :
          EDA_ITEM( nullptr, SCHEMATIC_T ),
          m_project( nullptr ),
          m_rootSheet( nullptr )
{
    m_currentSheet    = new SCH_SHEET_PATH();
    m_connectionGraph = new CONNECTION_GRAPH( this );

    SetProject( aPrj );
}


SCHEMATIC::~SCHEMATIC()
{
    delete m_currentSheet;
    delete m_connectionGraph;
}


void SCHEMATIC::Reset()
{
    // Assume project already saved
    if( m_project )
    {
        PROJECT_FILE& project = m_project->GetProjectFile();

        delete project.m_ErcSettings;
        delete project.m_SchematicSettings;

        project.m_ErcSettings       = nullptr;
        project.m_SchematicSettings = nullptr;
    }

    delete m_rootSheet;

    m_rootSheet = nullptr;

    m_connectionGraph->Reset();
    m_currentSheet->clear();
}


void SCHEMATIC::SetProject( PROJECT* aPrj )
{
    if( m_project )
    {
        PROJECT_FILE& project = m_project->GetProjectFile();

        delete project.m_ErcSettings;
        delete project.m_SchematicSettings;

        project.m_ErcSettings       = nullptr;
        project.m_SchematicSettings = nullptr;
    }

    m_project = aPrj;

    if( m_project )
    {
        PROJECT_FILE& project       = m_project->GetProjectFile();
        project.m_ErcSettings       = new ERC_SETTINGS( &project, "erc" );
        project.m_SchematicSettings = new SCHEMATIC_SETTINGS( &project, "schematic" );

        project.m_SchematicSettings->m_TemplateFieldNames = project.m_TemplateFieldNames;
        project.m_SchematicSettings->LoadFromFile();

        project.m_ErcSettings->LoadFromFile();
    }
}


void SCHEMATIC::SetTemplateFieldNames( TEMPLATES* aTemplates )
{
    wxASSERT( m_project );
    PROJECT_FILE& project = m_project->GetProjectFile();

    project.m_TemplateFieldNames = aTemplates;

    if( project.m_SchematicSettings )
        project.m_SchematicSettings->m_TemplateFieldNames = aTemplates;
}


void SCHEMATIC::SetRoot( SCH_SHEET* aRootSheet )
{
    wxCHECK_RET( aRootSheet, "Call to SetRoot with null SCH_SHEET!" );

    m_rootSheet = aRootSheet;

    m_currentSheet->clear();
    m_currentSheet->push_back( m_rootSheet );

    m_connectionGraph->Reset();
}


SCH_SCREEN* SCHEMATIC::RootScreen() const
{
    return IsValid() ? m_rootSheet->GetScreen() : nullptr;
}


wxString SCHEMATIC::GetFileName() const
{
    return IsValid() ? m_rootSheet->GetScreen()->GetFileName() : wxString( wxEmptyString );
}


SCHEMATIC_SETTINGS& SCHEMATIC::Settings() const
{
    wxASSERT( m_project );
    return *m_project->GetProjectFile().m_SchematicSettings;
}


ERC_SETTINGS& SCHEMATIC::ErcSettings() const
{
    wxASSERT( m_project );
    return *m_project->GetProjectFile().m_ErcSettings;
}


std::shared_ptr<BUS_ALIAS> SCHEMATIC::GetBusAlias( const wxString& aLabel ) const
{
    for( const auto& sheet : GetSheets() )
    {
        for( const auto& alias : sheet.LastScreen()->GetBusAliases() )
        {
            if( alias->GetName() == aLabel )
                return alias;
        }
    }

    return nullptr;
}


std::vector<wxString> SCHEMATIC::GetNetClassAssignmentCandidates()
{
    std::vector<wxString> names;

    SCH_SCREENS allScreens( Root() );

    for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items() )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN* pin = static_cast<SCH_PIN*>( item );

                if( pin->IsPowerConnection() )
                    names.emplace_back( pin->GetName() );
            }
                break;

            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_SHEET_PIN_T:
            {
                wxString unescaped = static_cast<SCH_TEXT*>( item )->GetShownText();

                wxString busPrefix;
                std::vector<wxString> busMembers;

                if( NET_SETTINGS::ParseBusVector( unescaped, nullptr, nullptr ) )
                {
                    // Allow netclass assignment to an entire vector.
                    names.emplace_back( EscapeString( unescaped, CTX_NETNAME ) );
                }
                else if( NET_SETTINGS::ParseBusGroup( unescaped, &busPrefix, &busMembers ) )
                {
                    // Allow netclass assignment to an entire group.
                    names.emplace_back( EscapeString( unescaped, CTX_NETNAME ) );

                    // Named bus groups generate a net prefix, unnamed ones don't
                    if( !busPrefix.IsEmpty() )
                        busPrefix += wxT( "." );

                    for( const wxString& member : busMembers )
                    {
                        // Handle alias inside bus group member list
                        if( const std::shared_ptr<BUS_ALIAS>& alias = GetBusAlias( member ) )
                        {
                            for( const wxString& alias_member : alias->Members() )
                                names.emplace_back( busPrefix + alias_member );
                        }
                        else
                        {
                            names.emplace_back( busPrefix + member );
                        }
                    }
                }
                else
                {
                    names.emplace_back( EscapeString( unescaped, CTX_NETNAME ) );
                }
            }
                break;

            default:
                break;
            }
        }
    }

    return names;
}


bool SCHEMATIC::ResolveCrossReference( wxString* token, int aDepth ) const
{
    SCH_SHEET_LIST sheetList = GetSheets();
    wxString       remainder;
    wxString       ref = token->BeforeFirst( ':', &remainder );
    SCH_SHEET_PATH dummy;
    SCH_ITEM*      refItem = sheetList.GetItem( KIID( ref ), &dummy );

    if( refItem && refItem->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* refComponent = static_cast<SCH_COMPONENT*>( refItem );

        if( refComponent->ResolveTextVar( &remainder, aDepth + 1 ) )
        {
            *token = remainder;
            return true;
        }
    }
    else if( refItem && refItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* refSheet = static_cast<SCH_SHEET*>( refItem );

        if( refSheet->ResolveTextVar( &remainder, aDepth + 1 ) )
        {
            *token = remainder;
            return true;
        }
    }

    return false;
}


wxString SCHEMATIC::ConvertRefsToKIIDs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                SCH_SHEET_LIST     sheetList = GetSheets();
                wxString           remainder;
                wxString           ref = token.BeforeFirst( ':', &remainder );
                SCH_REFERENCE_LIST references;

                sheetList.GetComponents( references );

                for( size_t jj = 0; jj < references.GetCount(); jj++ )
                {
                    SCH_COMPONENT* refComponent = references[ jj ].GetComp();

                    if( ref == refComponent->GetRef( &references[ jj ].GetSheetPath(), true ) )
                    {
                        wxString test( remainder );

                        if( refComponent->ResolveTextVar( &test ) )
                            token = refComponent->m_Uuid.AsString() + ":" + remainder;

                        break;
                    }
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString SCHEMATIC::ConvertKIIDsToRefs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                SCH_SHEET_LIST sheetList = GetSheets();
                wxString       remainder;
                wxString       ref = token.BeforeFirst( ':', &remainder );

                SCH_SHEET_PATH refSheetPath;
                SCH_ITEM*      refItem = sheetList.GetItem( KIID( ref ), &refSheetPath );

                if( refItem && refItem->Type() == SCH_COMPONENT_T )
                {
                    SCH_COMPONENT* refComponent = static_cast<SCH_COMPONENT*>( refItem );
                    token = refComponent->GetRef( &refSheetPath, true ) + ":" + remainder;
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


