/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_logger.h>
#include <lib_symbol_library_manager.h>
#include <symbol_edit_frame.h>


LIB_SYMBOL_LIBRARY_MANAGER::LIB_SYMBOL_LIBRARY_MANAGER( SYMBOL_EDIT_FRAME& aFrame ) :
        SYMBOL_LIBRARY_MANAGER( aFrame ),
        m_syncHash( 0 )
{
    m_adapter = SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( &aFrame, this );
    m_adapter->ShowUnits( false );
}


void LIB_SYMBOL_LIBRARY_MANAGER::Sync( const wxString& aForceRefresh,
                                       std::function<void( int, int,
                                                           const wxString& )> aProgressCallback )
{
    m_logger->Activate();
    {
        getAdapter()->Sync( aForceRefresh, aProgressCallback );
        m_syncHash = symTable()->GetModifyHash();
    }
    m_logger->Deactivate();
}


void LIB_SYMBOL_LIBRARY_MANAGER::OnDataChanged() const
{
    static_cast<SYMBOL_EDIT_FRAME&>( m_frame ).SyncLibraries( false );
}
