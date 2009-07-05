/***********************************************************/
/*              wxEeschemaStruct.h:                        */
/* descriptions des principales classes derivees utilisees */
/***********************************************************/

#ifndef  WX_EESCHEMA_STRUCT_H
#define  WX_EESCHEMA_STRUCT_H

#include "wxstruct.h"
#include "param_config.h"


class WinEDA_LibeditFrame;
class WinEDA_ViewlibFrame;
class SCH_SCREEN;
class DRAWSEGMENT;
class DrawPickedStruct;
class SCH_ITEM;
class DrawNoConnectStruct;
class LibraryStruct;
class EDA_LibComponentStruct;
class LibEDA_BaseStruct;
class EDA_BaseStruct;
class DrawBusEntryStruct;
class SCH_GLOBALLABEL;
class SCH_TEXT;
class EDA_DrawLineStruct;
class DrawSheetStruct;
class DrawSheetPath;
class Hierarchical_PIN_Sheet_Struct;
class SCH_COMPONENT;
class LibDrawField;
class SCH_CMP_FIELD;
class LibDrawPin;
class DrawJunctionStruct;

/*******************************/
/* class WinEDA_SchematicFrame */
/*******************************/

/* enum used in RotationMiroir() */
enum fl_rot_cmp {
    CMP_NORMAL,                         // orientation normale (O, pas de miroir)
    CMP_ROTATE_CLOCKWISE,               // nouvelle rotation de -90
    CMP_ROTATE_COUNTERCLOCKWISE,        // nouvelle rotation de +90
    CMP_ORIENT_0,                       // orientation 0, pas de miroir, id CMP_NORMAL
    CMP_ORIENT_90,                      // orientation 90, pas de miroir
    CMP_ORIENT_180,                     // orientation 180, pas de miroir
    CMP_ORIENT_270,                     // orientation -90, pas de miroir
    CMP_MIROIR_X = 0x100,               // miroir selon axe X
    CMP_MIROIR_Y = 0x200                // miroir selon axe Y
};


class WinEDA_SchematicFrame : public WinEDA_DrawFrame
{
public:
    WinEDAChoiceBox*     m_SelPartBox;
    DrawSheetPath*       m_CurrentSheet;    ///< which sheet we are presently working on.
    int                  m_Multiflag;
    int                  m_NetlistFormat;
    bool                 m_ShowAllPins;
    wxPoint              m_OldPos;
    WinEDA_LibeditFrame* m_LibeditFrame;
    WinEDA_ViewlibFrame* m_ViewlibFrame;
    wxString             m_UserLibraryPath;
    wxArrayString        m_ComponentLibFiles;


private:
    wxString             m_DefaultSchematicFileName;
    SCH_CMP_FIELD*       m_CurrentField;
    int                  m_TextFieldSize;
    bool                 m_ShowGrid;
    PARAM_CFG_ARRAY      m_projectFileParams;
    PARAM_CFG_ARRAY      m_configSettings;


public:
    WinEDA_SchematicFrame( wxWindow* father,
                           const wxString& title,
                           const wxPoint& pos, const wxSize& size,
                           long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_SchematicFrame();

    void OnCloseWindow( wxCloseEvent& Event );
    void Process_Special_Functions( wxCommandEvent& event );
    void Process_Config( wxCommandEvent& event );

    void GeneralControle( wxDC* DC, wxPoint MousePositionInPixels );

    PARAM_CFG_ARRAY& GetProjectFileParameters( void );
    void SaveProjectFile( wxWindow* displayframe );
    bool LoadProjectFile( const wxString& CfgFileName, bool ForceRereadConfig );

    PARAM_CFG_ARRAY& GetConfigurationSettings( void );
    void LoadSettings();
    void SaveSettings();

    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void CreateScreens();
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void ReCreateOptToolbar();
    void ReCreateMenuBar();
    void OnHotKey( wxDC*           DC,
                   int             hotkey,
                   EDA_BaseStruct* DrawStruct );

    SCH_CMP_FIELD* GetCurrentField() { return m_CurrentField; }

    void                    SetCurrentField( SCH_CMP_FIELD* aCurrentField )
    {
        m_CurrentField = aCurrentField;
    }


    DrawSheetPath*   GetSheet();

    SCH_SCREEN*      GetScreen() const;

    BASE_SCREEN*     GetBaseScreen() const;

    virtual wxString GetScreenDesc();

    void             InstallConfigFrame( const wxPoint& pos );

    void             OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void             OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool             OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void             OnSelectOptionToolbar( wxCommandEvent& event );
    int              BestZoom();        // Retourne le meilleur zoom

    SCH_ITEM*        SchematicGeneralLocateAndDisplay( bool IncludePin = TRUE );
    SCH_ITEM*        SchematicGeneralLocateAndDisplay(
        const wxPoint& refpoint,
        bool
                       IncludePin );

    /**
     * Function FillFootprintFieldForAllInstancesofComponent
     * searches for component "aReference", and places a Footprint in Footprint field
     * @param aReference = reference of the component to initialise
     * @param aFootPrint = new value for the filed Fottprint component
     * @param aSetVisible = true to have the field visible, false to set the invisible flag
     * @return true if the given component is found
     * Note:
     * the component is searched in the whole schematic, and because some components
     * have more than one instance (multiple parts per package components)
     * the search is not stopped when a reference is found (all instances must be found).
     */
    bool      FillFootprintFieldForAllInstancesofComponent( const wxString& aReference,
                                                            const wxString& aFootPrint,
                                                            bool            aSetVisible );

    SCH_ITEM* FindComponentAndItem( const wxString& component_reference,
                                    bool            Find_in_hierarchy,
                                    int             SearchType,
                                    const wxString& text_to_find,
                                    bool            mouseWarp );

    /* Cross probing with pcbnew */
    void                           SendMessageToPCBNEW( EDA_BaseStruct* objectToSync,
                                                        SCH_COMPONENT*  LibItem );

    /* netlist generation */
    void*                          BuildNetListBase();

    /**
     * Function DeleteAnnotation
     * Remove current component annotations
     * @param aCurrentSheetOnly : if false: remove all annotations, else remove annotation relative to the current sheet only
     * @param aRedraw : true to refresh display
     */
    void                           DeleteAnnotation( bool aCurrentSheetOnly, bool aRedraw );

    // Functions used for hierarchy handling
    void                           InstallPreviousSheet();
    void                           InstallNextScreen( DrawSheetStruct* Sheet );

    /** Function GetUniqueFilenameForCurrentSheet
     * @return a filename that can be used in plot and print functions
     * for the current screen anad sheet path.
     * This filename is unique and must be used insteed of the sreen filename
     * (or scheen filename) when one must creates file for each sheet in the heierarchy.
     * because in complex hierarchies a sheet and a SCH_SCREEN is used more than once
     * Name is <root sheet filename>-<sheet path>
     * and has no extension.
     * However if filename is too long name is <sheet filename>-<sheet number>
     */
    wxString                       GetUniqueFilenameForCurrentSheet();

    /**
     * Function SetSheetNumberAndCount
     * Set the m_ScreenNumber and m_NumberOfScreen members for screens
     * must be called after a delete or add sheet command, and when entering a sheet
     */
    void                           SetSheetNumberAndCount();

    // Plot functions:
    void                           ToPlot_PS( wxCommandEvent& event );
    void                           ToPlot_HPGL( wxCommandEvent& event );
    void                           ToPlot_DXF( wxCommandEvent& event );
    void                           ToPostProcess( wxCommandEvent& event );

    // read and save files
    void                           Save_File( wxCommandEvent& event );
    void                           SaveProject();
    int                            LoadOneEEProject( const wxString& FileName, bool IsNew );
    bool                           LoadOneEEFile( SCH_SCREEN* screen, const wxString& FullFileName );
    bool                           ReadInputStuffFile();


    /**
     * Function ProcessStuffFile
     * gets footprint info from each line in the Stuff File by Ref Desg
     * @param aFilename The file to read from.
     * @param aSetFielsAttributeToVisible = true to set the footprint field flag to visible
     * @return bool - true if success, else true.
     */
    bool                           ProcessStuffFile( FILE* aFilename,
                                                     bool  aSetFielsAttributeToVisible );

    bool                           SaveEEFile( SCH_SCREEN* screen, int FileSave );
    SCH_SCREEN*                    CreateNewScreen( SCH_SCREEN* OldScreen, int TimeStamp );

    // General search:

    /**
     * Function FindSchematicItem
     * finds a string in the schematic.
     * @param pattern The text to search for, either in value, reference or
     *         elsewhere.
     * @param SearchType:  0 => Search is made in current sheet
     *                     1 => the whole hierarchy
     *                     2 => or for the next item
     * @param mouseWarp If true, then move the mouse cursor to the item.
     */
    SCH_ITEM*                      FindSchematicItem( const wxString& pattern,
                                                      int             SearchType,
                                                      bool            mouseWarp = true );

    SCH_ITEM*                      FindMarker( int SearchType );

private:
    void                           Process_Move_Item( SCH_ITEM* DrawStruct, wxDC* DC );
    void                           OnExit( wxCommandEvent& event );
    void                           OnAnnotate( wxCommandEvent& event );
    void                           OnErc( wxCommandEvent& event );
    void                           OnCreateNetlist( wxCommandEvent& event );
    void                           OnCreateBillOfMaterials( wxCommandEvent& event );
    void                           OnFindItems( wxCommandEvent& event );
    void                           OnLoadFile( wxCommandEvent& event );
    void                           OnLoadStuffFile( wxCommandEvent& event );
    void                           OnNewProject( wxCommandEvent& event );
    void                           OnLoadProject( wxCommandEvent& event );
    void                           OnOpenPcbnew( wxCommandEvent& event );
    void                           OnOpenCvpcb( wxCommandEvent& event );
    void                           OnOpenLibraryViewer( wxCommandEvent& event );
    void                           OnOpenLibraryEditor( wxCommandEvent& event );

    /* User interface update event handlers. */
    void                    OnUpdateBlockSelected( wxUpdateUIEvent& event );
    void                    OnUpdatePaste( wxUpdateUIEvent& event );
    void                    OnUpdateSchematicUndo( wxUpdateUIEvent& event );
    void                    OnUpdateSchematicRedo( wxUpdateUIEvent& event );
    void                    OnUpdateGrid( wxUpdateUIEvent& event );
    void                    OnUpdateUnits( wxUpdateUIEvent& event );
    void                    OnUpdateSelectCursor( wxUpdateUIEvent& event );
    void                    OnUpdateHiddenPins( wxUpdateUIEvent& event );
    void                    OnUpdateBusOrientation( wxUpdateUIEvent& event );

    // Bus Entry
    DrawBusEntryStruct*            CreateBusEntry( wxDC* DC, int entry_type );
    void                           SetBusEntryShape( wxDC*               DC,
                                                     DrawBusEntryStruct* BusEntry,
                                                     int                 entry_type );
    int                            GetBusEntryShape( DrawBusEntryStruct* BusEntry );
    void                           StartMoveBusEntry( DrawBusEntryStruct* DrawLibItem, wxDC* DC );

    // NoConnect
    DrawNoConnectStruct*           CreateNewNoConnectStruct( wxDC* DC );

    // Junction
    DrawJunctionStruct*            CreateNewJunctionStruct( wxDC*          DC,
                                                            const wxPoint& pos,
                                                            bool           PutInUndoList = FALSE );

    // Text ,label, glabel
    SCH_TEXT*                      CreateNewText( wxDC* DC, int type );
    void                           EditSchematicText( SCH_TEXT* TextStruct, wxDC* DC );
    void                           ChangeTextOrient( SCH_TEXT* TextStruct, wxDC* DC );
    void                           StartMoveTexte( SCH_TEXT* TextStruct, wxDC* DC );
    void                           ConvertTextType( SCH_TEXT* Text, wxDC* DC, int newtype );

    // Wire, Bus
    void                           BeginSegment( wxDC* DC, int type );
    void                           EndSegment( wxDC* DC );
    void                           DeleteCurrentSegment( wxDC* DC );
    void                           DeleteConnection( wxDC* DC, bool DeleteFullConnection );

    // graphic lines
    void                           Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void                           Drawing_SetNewWidth( DRAWSEGMENT* DrawSegm, wxDC* DC );
    void                           Delete_Drawings_All_Layer( DRAWSEGMENT* Segment, wxDC* DC );
    DRAWSEGMENT*                   Begin_Edge( DRAWSEGMENT* Segment, wxDC* DC );

    // Hierarchical Sheet & PinSheet
    void                           InstallHierarchyFrame( wxDC* DC, wxPoint& pos );
    DrawSheetStruct*               CreateSheet( wxDC* DC );
    void                           ReSizeSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    bool                           EditSheet( DrawSheetStruct* Sheet, wxDC* DC );

    /** Function UpdateSheetNumberAndDate
     * Set a sheet number, the sheet count for sheets in the whole schematic
     * and update the date in all screens
     */
    void                           UpdateSheetNumberAndDate();

private:
    void                           StartMoveSheet( DrawSheetStruct* sheet, wxDC* DC );
    Hierarchical_PIN_Sheet_Struct* Create_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );
    void                           Edit_PinSheet( Hierarchical_PIN_Sheet_Struct* SheetLabel,
                                                  wxDC*                          DC );
    void                           StartMove_PinSheet( Hierarchical_PIN_Sheet_Struct* SheetLabel,
                                                       wxDC*                          DC );
    void                           Place_PinSheet( Hierarchical_PIN_Sheet_Struct* SheetLabel,
                                                   wxDC*                          DC );
    Hierarchical_PIN_Sheet_Struct* Import_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    void                           DeleteSheetLabel( bool aRedraw, Hierarchical_PIN_Sheet_Struct* aSheetLabelToDel );

private:

    // Component
    SCH_COMPONENT* Load_Component( wxDC*           DC,
                                   const wxString& libname,
                                   wxArrayString&  List,
                                   bool            UseLibBrowser );
    void           StartMovePart( SCH_COMPONENT* DrawLibItem, wxDC* DC );

public:
    void           CmpRotationMiroir( SCH_COMPONENT* DrawComponent,
                                      wxDC* DC, int type_rotate );

private:
    void           SelPartUnit( SCH_COMPONENT* DrawComponent,
                                int unit, wxDC* DC );
    void           ConvertPart( SCH_COMPONENT* DrawComponent, wxDC* DC );
    void           SetInitCmp( SCH_COMPONENT* DrawComponent, wxDC* DC );
    void           EditComponentReference( SCH_COMPONENT* DrawLibItem,
                                           wxDC*          DC );
    void           EditComponentValue( SCH_COMPONENT* DrawLibItem, wxDC* DC );
    void           EditComponentFootprint( SCH_COMPONENT* DrawLibItem,
                                           wxDC*          DC );
    void           StartMoveCmpField( SCH_CMP_FIELD* Field, wxDC* DC );
    void           EditCmpFieldText( SCH_CMP_FIELD* Field, wxDC* DC );
    void           RotateCmpField( SCH_CMP_FIELD* Field, wxDC* DC );

    /* Operations sur bloc */
    void           PasteStruct( wxDC* DC );

    /* Undo - redo */
public:
    void           SaveCopyInUndoList( SCH_ITEM* ItemToCopy,
                                       int       flag_type_command = 0 );

private:
    void           PutDataInPreviousState( DrawPickedStruct* List );
    bool           GetSchematicFromRedoList();
    bool           GetSchematicFromUndoList();


public:
    void           Key( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

    /* Gestion generale des operations sur block */
    int            ReturnBlockCommand( int key );
    void           InitBlockPasteInfos();
    void           HandleBlockPlace( wxDC* DC );
    int            HandleBlockEnd( wxDC* DC );
    void           HandleBlockEndByPopUp( int Command, wxDC* DC );

    // Repetition automatique de placements
    void           RepeatDrawItem( wxDC* DC );

    // Test des points de connexion en l'air (dangling ends)
    void           TestDanglingEnds( SCH_ITEM* DrawList, wxDC* DC );
    LibDrawPin*    LocatePinEnd( SCH_ITEM* DrawList, const wxPoint& pos );

    // ERC:
    /**
     * Function CheckAnnotate
     *  Check errors relatives to annotation:
     *      components not annotated
     *      components having the same reference (duplicates)
     *      for multiple parts per package components :
     *          part number > number of parts
     *          different values between parts
     * @return errors count
     * @param aMessageList = a wxTextCtrl to display merssages. If NULL, they are displyed in a wxMessageBox
     * @param aOneSheetOnly : true = search is made only in the current sheet
     *                       false = search in whole hierarchy (usual search).
     */
    int CheckAnnotate( wxTextCtrl * aMessageList, bool aOneSheetOnly );


    DECLARE_EVENT_TABLE()
};


/*****************************/
/* class WinEDA_LibeditFrame */
/*****************************/

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
    void               OnUpdateEditingPart( wxUpdateUIEvent& event );
    void               OnUpdateNotEditingPart( wxUpdateUIEvent& event );

    void               DisplayLibInfos();
    void               RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void               OnCloseWindow( wxCloseEvent& Event );
    void               ReCreateHToolbar();
    void               ReCreateVToolbar();
    void               OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool               OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int                BestZoom();  // Retourne le meilleur zoom
    void               SetToolbars();
    void               OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    SCH_SCREEN*        GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }
    void               OnHotKey( wxDC* DC, int hotkey,
                                 EDA_BaseStruct* DrawStruct );

    void               GeneralControle( wxDC*   DC,
                                        wxPoint MousePositionInPixels );

    void               LoadSettings();
    void               SaveSettings();

private:

    // General:
    void               CreateNewLibraryPart();
    void               DeleteOnePart();
    void               SaveOnePartInMemory();
    void               SelectActiveLibrary();
    bool               LoadOneLibraryPart();
    void               SaveActiveLibrary();

    int                LoadOneLibraryPartAux( EDA_LibComponentStruct* LibEntry,
                                              LibraryStruct*          Library,
                                              int                     noMsg = 0 );

    void               DisplayCmpDoc( const wxString& Name );
    void               InstallLibeditFrame();

    // General editing
public:
    void               SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy, int flag_type_command = 0 );
    void               InstallFieldsEditorDialog( void );

private:
    bool               GetComponentFromUndoList();
    bool               GetComponentFromRedoList();

    // Edition des Pins:
    void               CreatePin( wxDC* DC );
    void               DeletePin( wxDC*                   DC,
                                  EDA_LibComponentStruct* LibEntry,
                                  LibDrawPin*             Pin );
    void               StartMovePin( wxDC* DC );

    // Test des pins ( duplicates...)
    bool               TestPins( EDA_LibComponentStruct* LibEntry );

    // Edition de l'ancre
    void               PlaceAncre();

    // Edition des graphismes:
    LibEDA_BaseStruct* CreateGraphicItem( EDA_LibComponentStruct * LibEntry, wxDC* DC );
    void               GraphicItemBeginDraw( wxDC* DC );
    void               StartMoveDrawSymbol( wxDC* DC );
    void               EndDrawGraphicItem( wxDC* DC );
    void               LoadOneSymbol();
    void               SaveOneSymbol();
    void               EditGraphicSymbol( wxDC* DC, LibEDA_BaseStruct* DrawItem );
    void               EditSymbolText( wxDC* DC, LibEDA_BaseStruct* DrawItem );
    void               RotateSymbolText( wxDC* DC );
    void               DeleteDrawPoly( wxDC* DC );
    LibDrawField*      LocateField( EDA_LibComponentStruct* LibEntry );
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

    void               DeletePartInLib( LibraryStruct* Library, EDA_LibComponentStruct* Entry );
    void               PlacePin( wxDC* DC );
    void               InitEditOnePin();
    void               GlobalSetPins( wxDC* DC, LibDrawPin* MasterPin, int id );

    // Repetition automatique de placement de pins
    void               RepeatPinItem( wxDC* DC, LibDrawPin* Pin );

protected:
    wxString m_ConfigPath;
    wxString m_LastLibImportPath;
    wxString m_LastLibExportPath;

    DECLARE_EVENT_TABLE()
};


/************************************************************************************************/
/************************************************************************************************/
class LibraryStruct;
class WinEDA_ViewlibFrame : public WinEDA_DrawFrame
{
private:
    WinEDAChoiceBox*    SelpartBox;

    // List of libraries (for selection
    wxSashLayoutWindow* m_LibListWindow;                // The redimensionnable window to display the lib list
    wxListBox*          m_LibList;                      // The list of libs
    wxSize m_LibListSize;                               // size of the window

    // List of components in the selected library
    wxSashLayoutWindow* m_CmpListWindow;                // The redimensionnable window to display the component list
    wxListBox*          m_CmpList;                      // The list of components
    wxSize       m_CmpListSize;                         // size of the window

    // Flags
    wxSemaphore* m_Semaphore;           // != NULL if the frame must emulate a modal dialog
    wxString     m_ConfigPath;          // subpath for configuartion

public:
    WinEDA_ViewlibFrame( wxWindow*      father,
                         LibraryStruct* Library = NULL,
                         wxSemaphore*   semaphore = NULL );

    ~WinEDA_ViewlibFrame();

    void OnSize( wxSizeEvent& event );
    void OnSashDrag( wxSashEvent& event );
    void ReCreateListLib();
    void ReCreateListCmp();
    void Process_Special_Functions( wxCommandEvent& event );
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    int  BestZoom();    // Retourne le meilleur zoom
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }

    void GeneralControle( wxDC* DC, wxPoint MousePositionInPixels );

    void LoadSettings();
    void SaveSettings();

private:
    void SelectCurrentLibrary();
    void SelectAndViewLibraryPart( int option );
    void ExportToSchematicLibraryPart( wxCommandEvent& event );
    void ViewOneLibraryContent( LibraryStruct* Lib, int Flag );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    DECLARE_EVENT_TABLE()
};

#endif  // WX_EESCHEMA_STRUCT_H
