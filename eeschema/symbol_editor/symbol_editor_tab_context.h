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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SYMBOL_EDITOR_TAB_CONTEXT_H
#define SYMBOL_EDITOR_TAB_CONTEXT_H

#include <wx/string.h>
#include <wx/translation.h>

#include <kiid.h>
#include <widgets/editor_tab_context.h>

class LIB_SYMBOL;
class SCH_SCREEN;
class SYMBOL_BUFFER;


/**
 * One open symbol tab owning a working LIB_SYMBOL and screen lent to the frame while active.
 *
 * A tab is one of three kinds.  A library tab edits a buffered library symbol and is keyed by its
 * library:name pair and persisted across sessions.  An instance tab edits a symbol pulled from a
 * placed schematic instance (Ctrl-E); it owns a transient working symbol with no library buffer, is
 * keyed by the source instance UUID, and is session-only so it is never persisted.  An unsaved tab
 * edits a brand-new symbol with no library home yet; it is keyed by a session id and is promoted to
 * a library tab by the save that gives it an identity.
 */
class SYMBOL_EDITOR_TAB_CONTEXT : public EDITOR_TAB_CONTEXT
{
public:
    /**
     * What the tab edits, which decides its key, its label and whether it is persisted.
     */
    enum class KIND
    {
        LIBRARY,            ///< A library symbol, keyed library:name and persisted across sessions
        SCHEMATIC_INSTANCE, ///< A symbol pulled off a placed schematic symbol, session-only
        UNSAVED             ///< A brand-new symbol with no library home yet, session-only
    };


    /**
     * Construct a library tab over the buffer for aLibrary:aName.
     *
     * The context edits a private clone of the buffered symbol; the buffer keeps ownership of the
     * original.  A fresh empty screen is created since symbol geometry lives in the LIB_SYMBOL.
     */
    SYMBOL_EDITOR_TAB_CONTEXT( const wxString& aLib, const wxString& aName, SYMBOL_BUFFER* aBuffer );

    /**
     * Construct an unsaved tab over an already-built transient working symbol/screen.
     *
     * The context takes ownership of both objects, following the same frame-borrow contract as a
     * library tab.  There is no library buffer; the symbol has no library identity yet.
     */
    SYMBOL_EDITOR_TAB_CONTEXT( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen );

    /**
     * Construct an instance (schematic) tab over an already-built transient working symbol/screen.
     *
     * The context takes ownership of both objects, following the same frame-borrow contract as a
     * library tab.  There is no library buffer.  Save routing is driven by the stored source UUID and
     * reference, which the frame restores on activation.
     */
    SYMBOL_EDITOR_TAB_CONTEXT( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen,
                               const KIID& aSchematicSymbolUUID, const wxString& aReference );

    ~SYMBOL_EDITOR_TAB_CONTEXT() override;

    /**
     * De-duplication key for a library:symbol pair.
     */
    static wxString MakeTabKey( const wxString& aLib, const wxString& aName )
    {
        return aLib + wxT( ":" ) + aName;
    }

    /**
     * De-duplication key for a placed schematic instance, in a namespace disjoint from library keys.
     *
     * The leading control character cannot appear in a library nickname, so an instance key can never
     * collide with a library:name key.
     */
    static wxString MakeInstanceTabKey( const KIID& aSchematicSymbolUUID )
    {
        return wxString( wxT( "\x01@sym:" ) ) + aSchematicSymbolUUID.AsString();
    }

    /**
     * De-duplication key for an unsaved new symbol
     */
    static wxString MakeUnsavedTabKey( const KIID& aSessionId )
    {
        return wxString( wxT( "\x01@unsaved:" ) ) + aSessionId.AsString();
    }

    wxString GetTabKey() const override
    {
        switch( m_kind )
        {
        case KIND::SCHEMATIC_INSTANCE: return MakeInstanceTabKey( m_schematicSymbolUUID );
        case KIND::UNSAVED:            return MakeUnsavedTabKey( m_sessionId );
        default:                       return MakeTabKey( m_lib, m_name );
        }
    }

    wxString GetDisplayName( bool aShortForm = false ) const override
    {
        switch( m_kind )
        {
        case KIND::SCHEMATIC_INSTANCE:
            if( aShortForm )
                return m_reference;
            else
                return m_reference + wxS( " " ) + _( "[from schematic]" );

        case KIND::UNSAVED:
            return _( "<unnamed>" );

        default:
            return m_name;
        }
    }

    /**
     * True for a tab that is session-only and not persisted.
     */
    bool IsTransient() const { return m_kind != KIND::LIBRARY; }

    bool        IsFromSchematic() const          { return m_kind == KIND::SCHEMATIC_INSTANCE; }
    bool        IsUnsaved() const                { return m_kind == KIND::UNSAVED; }
    const KIID& GetSchematicSymbolUUID() const   { return m_schematicSymbolUUID; }
    const wxString& GetReference() const         { return m_reference; }

    /**
     * True when the working screen carries unsaved edits.
     */
    bool     IsModified() const override;

    const wxString& GetLibrary() const { return m_lib; }
    const wxString& GetName() const    { return m_name; }
    void            SetName( const wxString& aName ) { m_name = aName; }

    /**
     * Gives an unsaved new symbol a library identity
     */
    void PromoteToLibrary( const wxString& aLib, const wxString& aName );

    /**
     * Observe the working symbol/screen.  Valid whether active or detached.
     */
    LIB_SYMBOL* GetSymbol() const { return m_symbol; }
    SCH_SCREEN* GetScreen() const { return m_screen; }

    /**
     * Hand the working symbol/screen to the frame as the tab becomes active.
     *
     * The context stops deleting them until AdoptWorkingObjects takes ownership back on detach.
     */
    void ReleaseToFrame() { m_frameOwns = true; }

    /**
     * Take ownership back on detach, capturing whatever the frame's symbol now points at.
     */
    void AdoptWorkingObjects( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen )
    {
        m_symbol = aSymbol;
        m_screen = aScreen;
        m_frameOwns = false;
    }

    /**
     * Track the frame's new working symbol after undo/redo replaces it, while staying frame-owned.
     */
    void RefreshFrameOwnedObjects( LIB_SYMBOL* aSymbol, SCH_SCREEN* aScreen )
    {
        m_symbol = aSymbol;
        m_screen = aScreen;
    }

    int  GetUnit() const      { return m_unit; }
    void SetUnit( int aUnit ) { m_unit = aUnit; }

    int  GetBodyStyle() const           { return m_bodyStyle; }
    void SetBodyStyle( int aBodyStyle ) { m_bodyStyle = aBodyStyle; }

private:
    wxString    m_lib;
    wxString    m_name;
    LIB_SYMBOL* m_symbol;     ///< Working copy; owned by the context while detached.
    SCH_SCREEN* m_screen;     ///< Working screen; owned by the context while detached.
    bool        m_frameOwns;  ///< True while the tab is active and the frame owns the symbol/screen.
    int         m_unit;
    int         m_bodyStyle;

    KIND        m_kind = KIND::LIBRARY;

    /// Source instance UUID, used as both the de-dup key and the save-back target.
    KIID        m_schematicSymbolUUID;

    /// Reference designator of the source instance, shown as the tab label.
    wxString    m_reference;

    /// Identity of an unsaved new symbol, which has no library:name pair to key on.
    KIID        m_sessionId;
};

#endif // SYMBOL_EDITOR_TAB_CONTEXT_H
