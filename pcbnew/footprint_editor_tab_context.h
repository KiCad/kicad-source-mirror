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

#ifndef FOOTPRINT_EDITOR_TAB_CONTEXT_H
#define FOOTPRINT_EDITOR_TAB_CONTEXT_H

#include <memory>

#include <wx/string.h>

#include <widgets/editor_tab_context.h>

class BOARD;
class FOOTPRINT;


/**
 * One open footprint tab owning its fp-holder board, lent to the frame by raw pointer while active.
 */
class FOOTPRINT_EDITOR_TAB_CONTEXT : public EDITOR_TAB_CONTEXT
{
public:
    FOOTPRINT_EDITOR_TAB_CONTEXT( const wxString& aLib, const wxString& aName,
                                  std::unique_ptr<BOARD> aBoard );

    ~FOOTPRINT_EDITOR_TAB_CONTEXT() override;

    wxString GetTabKey() const override     { return m_lib + wxT( ":" ) + m_name; }
    wxString GetDisplayName() const override { return m_name; }

    /**
     * True only when dirty and the board actually holds a footprint to edit.
     */
    bool IsModified() const override;

    void SetModified( bool aModified ) { m_modified = aModified; }

    /**
     * The fp-holder board owned by this context. The frame borrows this pointer when active.
     */
    BOARD* GetBoard() const { return m_board.get(); }

    const wxString& GetLib() const  { return m_lib; }
    const wxString& GetName() const { return m_name; }
    void SetName( const wxString& aName ) { m_name = aName; }

    /**
     * Baseline clone captured at load, used to detect edits and to revert.
     */
    FOOTPRINT* GetOriginalFootprintCopy() const { return m_originalFootprintCopy.get(); }
    void       SetOriginalFootprintCopy( std::unique_ptr<FOOTPRINT> aCopy );

    const wxString& GetFootprintNameWhenLoaded() const { return m_footprintNameWhenLoaded; }
    void SetFootprintNameWhenLoaded( const wxString& aName ) { m_footprintNameWhenLoaded = aName; }

private:
    wxString                   m_lib;
    wxString                   m_name;
    std::unique_ptr<BOARD>     m_board;
    std::unique_ptr<FOOTPRINT> m_originalFootprintCopy;
    wxString                   m_footprintNameWhenLoaded;
    bool                       m_modified = false;
};

#endif // FOOTPRINT_EDITOR_TAB_CONTEXT_H
