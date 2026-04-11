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

#include <api/sch_context.h>
#include <api/sch_api_save.h>

#include <sch_edit_frame.h>


class SCH_EDIT_FRAME_CONTEXT : public SCH_CONTEXT
{
public:
    explicit SCH_EDIT_FRAME_CONTEXT( SCH_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    SCHEMATIC* GetSchematic() const override
    {
        return &m_frame->Schematic();
    }

    PROJECT& Prj() const override
    {
        return m_frame->Prj();
    }

    TOOL_MANAGER* GetToolManager() const override
    {
        return m_frame->GetToolManager();
    }

    KIWAY* GetKiway() const override
    {
        return &m_frame->Kiway();
    }

    wxString GetCurrentFileName() const override
    {
        return m_frame->GetCurrentFileName();
    }

    bool CanAcceptApiCommands() const override
    {
        return m_frame->CanAcceptApiCommands();
    }

    std::optional<SCH_SHEET_PATH> GetCurrentSheet() const override
    {
        return m_frame->GetCurrentSheet();
    }

    bool SaveSchematic() override
    {
        return SCH_API_SAVE::SaveSchematic( m_frame->Schematic(), m_frame->Prj() );
    }

    bool SaveSchematicCopy( const wxString& aFileName, bool aCreateProject ) override
    {
        return SCH_API_SAVE::SaveSchematicCopy( m_frame->Schematic(), m_frame->Prj(), aFileName, aCreateProject );
    }

private:
    SCH_EDIT_FRAME* m_frame;
};


std::shared_ptr<SCH_CONTEXT> CreateSchFrameContext( SCH_EDIT_FRAME* aFrame )
{
    return std::make_shared<SCH_EDIT_FRAME_CONTEXT>( aFrame );
}
