/*****************************/
/* class WinEDA_LibeditFrame */
/*****************************/

#ifndef __LIBEDITFRM_H__
#define __LIBEDITFRM_H__

#include "wxstruct.h"


class SCH_SCREEN;
class CMP_LIBRARY;
class LIB_COMPONENT;
class LIB_ALIAS;
class LIB_DRAW_ITEM;
class WinEDA_bodygraphics_PropertiesFrame;
class Dialog_BodyGraphicText_Properties;


/**
 * The component library editor main window.
 */
class WinEDA_LibeditFrame : public WinEDA_DrawFrame
{
public:
    WinEDAChoiceBox* m_SelpartBox;      // a Box to select a part to edit (if any)
    WinEDAChoiceBox* m_SelAliasBox;     // a box to select the alias to edit (if any)

public:
    WinEDA_LibeditFrame( wxWindow* father,
                         const wxString& title,
                         const wxPoint& pos, const wxSize& size,
                         long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_LibeditFrame();

    void Process_Special_Functions( wxCommandEvent& event );
    void OnImportPart( wxCommandEvent& event );
    void OnExportPart( wxCommandEvent& event );
    void OnSelectAlias( wxCommandEvent& event );
    void OnSelectPart( wxCommandEvent& event );
    void DeleteOnePart( wxCommandEvent& event );
    void CreateNewLibraryPart( wxCommandEvent& event );
    void OnEditComponentProperties( wxCommandEvent& event );
    void InstallFieldsEditorDialog(  wxCommandEvent& event );
    void LoadOneLibraryPart( wxCommandEvent& event );
    void OnViewEntryDoc( wxCommandEvent& event );
    void OnCheckComponent( wxCommandEvent& event );
    void OnSelectBodyStyle( wxCommandEvent& event );

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
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int  BestZoom();                // Retourne le meilleur zoom
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }
    void OnHotKey( wxDC* DC, int hotkey,
                   EDA_BaseStruct* DrawStruct );

    void GeneralControle( wxDC*   DC,
                          wxPoint MousePositionInPixels );

    void LoadSettings();
    void SaveSettings();

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
        wxASSERT( convert >= 1 );
        m_convert = convert;
    }


    LIB_DRAW_ITEM* GetLastDrawItem( void ) { return m_lastDrawItem; }

    void           SetLastDrawItem( LIB_DRAW_ITEM* drawItem )
    {
        m_lastDrawItem = drawItem;
    }


    LIB_DRAW_ITEM* GetDrawItem( void ) { return m_drawItem; }

    void           SetDrawItem( LIB_DRAW_ITEM* drawItem );

    bool           GetShowDeMorgan( void ) { return m_showDeMorgan; }

    void           SetShowDeMorgan( bool show ) { m_showDeMorgan = show; }

    FILL_T         GetFillStyle( void ) { return m_drawFillStyle; }

private:

    // General:
    void           SaveOnePartInMemory();
    void           SelectActiveLibrary();
    void           SaveActiveLibrary( wxCommandEvent& event );

    bool           LoadOneLibraryPartAux( CMP_LIB_ENTRY* LibEntry,
                                          CMP_LIBRARY*   Library );

    void           DisplayCmpDoc();
    void           EditComponentProperties();

    // General editing
public:
    void           SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                       int             flag_type_command = 0 );

private:
    void           GetComponentFromUndoList( wxCommandEvent& event );
    void           GetComponentFromRedoList( wxCommandEvent& event );

    // Edition des Pins:
    void           CreatePin( wxDC* DC );
    void           DeletePin( wxDC*          DC,
                              LIB_COMPONENT* LibEntry,
                              LIB_PIN*       Pin );
    void           StartMovePin( wxDC* DC );

    // Edition de l'ancre
    void           PlaceAncre();

    // Edition des graphismes:
    LIB_DRAW_ITEM* CreateGraphicItem( LIB_COMPONENT* LibEntry, wxDC* DC );
    void           GraphicItemBeginDraw( wxDC* DC );
    void           StartMoveDrawSymbol( wxDC* DC );
    void           EndDrawGraphicItem( wxDC* DC );
    void           LoadOneSymbol();
    void           SaveOneSymbol();
    void           EditGraphicSymbol( wxDC*          DC,
                                      LIB_DRAW_ITEM* DrawItem );
    void           EditSymbolText( wxDC* DC, LIB_DRAW_ITEM* DrawItem );
    void           RotateSymbolText( wxDC* DC );
    void           DeleteDrawPoly( wxDC* DC );
    LIB_DRAW_ITEM* LocateItemUsingCursor();
    void           RotateField( wxDC* DC, LIB_FIELD* Field );
    void           PlaceField( wxDC* DC, LIB_FIELD* Field );
    void           EditField( wxDC* DC, LIB_FIELD* Field );
    void           StartMoveField( wxDC* DC, LIB_FIELD* field );

public:
    /* Block commands: */
    int            ReturnBlockCommand( int key );
    void           HandleBlockPlace( wxDC* DC );
    int            HandleBlockEnd( wxDC* DC );

    void           PlacePin( wxDC* DC );
    void           InitEditOnePin();
    void           GlobalSetPins( wxDC* DC, LIB_PIN* MasterPin, int id );

    // Repetition automatique de placement de pins
    void           RepeatPinItem( wxDC* DC, LIB_PIN* Pin );

protected:
    wxString              m_ConfigPath;
    wxString              m_LastLibImportPath;
    wxString              m_LastLibExportPath;

    /** Convert of the item currently being drawn. */
    bool                  m_drawSpecificConvert;

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

    /** The current component being edited.  NULL if no component is selected. */
    static LIB_COMPONENT* m_component;

    /** The current active libary.   NULL if no library is active. */
    static CMP_LIBRARY*   m_library;
    static LIB_DRAW_ITEM* m_lastDrawItem;
    static LIB_DRAW_ITEM* m_drawItem;
    static wxString       m_aliasName;

    // The unit number to edit and show
    static int            m_unit;

    // Show the normal shape ( m_convert <= 1 ) or the converted shape
    // ( m_convert > 1 )
    static int            m_convert;

    // true to force DeMorgan/normal tools selection enabled.
    // They are enabled when the loaded component has
    // Graphic items for converted shape
    // But under some circumstances (New component created)
    // these tools must left enable
    static bool           m_showDeMorgan;

    /// The current text size setting.
    static int            m_textSize;

    /// Current text orientation setting.
    static int            m_textOrientation;

    static wxSize         m_clientSize;

    friend class Dialog_BodyGraphicText_Properties;

    DECLARE_EVENT_TABLE()
};

#endif  /* __LIBEDITFRM_H__ */
