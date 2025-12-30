/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef SYMBOL_EDIT_FRAME_H
#define SYMBOL_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <symbol_tree_pane.h>
#include <optional>

class SCH_EDIT_FRAME;
class LIB_SYMBOL;
class LIB_TREE_NODE;
class LIB_ID;
class LIB_SYMBOL_LIBRARY_MANAGER;
class SYMBOL_EDITOR_SETTINGS;
class EDA_LIST_DIALOG;


#define UNITS_ALL _HKI( "ALL" )
#define DEMORGAN_ALL _HKI( "ALL" )
#define DEMORGAN_STD _HKI( "Standard" )
#define DEMORGAN_ALT _HKI( "Alternate" )


/**
 * The symbol library editor main window.
 */
class SYMBOL_EDIT_FRAME : public SCH_BASE_FRAME
{
public:
    SYMBOL_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~SYMBOL_EDIT_FRAME() override;

    std::unique_ptr<GRID_HELPER> MakeGridHelper() override;

    /**
     * Switch currently used canvas ( Cairo / OpenGL).
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Get if any symbols or libraries have been modified but not saved.
     *
     * @return true if the any changes have not been saved
     */
    bool IsContentModified() const override;

    /**
     * Check if any pending libraries have been modified.
     *
     * This only checks for modified libraries.  If a new symbol was created and modified
     * and no libraries have been modified, the return value will be false.
     *
     * @return True if there are any pending library modifications.
     */
    bool HasLibModifications() const;

    bool CanCloseSymbolFromSchematic( bool doClose );

    /**
     * The nickname of the current library being edited and empty string if none.
     */
    wxString GetCurLib() const;

    /**
     * Set the current library nickname and returns the old library nickname.
     */
    wxString SetCurLib( const wxString& aLibNickname );

    LIB_TREE* GetLibTree() const override { return m_treePane->GetLibTree(); }

    /**
     * Return the LIB_ID of the library or symbol selected in the symbol tree.
     */
    LIB_ID GetTreeLIBID( int* aUnit = nullptr ) const;

    int GetTreeSelectionCount() const;

    int GetTreeLIBIDs( std::vector<LIB_ID>& aSelection ) const;

    /**
     * Return the current symbol being edited or NULL if none selected.
     *
     * This is a LIB_SYMBOL that I own, it is at best a copy of one in a library.
     */
    LIB_SYMBOL* GetCurSymbol() const { return m_symbol; }

    /**
     * Take ownership of aSymbol and notes that it is the one currently being edited.
     */
    void SetCurSymbol( LIB_SYMBOL* aSymbol, bool aUpdateZoom );

    LIB_SYMBOL_LIBRARY_MANAGER& GetLibManager();

    SELECTION& GetCurrentSelection() override;

    // See comments for m_SyncPinEdit.
    bool SynchronizePins();

    /**
     * Create or add an existing library to the symbol library table.
     */
    wxString AddLibraryFile( bool aCreateNew );

    /**
     * Add a library dropped file to the symbol library table.
     */
    void DdAddLibrary( wxString aLibFile );

    /**
     * Create a new symbol in the selected library.
     *
     * @param newName is the name of the symbol to derive the new symbol from or empty
     *                     to create a new root symbol.
     */
    void CreateNewSymbol( const wxString& newName = wxEmptyString );

    void ImportSymbol();
    void ExportSymbol();

    /**
     * Save the selected symbol or library.
     */
    void Save();

    /**
     * Save the currently selected symbol to a new name and/or location.
     */
    void SaveSymbolCopyAs( bool aOpenCopy );

    /**
     * Save the currently selected library to a new file.
     */
    void SaveLibraryAs();

    /**
     * Save all modified symbols and libraries.
     */
    void SaveAll();

    /**
     * Revert unsaved changes in a symbol, restoring to the last saved state.
     */
    void Revert( bool aConfirm = true );
    void RevertAll();

    void DeleteSymbolFromLibrary();

    void CopySymbolToClipboard();

    void LoadSymbol( const wxString& aLibrary, const wxString& aSymbol, int Unit );

    /**
     * Insert a duplicate symbol.
     *
     * If \a aFromClipboard is true then action is a paste.
     */
    void DuplicateSymbol( bool aFromClipboard );

    void OnSelectUnit( wxCommandEvent& event );
    void OnSelectBodyStyle( wxCommandEvent& event );

    void ToggleProperties() override;

    void ToggleLibraryTree() override;
    bool IsLibraryTreeShown() const override;
    void FocusLibraryTreeInput() override;
    void FreezeLibraryTree();
    void ThawLibraryTree();

    void OnUpdateUnitNumber( wxUpdateUIEvent& event );
    void OnUpdateBodyStyle( wxUpdateUIEvent& event );

    void UpdateAfterSymbolProperties( wxString* aOldName = nullptr );
    void RebuildSymbolUnitAndBodyStyleLists();

    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override;
    void doCloseWindow() override;
    void OnExitKiCad( wxCommandEvent& event );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    SYMBOL_EDITOR_SETTINGS* GetSettings() const
    {
        return m_settings;
    }

    APP_SETTINGS_BASE* config() const override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    /**
     * Trigger the wxCloseEvent, which is handled by the function given to EVT_CLOSE() macro:
     * <p>
     * EVT_CLOSE( SYMBOL_EDIT_FRAME::OnCloseWindow )
     * </p>
     */
    void CloseWindow( wxCommandEvent& event )
    {
        // Generate a wxCloseEvent
        Close( false );
    }

    /**
     * Must be called after a schematic change in order to set the "modify" flag of the
     * current symbol.
     */
    void OnModify() override;

    int GetUnit() const { return m_unit; }
    void SetUnit( int aUnit );

    int  GetBodyStyle() const { return m_bodyStyle; }
    void SetBodyStyle( int aBodyStyle );

    bool GetShowInvisibleFields();
    bool GetShowInvisiblePins();

    void ClearMsgPanel() override
    {
        UpdateSymbolMsgPanelInfo();
    }

    void UpdateMsgPanel() override
    {
        UpdateSymbolMsgPanelInfo();
    }

    /**
     * Update the main window title bar with the current library name and read only status
     * of the library.
     */
    void UpdateTitle();

    bool IsSymbolFromSchematic() const
    {
        // If we've already vetted closing this window, then we have no symbol anymore
        if( m_isClosing )
            return false;

        return m_isSymbolFromSchematic;
    }

    bool IsSymbolFromLegacyLibrary() const;

    /**
     * Display the documentation of the selected symbol.
     */
    void UpdateSymbolMsgPanelInfo();

    // General editing
    /**
     * Create a copy of the current symbol, and save it in the undo list.
     *
     * Because a symbol in library editor does not have a lot of primitives, the full data is
     * duplicated. It is not worth to try to optimize this save function.
     */
    void SaveCopyInUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbol,
                             UNDO_REDO aUndoType = UNDO_REDO::LIBEDIT );

    void PushSymbolToUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbolCopy,
                               UNDO_REDO aUndoType = UNDO_REDO::LIBEDIT );

    void GetSymbolFromUndoList();
    void GetSymbolFromRedoList();

    /**
     * Free the undo or redo list from \a aList element.
     *
     * - Wrappers are deleted.
     * - data pointed by wrappers are deleted if not in use in schematic
     *   i.e. when they are copy of a schematic item or they are no more in use (DELETED)
     *
     * @param whichList = the UNDO_REDO_CONTAINER to clear
     * @param aItemCount = the count of items to remove. < 0 for all items
     * items are removed from the beginning of the list.
     * So this function can be called to remove old commands
     */
    void ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount = -1 ) override;

    /**
     * Select the currently active library and loads the symbol from \a aLibId.
     *
     * @param aLibId is the #LIB_ID of the symbol to select.
     * @param aUnit the unit to show
     * @param aBodyStyle the DeMorgan variant to show
     * @return true if the symbol defined by \a aLibId was loaded.
     */
    bool LoadSymbol( const LIB_ID& aLibId, int aUnit, int aBodyStyle );

    /**
     * Create the SVG print file for the current edited symbol.
     * @param aFullFileName is the full filename
     * @param aOffset is a plot offset, in iu
     */
    void SVGPlotSymbol( const wxString& aFullFileName, const VECTOR2I& aOffset );

    /**
     * Synchronize the library manager to the symbol library table, and then the symbol tree
     * to the library manager.  Optionally displays a progress dialog.
     */
    void SyncLibraries( bool aShowProgress, bool aPreloadCancelled = false,
                        const wxString& aForceRefresh = wxEmptyString );

    /**
     * Redisplay the library tree.  Used after changing modified states, descriptions, etc.
     */
    void RefreshLibraryTree();

    /**
     * Update a symbol node in the library tree.
     */
    void UpdateLibraryTree( const wxDataViewItem& aTreeItem, LIB_SYMBOL* aSymbol );

    /**
     * Return either the symbol selected in the symbol tree (if context menu is active) or the
     * symbol on the editor canvas.
     */
    LIB_ID GetTargetLibId() const override;

    /**
     * @return a list of selected items in the symbol tree
     */
    std::vector<LIB_ID> GetSelectedLibIds() const;

    void FocusOnLibId( const LIB_ID& aLibID );

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( int aFlags ) override;

    void ShowChangedLanguage() override;

    void SetScreen( BASE_SCREEN* aScreen ) override;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    void RebuildView();

    void UpdateItem( EDA_ITEM* aItem, bool isAddOrDelete = false,
                     bool aUpdateRtree = false ) override;

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    void FocusOnItem( EDA_ITEM* aItem, bool aAllowScroll = true ) override;

    /**
     * Load a symbol from the schematic to edit in place.
     *
     * @param aSymbol the symbol to edit.
     */
    void LoadSymbolFromSchematic( SCH_SYMBOL* aSymbol );

    /**
     * Test if a symbol is loaded and can be edited.
     *
     * The following conditions are required for a symbol to be editable:
     *  - The symbol must selected from either a library or the schematic.
     *  - The symbol must not be from a legacy library.
     *
     * Note that many things are not editable in a non-root symbol (ie: an alias), but others
     * are so this routine no longer returns false for an alias.
     */
    bool IsSymbolEditable() const;

    bool IsSymbolAlias() const;

    ///< Return true if \a aLibId is an alias for the editor screen symbol.
    bool IsCurrentSymbol( const LIB_ID& aLibId ) const;

    ///< Restore the empty editor screen, without any symbol or library selected.
    void emptyScreen();

    void ClearToolbarControl( int aId ) override;

protected:
    void configureToolbars() override;
    void setupUIConditions() override;

    void doReCreateMenuBar() override;

    void updateSelectionFilterVisbility() override;

private:
    // Set up the tool framework
    void setupTools();

    void saveSymbolCopyAs( bool aOpenCopy );

    /**
     * Save the changes to the current library.
     *
     * A backup file of the current library is saved with the .bak extension before the
     * changes made to the library are saved.
     *
     * @param aLibrary is the library name.
     * @param aNewFile Ask for a new file name to save the library.
     * @return True if the library was successfully saved.
     */
    bool saveLibrary( const wxString& aLibrary, bool aNewFile );

    /**
     * Set the current active library to \a aLibrary.
     *
     * @param aLibrary the nickname of the library in the symbol library table.  If empty,
     *                 display list of available libraries to select from.
     */
    void SelectActiveLibrary( const wxString& aLibrary = wxEmptyString );

    /**
     * Load a symbol from the current active library, optionally setting the selected unit
     * and convert.
     *
     * @param aSymbolName The symbol alias name to load from the current library.
     * @param aUnit Unit to be selected
     * @param aBodyStyle Convert to be selected
     * @return true if the symbol loaded correctly.
     */
    bool LoadSymbolFromCurrentLib( const wxString& aSymbolName, int aUnit = 0, int aBodyStyle = 0 );

    /**
     * Create a copy of \a aLibEntry into memory.
     *
     * @param aLibEntry A pointer to the LIB_SYMBOL object to an already loaded symbol.
     * @param aLibrary the path to the library file that \a aLibEntry was loaded from.  This is
     *                 for error messaging purposes only.
     * @param aUnit the initial unit to show.
     * @param aBodyStyle the initial DeMorgan variant to show.
     * @return True if a copy of \a aLibEntry was successfully copied.
     */
    bool LoadOneLibrarySymbolAux( LIB_SYMBOL* aLibEntry, const wxString& aLibrary, int aUnit,
                                  int aBodyStyle );

    ///< Create a backup copy of a file with requested extension.
    bool backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt );

    ///< Return currently edited symbol.
    LIB_SYMBOL* getTargetSymbol() const;

    ///< Return either the library selected in the symbol tree, if context menu is active or
    ///< the library that is currently modified.
    wxString getTargetLib() const;

    void centerItemIdleHandler( wxIdleEvent& aEvent );

    /*
     * Return true when the operation has succeeded (all requested libraries have been saved
     * or none was selected and confirmed by OK).
     *
     * @param aRequireConfirmation when true, the user must be asked to confirm.
     */
    bool saveAllLibraries( bool aRequireConfirmation );

    ///< Save the current symbol.
    bool saveCurrentSymbol();

    ///< Store the currently modified symbol in the library manager buffer.
    void storeCurrentSymbol();

    ///< Rename LIB_SYMBOL aliases to avoid conflicts before adding a symbol to a library.
    void ensureUniqueName( LIB_SYMBOL* aSymbol, const wxString& aLibrary );

    /**
     * Add \a aLibFile to the symbol library table defined by \a aScope.
     *
     * @note The library defined by \a aLibFile must be a KiCad (s-expression) library.
     *
     * @param aLibFile is the full path and file name of the symbol library to add to the table.
     * @param aScope defines if \a aLibFile is added to the global or project library table.
     * @return true if successful or false if a failure occurs.
     */
    bool addLibTableEntry( const wxString& aLibFile,
                           LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::GLOBAL );

    /**
     * Replace the file path of the symbol library table entry \a aLibNickname with \a aLibFile.
     *
     * @note The library defined by \a aLibFile must be a KiCad (s-expression) library.
     *
     * @param aLibNickmane is the nickname of an existing library table entry.
     * @param aLibFile is the full path and file name of the symbol library to replace in the
     *                 table.
     * @return true if successful or false if a failure occurs.
     */
    bool replaceLibTableEntry( const wxString& aLibNickname, const wxString& aLibFile );

    DECLARE_EVENT_TABLE()

public:
    /**
     * Set to true to synchronize pins at the same position when editing symbols with multiple
     * units or multiple body styles.  Deleting or moving pins will affect all pins at the same
     * location.
     * When units are interchangeable, synchronizing editing of pins is usually the best way,
     * because if units are interchangeable, it implies that all similar pins are at the same
     * location.
     * When units are not interchangeable, do not synchronize editing of pins, because each symbol
     * is specific, and there are no (or few) similar pins between units.
     *
     * Setting this to false allows editing each pin per symbol or body style regardless other
     * pins at the same location. This requires the user to open each symbol or body style to make
     * changes to the other pins at the same location.
     *
     * To know if others pins must be coupled when editing a pin, use SynchronizePins() instead
     * of m_syncPinEdit, because SynchronizePins() is more reliable (takes in account the fact
     * units are interchangeable, there are more than one unit).
     *
     * @todo Determine why this member variable is public when all the rest are private and
     *       either make it private or document why it needs to be public.
     */
    bool          m_SyncPinEdit;

private:
    ///< Helper screen used when no symbol is loaded
    SCH_SCREEN*         m_dummyScreen;

    LIB_SYMBOL*         m_symbol;                // a symbol I own, it is not in any library, but a copy could be.
    wxComboBox*         m_unitSelectBox;         // a ComboBox to select a unit to edit (if the
                                                 //   symbol has multiple units)
    wxComboBox*         m_bodyStyleSelectBox;    // a ComboBox to select a body style to edit (if the symbol has
                                                 //   multiple body styles)
    SYMBOL_TREE_PANE*           m_treePane;      // symbol search tree widget
    LIB_SYMBOL_LIBRARY_MANAGER* m_libMgr;        // manager taking care of temporary modifications
    SYMBOL_EDITOR_SETTINGS*     m_settings;      // Handle to the settings

    LIB_ID                      m_centerItemOnIdle;

    // The unit number to edit and show
    int         m_unit;

    // Show the normal shape (m_bodyStyle <= 1) or the DeMorgan converted shape (m_bodyStyle > 1)
    int         m_bodyStyle;

    ///< Flag if the symbol being edited was loaded directly from a schematic.
    bool        m_isSymbolFromSchematic;
    KIID        m_schematicSymbolUUID;

     ///< RefDes of the symbol (only valid if symbol was loaded from schematic)
    wxString    m_reference;

    // True to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded symbol has graphic items for converted shape
    // But under some circumstances (New symbol created) these tools must left enabled
    static bool m_showDeMorgan;
};

#endif  // SYMBOL_EDIT_FRAME_H
