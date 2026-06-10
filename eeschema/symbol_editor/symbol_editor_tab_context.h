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

#ifndef SYMBOL_EDITOR_TAB_CONTEXT_H
#define SYMBOL_EDITOR_TAB_CONTEXT_H

#include <wx/string.h>

#include <kiid.h>
#include <widgets/editor_tab_context.h>

class LIB_SYMBOL;
class SCH_SCREEN;
class SYMBOL_BUFFER;


/**
 * One open symbol tab owning a working LIB_SYMBOL and screen lent to the frame while active.
 *
 * A tab is one of two kinds.  A library tab edits a buffered library symbol and is keyed by its
 * library:name pair and persisted across sessions.  An instance tab edits a symbol pulled from a
 * placed schematic instance (Ctrl-E); it owns a transient working symbol with no library buffer, is
 * keyed by the source instance UUID, and is session-only so it is never persisted.
 */
class SYMBOL_EDITOR_TAB_CONTEXT : public EDITOR_TAB_CONTEXT
{
public:
    SYMBOL_EDITOR_TAB_CONTEXT( const wxString& aLib, const wxString& aName, SYMBOL_BUFFER* aBuffer );

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

    wxString GetTabKey() const override
    {
        return m_fromSchematic ? MakeInstanceTabKey( m_schematicSymbolUUID )
                               : MakeTabKey( m_lib, m_name );
    }

    wxString GetDisplayName() const override { return m_fromSchematic ? m_reference : m_name; }

    /**
     * True for an instance (schematic) tab, which is session-only and never persisted.
     */
    bool IsTransient() const { return m_fromSchematic; }

    bool        IsFromSchematic() const          { return m_fromSchematic; }
    const KIID& GetSchematicSymbolUUID() const   { return m_schematicSymbolUUID; }
    const wxString& GetReference() const         { return m_reference; }

    /**
     * True when the working screen carries unsaved edits.
     */
    bool     IsModified() const override;

    const wxString& GetLibrary() const { return m_lib; }
    const wxString& GetName() const    { return m_name; }

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

    ///< True for an instance tab edited in place from a placed schematic symbol.
    bool        m_fromSchematic;

    ///< Source instance UUID, used as both the de-dup key and the save-back target.
    KIID        m_schematicSymbolUUID;

    ///< Reference designator of the source instance, shown as the tab label.
    wxString    m_reference;
};

#endif // SYMBOL_EDITOR_TAB_CONTEXT_H
