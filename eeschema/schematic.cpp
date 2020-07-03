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



