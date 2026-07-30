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

#include <footprint_editor_tab_context.h>

#include <wx/translation.h>

#include <board.h>
#include <footprint.h>


FOOTPRINT_EDITOR_TAB_CONTEXT::FOOTPRINT_EDITOR_TAB_CONTEXT( const wxString& aLib,
                                                            const wxString& aName,
                                                            std::unique_ptr<BOARD> aBoard ) :
        m_lib( aLib ),
        m_name( aName ),
        m_board( std::move( aBoard ) ),
        m_footprintNameWhenLoaded( aName )
{
}


FOOTPRINT_EDITOR_TAB_CONTEXT::FOOTPRINT_EDITOR_TAB_CONTEXT( const KIID&            aSourceUuid,
                                                            const wxString&        aReference,
                                                            std::unique_ptr<BOARD> aBoard ) :
        m_board( std::move( aBoard ) ),
        m_kind( KIND::BOARD_INSTANCE ),
        m_sourceUuid( aSourceUuid ),
        m_reference( aReference )
{
}


FOOTPRINT_EDITOR_TAB_CONTEXT::FOOTPRINT_EDITOR_TAB_CONTEXT( KIND aKind, std::unique_ptr<BOARD> aBoard ) :
        m_board( std::move( aBoard ) ),
        m_kind( aKind )
{
}


FOOTPRINT_EDITOR_TAB_CONTEXT::~FOOTPRINT_EDITOR_TAB_CONTEXT() = default;


std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>
FOOTPRINT_EDITOR_TAB_CONTEXT::MakeUnsaved( std::unique_ptr<BOARD> aBoard )
{
    return std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT>(
            new FOOTPRINT_EDITOR_TAB_CONTEXT( KIND::UNSAVED, std::move( aBoard ) ) );
}


wxString FOOTPRINT_EDITOR_TAB_CONTEXT::GetDisplayName() const
{
    switch( m_kind )
    {
    case KIND::BOARD_INSTANCE: return m_reference;

    // The imported name is not an identity the editor can save to, so the tab reads as unnamed
    case KIND::UNSAVED:        return _( "<unnamed>" );

    default:                   return m_name;
    }
}


void FOOTPRINT_EDITOR_TAB_CONTEXT::PromoteToLibrary( const wxString& aLib, const wxString& aName )
{
    wxCHECK( m_kind == KIND::UNSAVED, /* void */ );

    m_kind = KIND::LIBRARY;
    m_lib = aLib;
    m_name = aName;
    m_footprintNameWhenLoaded = aName;
}


bool FOOTPRINT_EDITOR_TAB_CONTEXT::IsModified() const
{
    return m_modified && m_board && m_board->GetFirstFootprint();
}


void FOOTPRINT_EDITOR_TAB_CONTEXT::SetOriginalFootprintCopy( std::unique_ptr<FOOTPRINT> aCopy )
{
    m_originalFootprintCopy = std::move( aCopy );
}
