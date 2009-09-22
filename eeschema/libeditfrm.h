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

/**
 * The component library editor main window.
 */
class WinEDA_LibeditFrame : public WinEDA_DrawFrame
{
public:
    WinEDAChoiceBox* m_SelpartBox;
    WinEDAChoiceBox* m_SelAliasBox;

public:
    WinEDA_LibeditFrame( wxWindow* father,
                         const wxString& title,
                         const wxPoint& pos, const wxSize& size,
                         long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_LibeditFrame();

    void               Process_Special_Functions( wxCommandEvent& event );
    void               OnImportPart( wxCommandEvent& event );
    void               OnExportPart( wxCommandEvent& event );
    void               OnSelectAlias( wxCommandEvent& event );
    void               OnSelectPart( wxCommandEvent& event );
    void               DeleteOnePart( wxCommandEvent& event );
    void               CreateNewLibraryPart( wxCommandEvent& event );
    void               OnEditComponentProperties( wxCommandEvent& event );
    void               InstallFieldsEditorDialog(  wxCommandEvent& event );

    void               OnUpdateEditingPart( wxUpdateUIEvent& event );
    void               OnUpdateNotEditingPart( wxUpdateUIEvent& event );
    void               OnUpdateUndo( wxUpdateUIEvent& event );
    void               OnUpdateRedo( wxUpdateUIEvent& event );
    void               OnUpdateSaveCurrentLib( wxUpdateUIEvent& event );
    void               OnUpdateViewDoc( wxUpdateUIEvent& event );
    void               OnUpdatePinByPin( wxUpdateUIEvent& event );
    void               OnUpdatePartNumber( wxUpdateUIEvent& event );
    void               OnUpdateDeMorganNormal( wxUpdateUIEvent& event );
    void               OnUpdateDeMorganConvert( wxUpdateUIEvent& event );
    void               OnUpdateSelectAlias( wxUpdateUIEvent& event );

    void               UpdateAliasSelectList();
    void               UpdatePartSelectList();
    void               DisplayLibInfos();
    void               RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void               OnCloseWindow( wxCloseEvent& Event );
    void               ReCreateHToolbar();
    void               ReCreateVToolbar();
    void               OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool               OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int                BestZoom();  // Retourne le meilleur zoom
    void               OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    SCH_SCREEN*        GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }
    void               OnHotKey( wxDC* DC, int hotkey,
                                 EDA_BaseStruct* DrawStruct );

    void               GeneralControle( wxDC*   DC,
                                        wxPoint MousePositionInPixels );

    void               LoadSettings();
    void               SaveSettings();

    LIB_COMPONENT*     GetCurrentComponent( void )
    {
        return m_currentComponent;
    }

private:

    // General:
    void               SaveOnePartInMemory();
    void               SelectActiveLibrary();
    bool               LoadOneLibraryPart();
    void               SaveActiveLibrary( wxCommandEvent& event );

    bool               LoadOneLibraryPartAux( CMP_LIB_ENTRY* LibEntry,
                                              CMP_LIBRARY* Library );

    void               DisplayCmpDoc();
    void               EditComponentProperties();

    // General editing
public:
    void               SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                           int flag_type_command = 0 );

private:
    void               GetComponentFromUndoList(wxCommandEvent& event);
    void               GetComponentFromRedoList(wxCommandEvent& event);

    // Edition des Pins:
    void               CreatePin( wxDC* DC );
    void               DeletePin( wxDC*          DC,
                                  LIB_COMPONENT* LibEntry,
                                  LibDrawPin*    Pin );
    void               StartMovePin( wxDC* DC );

    // Test des pins ( duplicates...)
    bool               TestPins( LIB_COMPONENT* LibEntry );

    // Edition de l'ancre
    void               PlaceAncre();

    // Edition des graphismes:
    LibEDA_BaseStruct* CreateGraphicItem( LIB_COMPONENT* LibEntry, wxDC* DC );
    void               GraphicItemBeginDraw( wxDC* DC );
    void               StartMoveDrawSymbol( wxDC* DC );
    void               EndDrawGraphicItem( wxDC* DC );
    void               LoadOneSymbol();
    void               SaveOneSymbol();
    void               EditGraphicSymbol( wxDC* DC,
                                          LibEDA_BaseStruct* DrawItem );
    void               EditSymbolText( wxDC* DC, LibEDA_BaseStruct* DrawItem );
    void               RotateSymbolText( wxDC* DC );
    void               DeleteDrawPoly( wxDC* DC );
    LibDrawField*      LocateField( LIB_COMPONENT* LibEntry );
    LibEDA_BaseStruct* LocateItemUsingCursor();
    void               RotateField( wxDC* DC, LibDrawField* Field );
    void               PlaceField( wxDC* DC, LibDrawField* Field );
    void               EditField( wxDC* DC, LibDrawField* Field );
    void               StartMoveField( wxDC* DC, LibDrawField* field );

public:
    /* Block commands: */
    int                ReturnBlockCommand( int key );
    void               HandleBlockPlace( wxDC* DC );
    int                HandleBlockEnd( wxDC* DC );

    void               PlacePin( wxDC* DC );
    void               InitEditOnePin();
    void               GlobalSetPins( wxDC* DC, LibDrawPin* MasterPin, int id );

    // Repetition automatique de placement de pins
    void               RepeatPinItem( wxDC* DC, LibDrawPin* Pin );

protected:
    wxString m_ConfigPath;
    wxString m_LastLibImportPath;
    wxString m_LastLibExportPath;

    static LIB_COMPONENT* m_currentComponent;

    DECLARE_EVENT_TABLE()
};

#endif  /* __LIBEDITFRM_H__ */
