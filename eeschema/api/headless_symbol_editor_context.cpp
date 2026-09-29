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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <api/headless_symbol_editor_context.h>

#include <kiface_base.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <libraries/symbol_library_adapter.h>
#include <project.h>
#include <project_sch.h>
#include <sch_screen.h>
#include <tool/tool_manager.h>

#include <wx/debug.h>

HEADLESS_SYMBOL_EDITOR_CONTEXT::HEADLESS_SYMBOL_EDITOR_CONTEXT( std::unique_ptr<LIB_SYMBOL> aSymbol,
                                                                const LIB_ID& aLibId, PROJECT* aProject,
                                                                APP_SETTINGS_BASE* aSettings,
                                                                KIWAY* aKiway ) :
        m_symbol( std::move( aSymbol ) ),
        m_screen( std::make_unique<SCH_SCREEN>() ),
        m_libId( aLibId ),
        m_project( aProject ),
        m_kiway( aKiway ),
        m_toolManager( std::make_unique<TOOL_MANAGER>() )
{
    wxCHECK( m_symbol, /* void */ );
    wxCHECK( m_project, /* void */ );

    if( m_symbol )
        m_symbol->SetLibId( m_libId );

    m_toolManager->SetEnvironment( m_screen.get(), nullptr, nullptr, aSettings, nullptr );
}


HEADLESS_SYMBOL_EDITOR_CONTEXT::~HEADLESS_SYMBOL_EDITOR_CONTEXT() = default;


PROJECT& HEADLESS_SYMBOL_EDITOR_CONTEXT::Prj() const
{
    wxASSERT( m_project );
    return *m_project;
}


TOOL_MANAGER* HEADLESS_SYMBOL_EDITOR_CONTEXT::GetToolManager() const
{
    return m_toolManager.get();
}


bool HEADLESS_SYMBOL_EDITOR_CONTEXT::SaveSymbol()
{
    wxCHECK( m_symbol && m_project, false );

    if( m_libId.GetLibNickname().empty() )
        return false;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( m_project );

    if( !adapter->IsSymbolLibWritable( m_libId.GetUniStringLibNickname() ) )
        return false;

    try
    {
        std::unique_ptr<LIB_SYMBOL> copy = std::make_unique<LIB_SYMBOL>( *m_symbol );
        copy->SetLibId( m_libId );

        if( adapter->SaveSymbol( m_libId.GetUniStringLibNickname(), std::move( copy ) )
            != SYMBOL_LIBRARY_ADAPTER::SAVE_OK )
        {
            return false;
        }
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    m_contentModified = false;
    return true;
}


bool HEADLESS_SYMBOL_EDITOR_CONTEXT::SaveSymbolCopy( const wxString& aFileName, bool aOverwrite )
{
    wxCHECK( m_symbol, false );

    return SaveSymbolCopyToFile( *m_symbol, aFileName, aOverwrite );
}


bool HEADLESS_SYMBOL_EDITOR_CONTEXT::RevertSymbol()
{
    wxCHECK( m_symbol && m_project, false );

    if( m_libId.GetLibNickname().empty() || m_libId.GetLibItemName().empty() )
        return false;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( m_project );

    LIB_SYMBOL* loaded = adapter->LoadSymbol( m_libId );

    if( !loaded )
        return false;

    m_symbol = std::make_unique<LIB_SYMBOL>( *loaded );
    m_contentModified = false;

    return true;
}


bool HEADLESS_SYMBOL_EDITOR_CONTEXT::LoadSymbol( const LIB_ID& aLibId )
{
    wxCHECK( m_project, false );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( m_project );

    LIB_SYMBOL* loaded = adapter->LoadSymbol( aLibId );

    if( !loaded )
        return false;

    m_symbol = std::make_unique<LIB_SYMBOL>( *loaded );
    m_libId = aLibId;
    m_contentModified = false;

    return true;
}
