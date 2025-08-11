/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_embedded_files.h>
#include <eda_draw_frame.h>
#include <eda_item.h>
#include <embedded_files.h>
#include <tool/actions.h>
#include <wx/debug.h>
#include <wx/filedlg.h>

#include <tool/embed_tool.h>


EMBED_TOOL::EMBED_TOOL( const std::string& aName ) :
        TOOL_INTERACTIVE( aName ),
        m_files( nullptr )
{
}


EMBED_TOOL::EMBED_TOOL() :
        TOOL_INTERACTIVE( "common.Embed" ),
        m_files( nullptr )
{
}


bool EMBED_TOOL::Init()
{
    m_files = getModel<EDA_ITEM>()->GetEmbeddedFiles();

    return true;
}


void EMBED_TOOL::Reset( RESET_REASON aReason )
{
    m_files = getModel<EDA_ITEM>()->GetEmbeddedFiles();
}


int EMBED_TOOL::AddFile( const TOOL_EVENT& aEvent )
{
    wxString name = aEvent.Parameter<wxString>();
    m_files->AddFile( name, false );

    return 1;
}


int EMBED_TOOL::RemoveFile( const TOOL_EVENT& aEvent )
{
    wxString name = aEvent.Parameter<wxString>();
    m_files->RemoveFile( name );

    return 1;
}


std::vector<wxString> EMBED_TOOL::GetFileList()
{
    std::vector<wxString> list;

    for( auto& [name, file] : m_files->EmbeddedFileMap() )
        list.push_back( name );

    return list;
}


void EMBED_TOOL::setTransitions()
{
    Go( &EMBED_TOOL::AddFile, ACTIONS::embeddedFiles.MakeEvent() );
    Go( &EMBED_TOOL::RemoveFile, ACTIONS::removeFile.MakeEvent() );
}

