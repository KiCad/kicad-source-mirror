/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <symbol_editor/symbol_editor_tab_context.h>

#include <lib_symbol.h>
#include <sch_screen.h>
#include <symbol_library_manager.h>


SYMBOL_EDITOR_TAB_CONTEXT::SYMBOL_EDITOR_TAB_CONTEXT( const wxString& aLib, const wxString& aName,
                                                      SYMBOL_BUFFER* aBuffer ) :
        m_lib( aLib ),
        m_name( aName ),
        m_symbol( nullptr ),
        m_screen( nullptr ),
        m_frameOwns( false ),
        m_unit( 1 ),
        m_bodyStyle( 1 ),
        m_fromSchematic( false )
{
    // Edit a private clone; the buffer's symbol stays owned by the libMgr.
    if( aBuffer )
        m_symbol = new LIB_SYMBOL( aBuffer->GetSymbol() );

    // Symbol geometry lives in the LIB_SYMBOL, so a fresh empty screen is correct.
    m_screen = new SCH_SCREEN();

    if( aBuffer && aBuffer->IsModified() )
        m_screen->SetContentModified();
}


SYMBOL_EDITOR_TAB_CONTEXT::SYMBOL_EDITOR_TAB_CONTEXT( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                                                      const KIID&     aSchematicSymbolUUID,
                                                      const wxString& aReference ) :
        m_symbol( aSymbol ),
        m_screen( aScreen ),
        m_frameOwns( false ),
        m_unit( 1 ),
        m_bodyStyle( 1 ),
        m_fromSchematic( true ),
        m_schematicSymbolUUID( aSchematicSymbolUUID ),
        m_reference( aReference )
{
}


SYMBOL_EDITOR_TAB_CONTEXT::~SYMBOL_EDITOR_TAB_CONTEXT()
{
    // Delete only when the context still owns the objects; an active tab's frame owns them.
    if( !m_frameOwns )
    {
        delete m_symbol;
        delete m_screen;
    }
}


bool SYMBOL_EDITOR_TAB_CONTEXT::IsModified() const
{
    return m_screen && m_screen->IsContentModified();
}
