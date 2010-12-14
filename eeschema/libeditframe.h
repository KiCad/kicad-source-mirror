
/************************/
/* class LIB_EDIT_FRAME */
/************************/

#ifndef __LIBEDITFRM_H__
#define __LIBEDITFRM_H__

#include "wxstruct.h"
#include "class_sch_screen.h"

#include "lib_draw_item.h"


class SCH_EDIT_FRAME;
class CMP_LIBRARY;
class LIB_COMPONENT;
class LIB_ALIAS;
class LIB_FIELD;
class DIALOG_LIB_EDIT_TEXT;


/**
 * The component library editor main window.
 */
class LIB_EDIT_FRAME : public WinEDA_DrawFrame
{
    LIB_COMPONENT* m_tempCopyComponent;  ///< Temporary copy of current component during edit.

public:
    WinEDAChoiceBox* m_SelpartBox;       // a Box to select a part to edit (if any)
    WinEDAChoiceBox* m_SelAliasBox;      // a box to select the alias to edit (if any)

public:
    LIB_EDIT_FRAME( SCH_EDIT_FRAME* aParent, const wxString& title,
                    const wxPoint& pos, const wxSize& size,
                    long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~LIB_EDIT_FRAME();

    void        ReCreateMenuBar();

    /**
     * Function EnsureActiveLibExists
     * must be called after the libraries are reloaded
     * (for instance after loading a schematic project)
     */
    static void EnsureActiveLibExists();

    /**
     * Function SetLanguage
     * is called on a language menu selection
     */
    void        SetLanguage( wxCommandEvent& event );

    void        InstallConfigFrame( wxCommandEvent& event );
    void        InstallDimensionsDialog( wxCommandEvent& event );
    void        OnColorConfig( wxCommandEvent& aEvent );
    void        Process_Config( wxCommandEvent& event );
    void        OnPlotCurrentComponent( wxCommandEvent& event );
    void        Process_Special_Functions( wxCommandEvent& event );
    void        OnImportPart( wxCommandEvent& event );
    void        OnExportPart( wxCommandEvent& event );
    void        OnSelectAlias( wxCommandEvent& event );
    void        OnSelectPart( wxCommandEvent& event );
    void        DeleteOnePart( wxCommandEvent& event );
    void        CreateNewLibraryPart( wxCommandEvent& event );
    void        OnCreateNewPartFromExisting( wxCommandEvent& event );
    void        OnEditComponentProperties( wxCommandEvent& event );
    void        InstallFieldsEditorDialog(  wxCommandEvent& event );
    void        LoadOneLibraryPart( wxCommandEvent& event );
    void        OnViewEntryDoc( wxCommandEvent& event );
    void        OnCheckComponent( wxCommandEvent& event );
    void        OnSelectBodyStyle( wxCommandEvent& event );
    void        OnEditPin( wxCommandEvent& event );
    void        OnRotatePin( wxCommandEvent& event );

    void        OnUpdateEditingPart( wxUpdateUIEvent& event );
    void        OnUpdateNotEditingPart( wxUpdateUIEvent& event );
    void        OnUpdateUndo( wxUpdateUIEvent& event );
    void        OnUpdateRedo( wxUpdateUIEvent& event );
    void        OnUpdateSaveCurrentLib( wxUpdateUIEvent& event );
    void        OnUpdateViewDoc( wxUpdateUIEvent& event );
    void        OnUpdatePinByPin( wxUpdateUIEvent& event );
    void        OnUpdatePartNumber( wxUpdateUIEvent& event );
    void        OnUpdateDeMorganNormal( wxUpdateUIEvent& event );
    void        OnUpdateDeMorganConvert( wxUpdateUIEvent& event );
    void        OnUpdateSelectAlias( wxUpdateUIEvent& event );

    void        UpdateAliasSelectList();
    void        UpdatePartSelectList();
    void        DisplayLibInfos();
    void        RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void        OnCloseWindow( wxCloseEvent& Event );
    void        ReCreateHToolbar();
    void        ReCreateVToolbar();
    void        OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool        OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int         BestZoom();         // Returns the best zoom
    void        OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }
    void        OnHotKey( wxDC* DC, int hotkey, EDA_ITEM* DrawStruct );

    void        GeneralControle( wxDC* DC, wxPoint MousePositionInPixels );

    void        LoadSettings();
    void        SaveSettings();

    /**
     * Function CloseWindow
     * triggers the wxCloseEvent, which is handled by the function given
     * to EVT_CLOSE() macro:
     * <p>
     * EVT_CLOSE( LIB_EDIT_FRAME::OnCloseWindow )
     */
    void CloseWindow( wxCommandEvent& WXUNUSED(event) )
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


    LIB_COMPONENT* GetComponent( void ) { return m_component; }

    CMP_LIBRARY* GetLibrary( void ) { return m_library; }

    wxString& GetAliasName( void ) { return m_aliasName; }

    int                GetUnit( void ) { return m_unit; }

    void               SetUnit( int unit )
    {
        wxASSERT( unit >= 1 );
        m_unit = unit;
    }


    int                GetConvert( void ) { return m_convert; }

    void               SetConvert( int convert )
    {
        wxASSERT( convert >= 0 );
        m_convert = convert;
    }


    LIB_DRAW_ITEM* GetLastDrawItem( void ) { return m_lastDrawItem; }

    void           SetLastDrawItem( LIB_DRAW_ITEM* drawItem )
    {
        m_lastDrawItem = drawItem;
    }


    LIB_DRAW_ITEM* GetDrawItem( void ) { return m_drawItem; }

    void SetDrawItem( LIB_DRAW_ITEM* drawItem );

    bool           GetShowDeMorgan( void ) { return m_showDeMorgan; }

    void           SetShowDeMorgan( bool show ) { m_showDeMorgan = show; }

    FILL_T         GetFillStyle( void ) { return m_drawFillStyle; }

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
    LIB_COMPONENT* GetTempCopyComponent() { return m_tempCopyComponent; }

    /**
     * Function ClearTempCopyComponent
     * delete temporary copy of the current component and clear pointer
     */
    void ClearTempCopyComponent();

    bool           IsEditingDrawItem() { return m_drawItem && m_drawItem->InEditMode(); }

private:

    /**
     * Function OnActivate
     * is called when the frame is activated. Tests if the current library exists.
     * The library list can be changed by the schematic editor after reloading a new schematic
     * and the current m_library can point a non existent lib.
     */
    virtual void   OnActivate( wxActivateEvent& event );

    // General:
    void           SaveOnePartInMemory();
    void           SelectActiveLibrary();
    void           SaveActiveLibrary( wxCommandEvent& event );

    bool           LoadOneLibraryPartAux( LIB_ALIAS* LibEntry, CMP_LIBRARY* Library );

    void           DisplayCmpDoc();

    // General editing
public:
    void           SaveCopyInUndoList( EDA_ITEM* ItemToCopy, int flag_type_command = 0 );

private:
    void           GetComponentFromUndoList( wxCommandEvent& event );
    void           GetComponentFromRedoList( wxCommandEvent& event );

    // Editing pins
    void           CreatePin( wxDC* DC );
    void           DeletePin( wxDC* DC, LIB_COMPONENT* LibEntry, LIB_PIN* Pin );
    void           StartMovePin( wxDC* DC );

    // Editing anchor
    void           PlaceAncre();

    // Editing graphic items
    LIB_DRAW_ITEM* CreateGraphicItem( LIB_COMPONENT* LibEntry, wxDC* DC );
    void           GraphicItemBeginDraw( wxDC* DC );
    void           StartMoveDrawSymbol( wxDC* DC );
    void           StartModifyDrawSymbol( wxDC* DC ); //<! Modify the item, adjust size etc.
    void           EndDrawGraphicItem( wxDC* DC );
    void           LoadOneSymbol();
    void           SaveOneSymbol();
    void           EditGraphicSymbol( wxDC* DC, LIB_DRAW_ITEM* DrawItem );
    void           EditSymbolText( wxDC* DC, LIB_DRAW_ITEM* DrawItem );
    LIB_DRAW_ITEM* LocateItemUsingCursor();
    void           EditField( wxDC* DC, LIB_FIELD* Field );

public:
    /* Block commands: */
    virtual int    ReturnBlockCommand( int aKey );
    virtual void   HandleBlockPlace( wxDC* DC );
    virtual bool   HandleBlockEnd( wxDC* DC );

    void           PlacePin( wxDC* DC );
    void           GlobalSetPins( wxDC* DC, LIB_PIN* MasterPin, int id );

    // Automatic placement of pins
    void           RepeatPinItem( wxDC* DC, LIB_PIN* Pin );

protected:
    wxString m_ConfigPath;
    wxString m_LastLibImportPath;
    wxString m_LastLibExportPath;

    /** Convert of the item currently being drawn. */
    bool     m_drawSpecificConvert;

    /**
     * Specify which component parts the current draw item applies to.
     *
     * If true, the item being drawn or edited applies only to the selected
     * part.  Otherwise it applies to all parts in the component.
     */
    bool                  m_drawSpecificUnit;

    /** The current draw or edit graphic item fill style. */
    static FILL_T         m_drawFillStyle;

    /** Default line width for drawing or editing graphic items. */
    static int            m_drawLineWidth;

    /** The current active libary. NULL if no active library is selected. */
    static CMP_LIBRARY*   m_library;
    /** The current component being edited.  NULL if no component is selected. */
    static LIB_COMPONENT* m_component;

    static LIB_DRAW_ITEM* m_lastDrawItem;
    static LIB_DRAW_ITEM* m_drawItem;
    static wxString       m_aliasName;

    // The unit number to edit and show
    static int            m_unit;

    // Show the normal shape ( m_convert <= 1 ) or the converted shape
    // ( m_convert > 1 )
    static int m_convert;

    // true to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded component has
    // Graphic items for converted shape
    // But under some circumstances (New component created)
    // these tools must left enable
    static bool   m_showDeMorgan;

    /// The current text size setting.
    static int    m_textSize;

    /// Current text orientation setting.
    static int    m_textOrientation;

    static wxSize m_clientSize;

    friend class DIALOG_LIB_EDIT_TEXT;

    /**
     * Function CreatePNGorJPEGFile
     * Create an image (screenshot) of the current component.
     *  Output file format is png or jpeg
     * @param aFileName = the full filename
     * @param aFmt_jpeg = true to use JPEG ffile format, false to use PNG file format
     */
    void         CreatePNGorJPEGFile( const wxString& aFileName, bool aFmt_jpeg );


    /** Virtual function PrintPage
     * used to print a page
     * Print the page pointed by ActiveScreen, set by the calling print function
     * @param aDC = wxDC given by the calling print function
     * @param aPrint_Sheet_Ref = true to print page references
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref, int aPrintMask,
                            bool aPrintMirrorMode, void* aData = NULL );

    /**
     * Function SVG_Print_Component
     * Creates the SVG print file for the current edited component.
     * @param aFullFileName = the full filename of the file
    */
    void SVG_Print_Component( const wxString& aFullFileName );


    DECLARE_EVENT_TABLE()
};

#endif  /* __LIBEDITFRM_H__ */
