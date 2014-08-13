/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file libeditframe.h
 * @brief Definition of class LIB_EDIT_FRAME
 */

#ifndef LIBEDITFRM_H_
#define LIBEDITFRM_H_

#include <sch_base_frame.h>
#include <class_sch_screen.h>

#include <lib_draw_item.h>
#include <lib_collectors.h>


class SCH_EDIT_FRAME;
class PART_LIB;
class LIB_PART;
class LIB_ALIAS;
class LIB_FIELD;
class DIALOG_LIB_EDIT_TEXT;

/**
 * The component library editor main window.
 */
class LIB_EDIT_FRAME : public SCH_BASE_FRAME
{
    LIB_PART*       m_my_part;              ///< a part I own, it is not in any library, but a copy could be.
    LIB_PART*       m_tempCopyComponent;    ///< temp copy of a part during edit, I own it here.
    LIB_COLLECTOR   m_collectedItems;       ///< Used for hit testing.
    wxComboBox*     m_partSelectBox;        ///< a Box to select a part to edit (if any)
    wxComboBox*     m_aliasSelectBox;       ///< a box to select the alias to edit (if any)

    wxString m_configPath;
    wxString m_lastLibImportPath;
    wxString m_lastLibExportPath;

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
     * components with multiple parts or multiple body styles.  Setting this
     * to false allows editing each pin per part or body style individually.
     * This requires the user to open each part or body style to make changes
     * to the pin at the same location.
     */
    bool m_editPinsPerPartOrConvert;

    /** The current draw or edit graphic item fill style. */
    static FILL_T m_drawFillStyle;

    /** Default line width for drawing or editing graphic items. */
    static int m_drawLineWidth;

    static LIB_ITEM*    m_lastDrawItem;
    static LIB_ITEM*    m_drawItem;
    static wxString     m_aliasName;

    // The unit number to edit and show
    static int m_unit;

    // Show the normal shape ( m_convert <= 1 ) or the converted shape
    // ( m_convert > 1 )
    static int m_convert;

    // true to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded component has
    // Graphic items for converted shape
    // But under some circumstances (New component created)
    // these tools must left enable
    static bool m_showDeMorgan;

    /// The current text size setting.
    static int m_textSize;

    /// Current text orientation setting.
    static int m_textOrientation;

    static wxSize m_clientSize;

    friend class DIALOG_LIB_EDIT_TEXT;

    LIB_ITEM* locateItem( const wxPoint& aPosition, const KICAD_T aFilterList[] );

public:

    LIB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~LIB_EDIT_FRAME();

    /**
     * Function GetLibEditFrameName (static)
     * @return the frame name used when creating the frame
     * used to get a reference to this frame, if exists
     */
    static const wxChar* GetLibEditFrameName();

    /** The current library being edited, or NULL if none. */
    PART_LIB* GetCurLib();

    /** Sets the current library and return the old. */
    PART_LIB* SetCurLib( PART_LIB* aLib );

    /**
     * Function GetCurPart
     * returns the current part being edited, or NULL if none selected.
     * This is a LIB_PART that I own, it is at best a copy of one in a library.
     */
    LIB_PART* GetCurPart();

    /**
     * Function SetCurPart
     * takes ownership over aPart and notes that it is the one currently
     * being edited.
     */
    void SetCurPart( LIB_PART* aPart );

    void ReCreateMenuBar();

    /**
     * Function EnsureActiveLibExists
     * must be called after the libraries are reloaded
     * (for instance after loading a schematic project)
     */
    static void EnsureActiveLibExists();

    void InstallConfigFrame( wxCommandEvent& event );
    void InstallDimensionsDialog( wxCommandEvent& event );
    void OnColorConfig( wxCommandEvent& aEvent );
    void Process_Config( wxCommandEvent& event );

    /**
     * Function SycnronizePins
     * @return True if the edit pins per part or convert is false and the current
     *         component has multiple parts or body styles.  Otherwise false is
     *         returned.
     */
    bool SynchronizePins();

    /**
     * Function OnPlotCurrentComponent
     * plot the current component in SVG or PNG format.
     */
    void OnPlotCurrentComponent( wxCommandEvent& event );
    void Process_Special_Functions( wxCommandEvent& event );
    void OnSelectTool( wxCommandEvent& aEvent );

    /**
     * Routine to read one part.
     * The format is that of libraries, but it loads only 1 component.
     * Or 1 component if there are several.
     * If the first component is an alias, it will load the corresponding root.
     */
    void OnImportPart( wxCommandEvent& event );

    /**
     * Function OnExportPart
     * creates a new library and backup the current component in this library or export
     * the component of the current library.
     */
    void OnExportPart( wxCommandEvent& event );
    void OnSelectAlias( wxCommandEvent& event );
    void OnSelectPart( wxCommandEvent& event );

    /**
     * Function DeleteOnePart
     * is the command event handler to delete an entry from the current library.
     *
     * The deleted entry can be an alias or a component.  If the entry is an alias,
     * it is removed from the component and the list of alias is updated.  If the
     * entry is a component and the list of aliases is empty, the component and all
     * it drawable items are deleted.  Otherwise the first alias in the alias list
     * becomes the new component name and the other aliases become dependent on
     * renamed component.
     *
     * @note This only deletes the entry in memory.  The file does not change.
     */
    void DeleteOnePart( wxCommandEvent& event );

    /**
     * Function CreateNewLibraryPart
     * is the command event handler to create a new library component.
     *
     * If an old component is currently in edit, it is deleted.
     */
    void CreateNewLibraryPart( wxCommandEvent& event );

    void OnCreateNewPartFromExisting( wxCommandEvent& event );
    void OnEditComponentProperties( wxCommandEvent& event );
    void InstallFieldsEditorDialog(  wxCommandEvent& event );

    /**
     * Function LoadOneLibraryPart
     * loads a library component from the currently selected library.
     *
     * If a library is already selected, the user is prompted for the component name
     * to load.  If there is no current selected library, the user is prompted to select
     * a library name and then select component to load.
     */
    void LoadOneLibraryPart( wxCommandEvent& event );

    void OnViewEntryDoc( wxCommandEvent& event );
    void OnCheckComponent( wxCommandEvent& event );
    void OnSelectBodyStyle( wxCommandEvent& event );
    void OnEditPin( wxCommandEvent& event );
    void OnSelectItem( wxCommandEvent& aEvent );

    void OnUpdateSelectTool( wxUpdateUIEvent& aEvent );
    void OnUpdateEditingPart( wxUpdateUIEvent& event );
    void OnUpdateNotEditingPart( wxUpdateUIEvent& event );
    void OnUpdateUndo( wxUpdateUIEvent& event );
    void OnUpdateRedo( wxUpdateUIEvent& event );
    void OnUpdateSaveCurrentLib( wxUpdateUIEvent& event );
    void OnUpdateViewDoc( wxUpdateUIEvent& event );
    void OnUpdatePinByPin( wxUpdateUIEvent& event );
    void OnUpdatePartNumber( wxUpdateUIEvent& event );
    void OnUpdateDeMorganNormal( wxUpdateUIEvent& event );
    void OnUpdateDeMorganConvert( wxUpdateUIEvent& event );
    void OnUpdateSelectAlias( wxUpdateUIEvent& event );

    void UpdateAliasSelectList();
    void UpdatePartSelectList();

    /**
     * Function DisplayLibInfos
     * updates the main window title bar with the current library name and read only status
     * of the library.
     */
    void DisplayLibInfos();

    /**
     * Function RedrawComponent
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
     * Function RedrawActiveWindow
     * Redraw the current component loaded in library editor, an axes
     * Display reference like in schematic (a reference U is shown U? or U?A)
     * update status bar and info shown in the bottom of the window
     */
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void CreateOptionToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    double BestZoom();         // Returns the best zoom
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    void OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    void GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    void LoadSettings( wxConfigBase* aCfg );

    void SaveSettings( wxConfigBase* aCfg );

    /**
     * Function CloseWindow
     * triggers the wxCloseEvent, which is handled by the function given
     * to EVT_CLOSE() macro:
     * <p>
     * EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
     */
    void CloseWindow( wxCommandEvent& event )
    {
        // Generate a wxCloseEvent
        Close( false );
    }

    /**
     * Function OnModify
     * Must be called after a schematic change
     * in order to set the "modify" flag of the current screen
     */
    void OnModify()
    {
        GetScreen()->SetModify();
    }

    const wxString& GetAliasName()      { return m_aliasName; }

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

    LIB_ITEM* GetDrawItem() { return m_drawItem; }

    void SetDrawItem( LIB_ITEM* drawItem );

    bool GetShowDeMorgan() { return m_showDeMorgan; }

    void SetShowDeMorgan( bool show ) { m_showDeMorgan = show; }

    FILL_T GetFillStyle() { return m_drawFillStyle; }

    /**
     * Function TempCopyComponent
     * create a temporary copy of the current edited component
     * Used to prepare an Undo ant/or abort command before editing the component
     */
    void TempCopyComponent();

    /**
     * Function RestoreComponent
     * Restore the current edited component from its temporary copy.
     * Used to abort a command
     */
    void RestoreComponent();

    /**
     * Function GetTempCopyComponent
     * @return the temporary copy of the current component.
     */
    LIB_PART*      GetTempCopyComponent() { return m_tempCopyComponent; }

    /**
     * Function ClearTempCopyComponent
     * delete temporary copy of the current component and clear pointer
     */
    void ClearTempCopyComponent();

    bool IsEditingDrawItem() { return m_drawItem && m_drawItem->InEditMode(); }

private:

    /**
     * Function OnActivate
     * is called when the frame is activated. Tests if the current library exists.
     * The library list can be changed by the schematic editor after reloading a new schematic
     * and the current library can point a non existent lib.
     */
    virtual void OnActivate( wxActivateEvent& event );

    // General:

    /**
     * Function SaveOnePart
     * saves the current LIB_PART into the provided PART_LIB.
     *
     * Any changes are updated in memory only and NOT to a file.  The old component is
     * deleted from the library and/or any aliases before the edited component is updated
     * in the library.
     */
    void SaveOnePart( PART_LIB* aLib );

    /**
     * Function SelectActiveLibrary
     * sets the current active library to \a aLibrary.
     *
     * @param aLibrary A pointer to the PART_LIB object to select.  If NULL, then display
     *                 list of available libraries to select from.
     */
    void SelectActiveLibrary( PART_LIB* aLibrary = NULL );

    /**
     * Function OnSaveActiveLibrary
     * it the command event handler to save the changes to the current library.
     *
     * A backup file of the current library is saved with the .bak extension before the
     * changes made to the library are saved.
     */
    void OnSaveActiveLibrary( wxCommandEvent& event );

    /**
     * Function SaveActiveLibrary
     * saves the changes to the current library.
     *
     * A backup file of the current library is saved with the .bak extension before the
     * changes made to the library are saved.
     * @param newFile Ask for a new file name to save the library.
     * @return True if the library was successfully saved.
     */
    bool SaveActiveLibrary( bool newFile );

    /**
     * Function LoadComponentFromCurrentLib
     * loads a component from the current active library.
     * @param aLibEntry The component to load from \a aLibrary (can be an alias)
     * @return true if \a aLibEntry loaded correctly.
     */
    bool LoadComponentFromCurrentLib( LIB_ALIAS* aLibEntry );

    /**
     * Function LoadOneLibraryPartAux
     * loads a copy of \a aLibEntry from \a aLibrary into memory.
     *
     * @param aLibEntry A pointer to the LIB_ALIAS object to load.
     * @param aLibrary A pointer to the PART_LIB object to load \a aLibEntry from.
     * @return True if a copy of \a aLibEntry was successfully loaded from \a aLibrary.
     */
    bool LoadOneLibraryPartAux( LIB_ALIAS* aLibEntry, PART_LIB* aLibrary );

    /**
     * Function DisplayCmpDoc
     * displays the documentation of the selected component.
     */
    void DisplayCmpDoc();

    /**
     * Function OnRotateItem
     * rotates the current item.
     */
    void OnRotateItem( wxCommandEvent& aEvent );

    /**
     * Function deleteItem
     * deletes the currently selected draw item.
     * @param aDC The device context to draw upon when removing item.
     */
    void deleteItem( wxDC* aDC );

    // General editing
public:
    void SaveCopyInUndoList( EDA_ITEM* ItemToCopy, int flag_type_command = 0 );

private:
    void GetComponentFromUndoList( wxCommandEvent& event );
    void GetComponentFromRedoList( wxCommandEvent& event );

    // Editing pins
    void CreatePin( wxDC* DC );
    void StartMovePin( wxDC* DC );

    /**
     * Function CreateImagePins
     * adds copies of \a aPin for \a aUnit in components with multiple parts and
     * \a aConvert for components that have multiple body styles.
     *
     * @param aPin The pin to copy.
     * @param aUnit The unit to add a copy of \a aPin to.
     * @param aConvert The alternate body style to add a copy of \a aPin to.
     * @param aDeMorgan Flag to indicate if \a aPin should be created for the
     *                  alternate body style.
     */
    void CreateImagePins( LIB_PIN* aPin, int aUnit, int aConvert, bool aDeMorgan );

    /**
     * Function PlaceAnchor
     * places an  anchor reference coordinate for the current component.
     * <p>
     * All object coordinates are offset to the current cursor position.
     * </p>
     */
    void PlaceAnchor();

    // Editing graphic items
    LIB_ITEM* CreateGraphicItem( LIB_PART*      LibEntry, wxDC* DC );
    void GraphicItemBeginDraw( wxDC* DC );
    void StartMoveDrawSymbol( wxDC* DC );
    void StartModifyDrawSymbol( wxDC* DC ); //<! Modify the item, adjust size etc.
    void EndDrawGraphicItem( wxDC* DC );

    /**
     * Function LoadOneSymbol
     * read a component symbol file (*.sym ) and add graphic items to the current component.
     * <p>
     * A symbol file *.sym has the same format as a library, and contains only
     * one symbol.
     * </p>
     */
    void LoadOneSymbol();

    /**
     * Function SaveOneSymbol
     * saves the current component to a symbol file.
     * <p>
     * The symbol file format is similar to the standard component library file format, but
     * there is only one symbol.  Invisible pins are not saved.
     */
    void SaveOneSymbol();

    void EditGraphicSymbol( wxDC* DC, LIB_ITEM* DrawItem );
    void EditSymbolText( wxDC* DC, LIB_ITEM* DrawItem );
    LIB_ITEM* LocateItemUsingCursor( const wxPoint& aPosition,
                                     const KICAD_T aFilterList[] = LIB_COLLECTOR::AllItems );
    void EditField( LIB_FIELD* Field );

public:
    /**
     * Function LoadComponentAndSelectLib
     * selects the current active library.
     *
     * @param aLibrary The PART_LIB to select
     * @param aLibEntry The component to load from aLibrary (can be an alias).
     * @return true if \a aLibEntry was loaded from \a aLibrary.
     */
    bool LoadComponentAndSelectLib( LIB_ALIAS* aLibEntry, PART_LIB* aLibrary );

    /* Block commands: */

    /**
     * Function BlockCommand
     * returns the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
     * the \a aKey (ALT, SHIFT ALT ..)
     */
    virtual int BlockCommand( int aKey );

    /**
     * Function HandleBlockPlace
     * handles the block place command.
     */
    virtual void HandleBlockPlace( wxDC* DC );

    /**
     * Function HandleBlockEnd
     * performs a block end command.
     * @return If command finished (zoom, delete ...) false is returned otherwise true
     *         is returned indicating more processing is required.
     */
    virtual bool HandleBlockEnd( wxDC* DC );

    /**
     * Function PlacePin
     * Place at cursor location the pin currently moved (i.e. pin pointed by m_drawItem)
     * (and the linked pins, if any)
     */
    void PlacePin();

    /**
     * Function GlobalSetPins
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
     * Function CreatePNGorJPEGFile
     * creates an image (screenshot) of the current component in PNG or JPEG format.
     * @param aFileName = the full filename
     * @param aFmt_jpeg = true to use JPEG file format, false to use PNG file format
     */
    void CreatePNGorJPEGFile( const wxString& aFileName, bool aFmt_jpeg );

    /**
     * Virtual function PrintPage
     * used to print a page
     * @param aDC = wxDC given by the calling print function
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, LSET aPrintMask,
                            bool aPrintMirrorMode, void* aData = NULL );

    /**
     * Function SVG_PlotComponent
     * Creates the SVG print file for the current edited component.
     * @param aFullFileName = the full filename
     */
    void SVG_PlotComponent( const wxString& aFullFileName );

    DECLARE_EVENT_TABLE()
};

#endif  // LIBEDITFRM_H_
