/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LIB_EDIT_FRAME_H
#define LIB_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <lib_item.h>
#include <ee_collectors.h>
#include <core/optional.h>

class SCH_EDIT_FRAME;
class SYMBOL_LIB_TABLE;
class LIB_PART;
class LIB_ALIAS;
class LIB_FIELD;
class DIALOG_LIB_EDIT_TEXT;
class SYMBOL_TREE_PANE;
class LIB_ID;
class LIB_MANAGER;


/**
 * The symbol library editor main window.
 */
class LIB_EDIT_FRAME : public SCH_BASE_FRAME
{
    LIB_PART*          m_my_part;           // a part I own, it is not in any library, but a copy
                                            // could be.
    wxComboBox*        m_unitSelectBox;     // a ComboBox to select a unit to edit (if the part
                                            // has multiple units)
    SYMBOL_TREE_PANE*  m_treePane;          // component search tree widget
    LIB_MANAGER*       m_libMgr;            // manager taking care of temporary modificatoins

    // The unit number to edit and show
    int m_unit;

    // Show the normal shape ( m_convert <= 1 ) or the converted shape ( m_convert > 1 )
    int m_convert;

    // True to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded component has graphic items for converted shape
    // But under some circumstances (New component created) these tools must left enabled
    static bool m_showDeMorgan;

    static int m_textPinNumDefaultSize;     // The default pin num text size setting.
    static int m_textPinNameDefaultSize;    // The default  pin name text size setting.
    static int m_defaultPinLength;          //  Default pin length

    /// Default repeat offset for pins in repeat place pin
    int m_repeatPinStep;

    int m_defaultLibWidth;

public:
    /**
     * Set to true to synchronize pins at the same position when editing symbols with multiple
     * units or multiple body styles.  Deleting or moving pins will affect all pins at the same
     * location.
     * When units are interchangeable, synchronizing editing of pins is usually the best way,
     * because if units are interchangeable, it implies that all similar pins are at the same
     * location.
     * When units are not interchangeable, do not synchronize editing of pins, because each part
     * is specific, and there are no (or few) similar pins between units.
     *
     * Setting this to false allows editing each pin per part or body style regardless other
     * pins at the same location. This requires the user to open each part or body style to make
     * changes to the other pins at the same location.
     *
     * To know if others pins must be coupled when editing a pin, use SynchronizePins() instead
     * of m_syncPinEdit, because SynchronizePins() is more reliable (takes in account the fact
     * units are interchangeable, there are more than one unit).
     */
    bool          m_SyncPinEdit;

    /** Convert of the item currently being drawn. */
    bool          m_DrawSpecificConvert;

    /**
     * Specify which component parts the current draw item applies to.
     *
     * If true, the item being drawn or edited applies only to the selected part.  Otherwise
     * it applies to all parts in the component.
     */
    bool          m_DrawSpecificUnit;

    static int    g_LastTextSize;
    static double g_LastTextAngle;
    static FILL_T g_LastFillStyle;
    static int    g_LastLineWidth;

public:
    LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~LIB_EDIT_FRAME() override;

    /**
     * switches currently used canvas ( Cairo / OpenGL).
     */
    void SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) override;

    /**
     * Check if any pending libraries have been modified.
     *
     * This only checks for modified libraries.  If a new symbol was created and modified
     * and no libraries have been modified, the return value will be false.
     *
     * @return True if there are any pending library modifications.
     */
    bool HasLibModifications() const;

    /** The nickname of the current library being edited and empty string if none. */
    wxString GetCurLib() const;

    /** Sets the current library nickname and returns the old library nickname. */
    wxString SetCurLib( const wxString& aLibNickname );

    /**
     * Return the LIB_ID of the library or symbol selected in the symbol tree.
     */
    LIB_ID GetTreeLIBID( int* aUnit = nullptr ) const;

    /**
     * Return the current part being edited or NULL if none selected.
     *
     * This is a LIB_PART that I own, it is at best a copy of one in a library.
     */
    LIB_PART* GetCurPart() const { return m_my_part; }

    /**
     * Take ownership of aPart and notes that it is the one currently being edited.
     */
    void SetCurPart( LIB_PART* aPart );

    static int GetPinNumDefaultSize() { return m_textPinNumDefaultSize; }
    static void SetPinNumDefaultSize( int aSize ) { m_textPinNumDefaultSize = aSize; }

    static int GetPinNameDefaultSize() { return m_textPinNameDefaultSize; }
    static void SetPinNameDefaultSize( int aSize ) { m_textPinNameDefaultSize = aSize; }

    static int GetDefaultPinLength() { return m_defaultPinLength; }
    static void SetDefaultPinLength( int aLength ) { m_defaultPinLength = aLength; }

    /**
     * @return the increment value of the position of a pin for the pin repeat command
     */
    int GetRepeatPinStep() const { return m_repeatPinStep; }
    void SetRepeatPinStep( int aStep) { m_repeatPinStep = aStep; }

    void ReCreateMenuBar() override;

    // See comments for m_SyncPinEdit.
    bool SynchronizePins();

    void OnImportBody( wxCommandEvent& aEvent );
    void OnExportBody( wxCommandEvent& aEvent );

    /**
     * Creates or adds an existing library to the symbol library table.
     */
    bool AddLibraryFile( bool aCreateNew );

    /**
     * Creates a new part in the selected library.
     */
    void CreateNewPart();

    void ImportPart();
    void ExportPart();

    /**
     * Saves the selected part or library.
     */
    void Save();

    /**
     * Saves the selected part or library to a new name and/or location.
     */
    void SaveAs();

    /**
     * Saves all modified parts and libraries.
     */
    void SaveAll();

    /**
     * Reverts unsaved changes in a part, restoring to the last saved state.
     */
    void Revert( bool aConfirm = true );
    void RevertAll();

    void DeletePartFromLibrary();

    void CopyPartToClipboard();

    void LoadPart( const wxString& aLibrary, const wxString& aPart, int Unit );

    /**
     * Inserts a duplicate part.  If aFromClipboard is true then action is a paste.
     */
    void DuplicatePart( bool aFromClipboard );

    void OnSelectUnit( wxCommandEvent& event );

    void OnToggleSearchTree( wxCommandEvent& event );

    bool IsSearchTreeShown();
    void FreezeSearchTree();
    void ThawSearchTree();

    void OnUpdatePartNumber( wxUpdateUIEvent& event );

    void UpdateAfterSymbolProperties( wxString* aOldName, wxArrayString* aOldAliases );
    void RebuildSymbolUnitsList();

    void OnCloseWindow( wxCloseEvent& Event );
    void   OnExitKiCad( wxCommandEvent& event );
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override;
    double BestZoom() override;         // Returns the best zoom

    void LoadSettings( wxConfigBase* aCfg ) override;
    void SaveSettings( wxConfigBase* aCfg ) override;

    /**
     * Trigger the wxCloseEvent, which is handled by the function given to EVT_CLOSE() macro:
     * <p>
     * EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
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
    void SetUnit( int aUnit ) { m_unit = aUnit; }

    int GetConvert() const { return m_convert; }
    void SetConvert( int aConvert ) { m_convert = aConvert; }

    bool GetShowDeMorgan() const { return m_showDeMorgan; }
    void SetShowDeMorgan( bool show ) { m_showDeMorgan = show; }

    void ClearMsgPanel() override { DisplayCmpDoc(); }

private:
    // Sets up the tool framework
    void setupTools();

    void savePartAs();

    /**
     * Saves the changes to the current library.
     *
     * A backup file of the current library is saved with the .bak extension before the
     * changes made to the library are saved.
     * @param aLibrary is the library name.
     * @param aNewFile Ask for a new file name to save the library.
     * @return True if the library was successfully saved.
     */
    bool saveLibrary( const wxString& aLibrary, bool aNewFile );

    /**
     * Updates the main window title bar with the current library name and read only status
     * of the library.
     */
    void updateTitle();

    /**
     * Set the current active library to \a aLibrary.
     *
     * @param aLibrary the nickname of the library in the symbol library table.  If empty,
     *                 display list of available libraries to select from.
     */
    void SelectActiveLibrary( const wxString& aLibrary = wxEmptyString );

    /**
     * Dispaly a list of loaded libraries in the symbol library and allows the user to select
     * a library.
     *
     * This list is sorted, with the library cache always at end of the list
     *
     * @return the library nickname used in the symbol library table.
     */
    wxString SelectLibraryFromList();

    /**
     * Loads a symbol from the current active library, optionally setting the selected unit
     * and convert.
     *
     * @param aAliasName The symbol alias name to load from the current library.
     * @param aUnit Unit to be selected
     * @param aConvert Convert to be selected
     * @return true if the symbol loaded correctly.
     */
    bool LoadComponentFromCurrentLib( const wxString& aAliasName, int aUnit = 0, int aConvert = 0 );

    /**
     * Create a copy of \a aLibEntry into memory.
     *
     * @param aLibEntry A pointer to the LIB_ALIAS object to an already loaded.
     * @param aLibrary the path to the library file that \a aLibEntry was loaded from.  This is
     *                 for error messaging purposes only.
     * @param aUnit the initial unit to show.
     * @param aConvert the initial DeMorgan variant to show.
     * @return True if a copy of \a aLibEntry was successfully copied.
     */
    bool LoadOneLibraryPartAux( LIB_ALIAS* aLibEntry, const wxString& aLibrary, int aUnit,
                                int aConvert );

    /**
     * Display the documentation of the selected component.
     */
    void DisplayCmpDoc();

    // General editing
public:
    /**
     * Create a copy of the current component, and save it in the undo list.
     *
     * Because a component in library editor does not have a lot of primitives,
     * the full data is duplicated. It is not worth to try to optimize this save function.
     */
    void SaveCopyInUndoList( EDA_ITEM* ItemToCopy, UNDO_REDO_T undoType = UR_LIBEDIT,
                             bool aAppend = false );

    void GetComponentFromUndoList();
    void GetComponentFromRedoList();

    void RollbackPartFromUndo();

private:
    /**
     * Read a component symbol file (*.sym ) and add graphic items to the current component.
     *
     * A symbol file *.sym has the same format as a library, and contains only one symbol.
     */
    void LoadOneSymbol();

    /**
     * Saves the current symbol to a symbol file.
     *
     * The symbol file format is similar to the standard component library file format, but
     * there is only one symbol.  Invisible pins are not saved.
     */
    void SaveOneSymbol();

    void refreshSchematic();

public:
    /**
     * Selects the currently active library and loads the symbol from \a aLibId.
     *
     * @param aLibId is the #LIB_ID of the symbol to select.
     * @param aUnit the unit to show
     * @param aConvert the DeMorgan variant to show
     * @return true if the symbol defined by \a aLibId was loaded.
     */
    bool LoadComponentAndSelectLib( const LIB_ID& aLibId, int aUnit, int aConvert );

    /**
     * Print a page
     *
     * @param aDC = wxDC given by the calling print function
     */
    void PrintPage( wxDC* aDC ) override;

    /**
     * Creates the SVG print file for the current edited component.
     */
    void SVG_PlotComponent( const wxString& aFullFileName );

    /**
     * Synchronize the library manager to the symbol library table, and then the symbol tree
     * to the library manager.  Optionally displays a progress dialog.
     */
    void SyncLibraries( bool aShowProgress );

    /**
     * Allows Libedit to install its preferences panel into the preferences dialog.
     */
    void InstallPreferences( PAGED_DIALOG* aParent, PANEL_HOTKEYS_EDITOR* aHotkeysPanel ) override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged( bool aEnvVarsChanged ) override;

    void ShowChangedLanguage() override;

    void SyncToolbars() override;

    void SetScreen( BASE_SCREEN* aScreen ) override;

    const BOX2I GetDocumentExtents() const override;

    void RebuildView();

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

    ///> Restores the empty editor screen, without any part or library selected.
    void emptyScreen();

private:
    ///> Helper screen used when no part is loaded
    SCH_SCREEN* m_dummyScreen;

    /**
     * Displays a dialog asking the user to select a symbol library table.
     * @param aOptional if set the Cancel button will be relabelled "Skip".
     * @return Pointer to the selected symbol library table or nullptr if cancelled.
     */
    SYMBOL_LIB_TABLE* selectSymLibTable( bool aOptional = false );

    ///> Creates a backup copy of a file with requested extension
    bool backupFile( const wxFileName& aOriginalFile, const wxString& aBackupExt );

    ///> Returns currently edited part.
    LIB_PART* getTargetPart() const;

    ///> Returns either the part selected in the component tree, if context menu is active
    ///> or the currently modified part.
    LIB_ID getTargetLibId() const;

    ///> Returns either the library selected in the component tree, if context menu is active
    ///> or the library that is currently modified.
    wxString getTargetLib() const;

    /* Returns true when the operation has succeded (all requested libraries have been saved or
     * none was selected and confirmed by OK).
     * @param aRequireConfirmation when true, the user must be asked to confirm.
     */
    bool saveAllLibraries( bool aRequireConfirmation );

    ///> Saves the current part.
    bool saveCurrentPart();

    ///> Stores the currently modified part in the library manager buffer.
    void storeCurrentPart();

    ///> Returns true if \a aLibId is an alias for the editor screen part.
    bool isCurrentPart( const LIB_ID& aLibId ) const;

    ///> Renames LIB_PART aliases to avoid conflicts before adding a component to a library
    void fixDuplicateAliases( LIB_PART* aPart, const wxString& aLibrary );

    DECLARE_EVENT_TABLE()
};

#endif  // LIB_EDIT_FRAME_H
