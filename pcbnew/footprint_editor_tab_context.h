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

#ifndef FOOTPRINT_EDITOR_TAB_CONTEXT_H
#define FOOTPRINT_EDITOR_TAB_CONTEXT_H

#include <map>
#include <memory>

#include <wx/string.h>

#include <kiid.h>
#include <widgets/editor_tab_context.h>

class BOARD;
class FOOTPRINT;


/**
 * One open footprint tab owning its fp-holder board, lent to the frame by raw pointer while active.
 *
 * A tab is one of three kinds.  A library tab edits a library footprint, is keyed by its library:name
 * pair and persisted across sessions.  An instance tab edits a footprint pulled from a placed board
 * footprint (Ctrl-E); it is keyed by the source board footprint UUID, carries the board-uuid remap
 * used to save the edits back to that instance, and is session-only so it is never persisted.  An
 * unsaved tab edits an imported footprint that has no library home yet; it is keyed by a session id
 * and is promoted to a library tab by the save-as that gives it an identity.
 */
class FOOTPRINT_EDITOR_TAB_CONTEXT : public EDITOR_TAB_CONTEXT
{
public:
    /**
     * What the tab edits, which decides its key, its label and whether it is persisted.
     */
    enum class KIND
    {
        LIBRARY,        ///< A library footprint, keyed library:name and persisted across sessions
        BOARD_INSTANCE, ///< A footprint pulled off a placed board footprint, session-only
        UNSAVED         ///< An imported footprint with no library home yet, session-only
    };

    FOOTPRINT_EDITOR_TAB_CONTEXT( const wxString& aLib, const wxString& aName,
                                  std::unique_ptr<BOARD> aBoard );

    /**
     * Construct an instance (board) tab over an fp-holder board carrying a clone of a placed
     * footprint.
     *
     * @param aSourceUuid is the source board footprint's UUID, used as the de-dup key and tab label.
     * @param aReference is the placed footprint reference shown on the tab.
     */
    FOOTPRINT_EDITOR_TAB_CONTEXT( const KIID& aSourceUuid, const wxString& aReference,
                                  std::unique_ptr<BOARD> aBoard );

    ~FOOTPRINT_EDITOR_TAB_CONTEXT() override;

    /**
     * Construct a tab for an imported footprint that has no library identity yet.
     *
     * Each import gets its own session id, so importing the same file twice opens two tabs rather
     * than one clobbering the other.
     */
    static std::unique_ptr<FOOTPRINT_EDITOR_TAB_CONTEXT> MakeUnsaved( std::unique_ptr<BOARD> aBoard );

    /**
     * De-duplication key for a placed board footprint, in a namespace disjoint from library keys.
     *
     * The leading control character cannot appear in a library nickname, so an instance key can never
     * collide with a library:name key.
     */
    static wxString MakeInstanceTabKey( const KIID& aSourceUuid )
    {
        return wxString( wxT( "\x01@fp:" ) ) + aSourceUuid.AsString();
    }

    /**
     * De-duplication key for an imported footprint, in a namespace disjoint from the library and
     * instance keys.
     */
    static wxString MakeUnsavedTabKey( const KIID& aSessionId )
    {
        return wxString( wxT( "\x01@import:" ) ) + aSessionId.AsString();
    }

    wxString GetTabKey() const override
    {
        switch( m_kind )
        {
        case KIND::BOARD_INSTANCE: return MakeInstanceTabKey( m_sourceUuid );
        case KIND::UNSAVED:        return MakeUnsavedTabKey( m_sessionId );
        default:                   return m_lib + wxT( ":" ) + m_name;
        }
    }

    wxString GetDisplayName() const override;

    /**
     * Give an imported footprint the library identity a save-as just assigned it.
     *
     * The key changes with the kind, so the caller must re-key the tab strip entry using the key it
     * captured before this call.
     */
    void PromoteToLibrary( const wxString& aLib, const wxString& aName );

    /**
     * True for a tab that is session-only and never persisted.
     */
    bool IsTransient() const { return m_kind != KIND::LIBRARY; }

    bool        IsFromBoard() const            { return m_kind == KIND::BOARD_INSTANCE; }
    bool        IsUnsaved() const              { return m_kind == KIND::UNSAVED; }
    const KIID& GetSourceUuid() const          { return m_sourceUuid; }
    const wxString& GetReference() const       { return m_reference; }

    /**
     * Editor-to-board UUID remap captured at load, used by SaveFootprintToBoard to restore the
     * original board item identities. Empty for a library tab.
     */
    std::map<KIID, KIID>& BoardFootprintUuids() { return m_boardFootprintUuids; }

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
    FOOTPRINT_EDITOR_TAB_CONTEXT( KIND aKind, std::unique_ptr<BOARD> aBoard );

    wxString                   m_lib;
    wxString                   m_name;
    std::unique_ptr<BOARD>     m_board;
    std::unique_ptr<FOOTPRINT> m_originalFootprintCopy;
    wxString                   m_footprintNameWhenLoaded;
    bool                       m_modified = false;

    KIND                       m_kind = KIND::LIBRARY;

    ///< Source board footprint UUID, used as the de-dup key and save-back target.
    KIID                       m_sourceUuid;

    ///< Identity of an unsaved import, which has no library:name pair to key on.
    KIID                       m_sessionId;

    ///< Reference designator of the source footprint, shown as the tab label.
    wxString                   m_reference;

    ///< Editor-to-board UUID remap used to save instance edits back to the original board items.
    std::map<KIID, KIID>       m_boardFootprintUuids;
};

#endif // FOOTPRINT_EDITOR_TAB_CONTEXT_H
