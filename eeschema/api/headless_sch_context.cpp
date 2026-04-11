/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/headless_sch_context.h>
#include <api/sch_api_save.h>
#include <project.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <kiface_base.h>
#include <wx/debug.h>


HEADLESS_SCH_CONTEXT::HEADLESS_SCH_CONTEXT( SCHEMATIC* aSchematic, PROJECT* aProject,
                                            KIWAY* aKiway ) :
        m_schematic( aSchematic ),
        m_project( aProject ),
        m_kiway( aKiway ),
        m_toolManager( std::make_unique<TOOL_MANAGER>() )
{
    wxCHECK( m_schematic, /* void */ );
    wxCHECK( m_project, /* void */ );

    m_toolManager->SetEnvironment( m_schematic, nullptr, nullptr,
                                   Kiface().KifaceSettings(), nullptr );
}


HEADLESS_SCH_CONTEXT::~HEADLESS_SCH_CONTEXT() = default;


SCHEMATIC* HEADLESS_SCH_CONTEXT::GetSchematic() const
{
    return m_schematic;
}


PROJECT& HEADLESS_SCH_CONTEXT::Prj() const
{
    wxASSERT( m_project );
    return *m_project;
}


TOOL_MANAGER* HEADLESS_SCH_CONTEXT::GetToolManager() const
{
    return m_toolManager.get();
}


wxString HEADLESS_SCH_CONTEXT::GetCurrentFileName() const
{
    if( !m_schematic )
        return wxEmptyString;

    return m_schematic->GetFileName();
}


bool HEADLESS_SCH_CONTEXT::SaveSchematic()
{
    wxCHECK( m_schematic && m_project, false );
    return SCH_API_SAVE::SaveSchematic( *m_schematic, *m_project );
}


bool HEADLESS_SCH_CONTEXT::SaveSchematicCopy( const wxString& aFileName, bool aCreateProject )
{
    wxCHECK( m_schematic && m_project, false );
    return SCH_API_SAVE::SaveSchematicCopy( *m_schematic, *m_project, aFileName, aCreateProject );
}
