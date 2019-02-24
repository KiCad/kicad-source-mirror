/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file lib_edit_frame.h
 * @brief Definition of class LIB_EDIT_FRAME
 */

#ifndef LIB_EDIT_FRAME_H
#define LIB_EDIT_FRAME_H

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <lib_draw_item.h>
#include <lib_collectors.h>
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
    LIB_PART*          m_my_part;              ///< a part I own, it is not in any library, but a copy could be.
    LIB_PART*          m_tempCopyComponent;    ///< temp copy of a part during edit, I own it here.
    LIB_COLLECTOR      m_collectedItems;       ///< Used for hit testing.
    wxComboBox*        m_partSelectBox;        ///< a Box to select a part to edit (if any)
    SYMBOL_TREE_PANE*  m_treePane;             ///< component search tree widget
    LIB_MANAGER*       m_libMgr;               ///< manager taking care of temporary modificatoins

    // Copy/cut/paste buffer to move parts between libraries
    std::unique_ptr<LIB_PART> m_copiedPart;

    /** Convert of the item currently being drawn. */
    bool m_drawSpecificConvert;

    /**
     * Specify which component parts the current draw item applies to.
     *
     * If true, the item being drawn or edited applies only to the selected
     * part.  Otherwise it applies to all parts in the component.
     */
    bool m_drawSpecificUnit;

    /**
     * Set to true to not synchronize pins at the same position when editing
     * symbols with multiple units or multiple body styles.
     * Therefore deleting, moving pins are made for all pins at the same location
     * When units are interchangeable, synchronizing editing of pins is usually
     * the best way, because if units are interchangeable, it imply all similar
     * pins are on the same location.
     * When units are non interchangeable, do not synchronize editing of pins, because
     * each part is specific, and there are no similar pins between units.
     *
     * Setting this to false allows editing each pin per part or body style
     * regardless other pins at the same location.
     * This requires the user to open each part or body style to make changes
     * to the other pins at the same location.
     * To know if others pins must be coupled when editing a pin, use
     * SynchronizePins() instead of m_syncPinEdit, because SynchronizePins()
     * is more reliable (takes in account the fact units are interchangeable,
     * there are more than one unit).
     */
    bool m_syncPinEdit;

    /**
     * the option to show the pin electrical name in the component editor
     */
    bool m_showPinElectricalTypeName;

    /** The current draw or edit graphic item fill style. */
    static FILL_T m_drawFillStyle;

    /** Default line width for drawing or editing graphic items. */
    static int m_drawLineWidth;

    static LIB_ITEM*    m_lastDrawItem;

    // The unit number to edit and show
    static int m_unit;

    // Show the normal shape ( m_convert <= 1 ) or the converted shape
    // ( m_convert > 1 )
    static int m_convert;

    // true to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded component has
    // Graphic items for converted shape
    // But under some circumstances (New component created)
    // these tools must left enabled
    static bool m_showDeMorgan;

    /// The current text size setting.
    static int m_textSize;

    /// Current text angle setting.
    static double m_current_text_angle;

    /// The default pin num text size setting.
    static int m_textPinNumDefaultSize;

    /// The default  pin name text size setting.
    static int m_textPinNameDefaultSize;

    ///  Default pin length
    static int m_defaultPinLength;

    /// Default repeat offset for pins in repeat place pin
    int m_repeatPinStep;

    int m_defaultLibWidth;

    friend class DIALOG_LIB_EDIT_TEXT;

    LIB_ITEM* locateItem( const wxPoint& aPosition, const KICAD_T aFilterList[] );

public:

    LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~LIB_EDIT_FRAME();

    /**
     * switches currently used canvas ( Cairo / OpenGL).
     */
    void OnSwitchCanvas( wxCommandEvent& aEvent ) override;

    /** The nickname of the current library being edited and empty string if none. */
    wxString GetCurLib() const;

    /** Sets the current library nickname and returns the old library nickname. */
    wxString SetCurLib( const wxString& aLibNickname );

    /**
     * Return the current part being edited or NULL if none selected.
     *
     * This is a LIB_PART that I own, it is at best a copy of one in a library.
     */
    LIB_PART* GetCurPart() const
    {
        return m_my_part;
    }

    /**
     * Take ownership of aPart and notes that it is the one currently being edited.
     */
    void SetCurPart( LIB_PART* aPart );

    /** @return the default pin num text size.
     */
    static int GetPinNumDefaultSize() { return m_textPinNumDefaultSize; }
    static void SetPinNumDefaultSize( int aSize ) { m_textPinNumDefaultSize = aSize; }

    /** @return The default pin name text size setting.
     */
    static int GetPinNameDefaultSize() { return m_textPinNameDefaultSize; }
    static void SetPinNameDefaultSize( int aSize ) { m_textPinNameDefaultSize = aSize; }

    /** @return The default pin len setting.
     */
    static int GetDefaultPinLength() { return m_defaultPinLength; }

    /** Set the default pin len.
     */
    static void SetDefaultPinLength( int aLength ) { m_defaultPinLength = aLength; }

    /**
     * @return the increment value of the position of a pin
     * for the pin repeat command
     */
    int GetRepeatPinStep() const { return m_repeatPinStep; }

    /**
     * Sets the repeat step value for pins repeat command
     * @param aStep the increment value of the position of an item
     * for the repeat command
     */
    void SetRepeatPinStep( int aStep) { m_repeatPinStep = aStep; }


    void ReCreateMenuBar() override;

    void InstallConfigFrame( wxCommandEvent& event );
    void OnPreferencesOptions( wxCommandEvent& event );
    void Process_Config( wxCommandEvent& event );

    /**
     * Pin editing (add, delete, move...) can be synchronized between units
     * when units are interchangeable because in this case similar pins are expected
     * at the same location
     * @return true if the edit pins separately option is false and the current symbol
     * has multiple interchengeable units.
     * Otherwise return false.
     */
    bool SynchronizePins();

    /**
     * Plot the current symbol in SVG or PNG format.
     */
    void OnPlotCurrentComponent( wxCommandEvent& event );
    void Process_Special_Functions( wxCommandEvent& event );
    void OnSelectTool( wxCommandEvent& aEvent );

    /**
     * Creates a new library. The library is added to the project libraries table.
     */
    void OnCreateNewLibrary( wxCommandEvent& aEvent )
    {
        addLibraryFile( true );
    }

    /**
     * Adds an existing library. The library is added to the project libraries table.
     */
    void OnAddLibrary( wxCommandEvent& aEvent )
    {
        addLibraryFile( false );
    }

    /**
     * Creates a new part in the selected library.
     */
    void OnCreateNewPart( wxCommandEvent& aEvent );

    /**
     * Opens the selected part for editing.
     */
    void OnEditPart( wxCommandEvent& aEvent );

    /**
     * Routine to read one part.
     * The format is that of libraries, but it loads only 1 component.
     * Or 1 component if there are several.
     * If the first component is an alias, it will load the corresponding root.
     */
    void OnImportPart( wxCommandEvent& event );

    /**
     * Creates a new library and backup the current component in this library or exports
     * the component of the current library.
     */
    void OnExportPart( wxCommandEvent& event );

    /**
     * Saves the selected part or library.
     */
    void OnSave( wxCommandEvent& aEvent );

    /**
     * Saves the selected part or library to a new name and/or location.
     */
    void OnSaveAs( wxCommandEvent& aEvent );

    /**
     * Saves all modified parts and libraries.
     */
    void OnSaveAll( wxCommandEvent& aEvent );

    /**
     * Reverts unsaved changes in a part, restoring to the last saved state.
     */
    void OnRevert( wxCommandEvent& aEvent );

    /**
     * Removes a part from the working copy of a library.
     */
    void OnRemovePart( wxCommandEvent& aEvent );

    void OnDuplicatePart( wxCommandEvent& aEvent );
    void OnCopyCutPart( wxCommandEvent& aEvent );
    void OnPasteDuplicatePart( wxCommandEvent& aEvent );

    void OnSelectPart( wxCommandEvent& event );

    /**
     * From Option toolbar: option to show the electrical pin type name
     */
    void OnShowElectricalType( wxCommandEvent& event );

    void OnToggleSearchTree( wxCommandEvent& event );

    void OnEditSymbolLibTable( wxCommandEvent& aEvent ) override;

    bool IsSearchTreeShown();
    void ClearSearchTreeSelection();

    void OnEditComponentProperties( wxCommandEvent& event );

    void OnViewEntryDoc( wxCommandEvent& event );
    void OnCheckComponent( wxCommandEvent& event );
    void OnSelectBodyStyle( wxCommandEvent& event );
    void OnEditPin( wxCommandEvent& event );
    void OnSelectItem( wxCommandEvent& aEvent );

    void OnOpenPinTable( wxCommandEvent& aEvent );

    void OnUpdatePaste( wxUpdateUIEvent& event );
    void OnUpdateSelectTool( wxUpdateUIEvent& aEvent );
    void OnUpdateEditingPart( wxUpdateUIEvent& event );
    void OnUpdateHavePart( wxUpdateUIEvent& aEvent );
    void OnUpdateSave( wxUpdateUIEvent& aEvent );
    void OnUpdateSaveAs( wxUpdateUIEvent& aEvent );
    void OnUpdateRevert( wxUpdateUIEvent& aEvent );
    void OnUpdateUndo( wxUpdateUIEvent& event );
    void OnUpdateRedo( wxUpdateUIEvent& event );
    void OnUpdateSyncPinEdit( wxUpdateUIEvent& event );
    void OnUpdatePinTable( wxUpdateUIEvent& event );
    void OnUpdatePartNumber( wxUpdateUIEvent& event );
    void OnUpdateDeMorganNormal( wxUpdateUIEvent& event );
    void OnUpdateDeMorganConvert( wxUpdateUIEvent& event );
    void OnUpdateElectricalType( wxUpdateUIEvent& aEvent );
    void OnUpdateSearchTreeTool( wxUpdateUIEvent& aEvent );

    void UpdatePartSelectList();

    /**
     * Redraw the current component loaded in library editor
     * Display reference like in schematic (a reference U is shown U? or U?A)
     * accordint to the current selected unit and De Morgan selection
     * although it is stored without ? and part id.
     * @param aDC = the current device context
     * @param aOffset = a draw offset. usually 0,0 to draw on the screen, but
     * can be set to page size / 2 to draw or print in SVG format.
     */
    void RedrawComponent( wxDC* aDC, wxPoint aOffset );

    /**
     * Redraw the current component loaded in library editor, an axes
     * Display reference like in schematic (a reference U is shown U? or U?A)
     * update status bar and info shown in the bottom of the window
     */
    void RedrawActiveWindow( wxDC* DC, bool EraseBg ) override;

    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos ) override;
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) override;
    double BestZoom() override;         // Returns the best zoom
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos ) override;

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const override;

    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL ) override;

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey = 0 ) override;

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
     * current screen.
     */
    void OnModify();

    int GetUnit() { return m_unit; }

    void SetUnit( int unit )
    {
        wxASSERT( unit >= 1 );
        m_unit = unit;
    }

    int GetConvert() { return m_convert; }

    void SetConvert( int convert )
    {
        wxASSERT( convert >= 0 );
        m_convert = convert;
    }

    LIB_ITEM* GetLastDrawItem() { return m_lastDrawItem; }

    void SetLastDrawItem( LIB_ITEM* drawItem )
    {
        m_lastDrawItem = drawItem;
    }

    LIB_ITEM* GetDrawItem() const { return GetScreen()->GetCurLibItem(); }

    void SetDrawItem( LIB_ITEM* drawItem ) { GetScreen()->SetCurLibItem( drawItem ); }

    bool GetShowDeMorgan() { return m_showDeMorgan; }

    void SetShowDeMorgan( bool show ) { m_showDeMorgan = show; }

    bool GetShowElectricalType() { return m_showPinElectricalTypeName; }

    void SetShowElectricalType( bool aShow ) { m_showPinElectricalTypeName = aShow; }

    FILL_T GetFillStyle() { return m_drawFillStyle; }

    /**
     * Create a temporary copy of the current edited component.
     *
     * Used to prepare an undo and/or abort command before editing the symbol.
     */
    void TempCopyComponent();

    /**
     * Restore the current edited component from its temporary copy.
     * Used to abort a command
     */
    void RestoreComponent();

    /**
     * @return the temporary copy of the current component.
     */
    LIB_PART*      GetTempCopyComponent() { return m_tempCopyComponent; }

    /**
     * Delete temporary copy of the current component and clear pointer
     */
    void ClearTempCopyComponent();

    bool IsEditingDrawItem() { return GetDrawItem() && GetDrawItem()->InEditMode(); }

private:
    void loadPart( const wxString& aLibrary, const wxString& aPart, int Unit );

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
     * Called when the frame is activated.  Tests if the current library exists.
     * The library list can be changed by the schematic editor after reloading a new schematic
     * and the current library can point a non existent lib.
     */
    virtual void OnActivate( wxActivateEvent& event ) override;

    // General:

    /**
     * Set the current active library to \a aLibrary.
     *
     * @param aLibrary the nickname of the library in the symbol library table.  If wxEmptyString,
     *                 then display list of available libraries to select from.
     */
    void SelectActiveLibrary( const wxString& aLibrary = wxEmptyString );

    /**
     * Loads a symbol from the current active library, optionally setting the selected
     * unit and convert.
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
    bool LoadOneLibraryPartAux( LIB_ALIAS* aLibEntry, const wxString& aLibrary,
                                int aUnit, int aConvert );

    /**
     * Display the documentation of the selected component.
     */
    void DisplayCmpDoc();

    /**
     * Rotates the current item.
     */
    void OnRotate( wxCommandEvent& aEvent );

    /**
     * Handles the ID_LIBEDIT_MIRROR_X and ID_LIBEDIT_MIRROR_Y events.
     */
    void OnOrient( wxCommandEvent& aEvent );

    /**
     * Deletes the currently selected draw item.
     *
     * @param aDC The device context to draw upon when removing item.
     * @param aItem The item to delete
     */
    void deleteItem( wxDC* aDC, LIB_ITEM* aItem );

    // General editing
public:
    /**
     * Create a copy of the current component, and save it in the undo list.
     *
     * Because a component in library editor does not have a lot of primitives,
     * the full data is duplicated. It is not worth to try to optimize this save function.
     */
    void SaveCopyInUndoList( EDA_ITEM* ItemToCopy, UNDO_REDO_T undoType = UR_LIBEDIT );

private:
    void GetComponentFromUndoList( wxCommandEvent& event );
    void GetComponentFromRedoList( wxCommandEvent& event );

    // Editing pins
    void CreatePin( wxDC* DC );

    /**
     * Prepare the displacement of a pin
     *
     * @param aPin the pin to prepare for movement.
     */
    void StartMovePin( LIB_ITEM* aPin );

    /**
     * Adds copies of \a aPin in components with multiple units in all units
     *
     * @param aPin The pin to copy to others units.
     */
    void CreateImagePins( LIB_PIN* aPin );

    /**
     * Places an  anchor reference coordinate for the current component.
     * <p>
     * All object coordinates are offset to the current cursor position.
     * </p>
     */
    void PlaceAnchor();

    // Editing graphic items
    LIB_ITEM* CreateGraphicItem( LIB_PART* LibEntry, wxDC* DC );
    void GraphicItemBeginDraw( wxDC* DC );
    void StartMoveDrawSymbol( wxDC* DC, LIB_ITEM* aItem );
    void StartModifyDrawSymbol( wxDC* DC, LIB_ITEM* aItem ); //<! Modify the item, adjust size etc.
    void EndDrawGraphicItem( wxDC* DC );

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

    void EditGraphicSymbol( wxDC* DC, LIB_ITEM* DrawItem );
    void EditSymbolText( wxDC* DC, LIB_ITEM* DrawItem );
    LIB_ITEM* LocateItemUsingCursor( const wxPoint& aPosition,
                                     const KICAD_T aFilterList[] = LIB_COLLECTOR::AllItems );
    void EditField( LIB_FIELD* Field );

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

    /* Block commands: */

    /**
     * Returns the block command (BLOCK_MOVE, BLOCK_DUPLICATE...) corresponding to
     * the \a aKey (ALT, SHIFT ALT ..)
     */
    virtual int BlockCommand( EDA_KEY aKey ) override;

    /**
     * Handles the block place command.
     */
    virtual void HandleBlockPlace( wxDC* DC ) override;

    /**
     * Performs a block end command.
     *
     * @return If command finished (zoom, delete ...) false is returned otherwise true
     *         is returned indicating more processing is required.
     */
    virtual bool HandleBlockEnd( wxDC* DC ) override;

    /**
     * Place at cursor location the pin currently moved (i.e. pin pointed by m_drawItem)
     * (and the linked pins, if any)
     */
    void PlacePin();

    /**
     * @param aMasterPin is the "template" pin
     * @param aId is a param to select what should be mofified:
     * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
     *          Change pins text name size
     * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
     *          Change pins text num size
     * - aId = ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
     *          Change pins length.
     *
     * If aMasterPin is selected ( .m_flag == IS_SELECTED ),
     * only the other selected pins are modified
     */
    void GlobalSetPins( LIB_PIN* aMasterPin, int aId );

    // Automatic placement of pins
    void RepeatPinItem( wxDC* DC, LIB_PIN* Pin );

    /**
     * Creates an image (screenshot) of the current symbol.
     *
     * @param aFileName = the full filename
     * @param aBitmapType = bitmap file format
     */
    void CreateImageFile( const wxString& aFileName, wxBitmapType aBitmapType = wxBITMAP_TYPE_PNG );

    /**
     * Print a page
     *
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMask,
                            bool aPrintMirrorMode, void* aData = NULL ) override;

    /**
     * Creates the SVG print file for the current edited component.
     *
     * @param aFullFileName = the full filename
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
    void InstallPreferences( PAGED_DIALOG* aParent ) override;

    /**
     * Called after the preferences dialog is run.
     */
    void CommonSettingsChanged() override;

    void ShowChangedLanguage() override;

    virtual void SetScreen( BASE_SCREEN* aScreen ) override;

    virtual const BOX2I GetDocumentExtents() const override;

    void RebuildView();

    /**
     * Rebuild the GAL and redraw the screen.  Call when something went wrong.
     */
    void HardRedraw() override;

    /**
     * Checks all draw objects of part to see if they are with block.
     *
     * Use this method to mark draw objects as selected during block
     * functions.
     *
     * @param aRect - The bounding rectangle to test in draw items are inside.
     * @param aUnit - The current unit number to test against.
     * @param aConvert - Are the draw items being selected a conversion.
     * @param aSyncPinEdit - Enable pin selection in other units.
     * @return The number of draw objects found inside the block select
     *         rectangle.
     */
    int BlockSelectItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock, int aUnit, int aConvert, bool aSyncPinEdit );

    /**
     * Clears all the draw items marked by a block select.
     */
    void BlockClearSelectedItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock );

    void BlockMoveSelectedItems( const wxPoint& aOffset, LIB_PART* aPart, BLOCK_SELECTOR* aBlock );
    void BlockDeleteSelectedItems( LIB_PART* aPart, BLOCK_SELECTOR* aBlock );
    void BlockCopySelectedItems( const wxPoint& aOffset, LIB_PART* aPart, BLOCK_SELECTOR* aBlock );

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

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

    ///> Creates or adds an existing library to the symbol library table.
    bool addLibraryFile( bool aCreateNew );

    ///> Stores the currently modified part in the library manager buffer.
    void storeCurrentPart();

    ///> Returns true if \a aLibId is an alias for the editor screen part.
    bool isCurrentPart( const LIB_ID& aLibId ) const;

    ///> Restores the empty editor screen, without any part or library selected.
    void emptyScreen();

    ///> Renames LIB_PART aliases to avoid conflicts before adding a component to a library
    void fixDuplicateAliases( LIB_PART* aPart, const wxString& aLibrary );

    void InitBlockPasteInfos() override;

    /**
     * Copies items selected in the current part to the internal clipboard.
     */
    void copySelectedItems();

    /**
     * Pastes items from the internal clipboard to the current part.
     * @param aOffset is the offset where the pasted items should be located.
     */
    void pasteClipboard( const wxPoint& aOffset );

    ///> Clipboard buffer storing LIB_ITEMs
    BLOCK_SELECTOR m_clipboard;

    DECLARE_EVENT_TABLE()
};

#endif  // LIB_EDIT_FRAME_H
