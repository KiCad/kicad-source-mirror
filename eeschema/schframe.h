/*****************************************************************************
 *
 * schframe.h
 *
 * Header for class definition of WinEDA_SchematicFrame.  This is the main
 * window for EESchema.
 *
 *****************************************************************************/

#ifndef _SCHFRAME_H_
#define _SCHFRAME_H_

class WinEDA_DrawFrame;

class WinEDA_SchematicFrame : public WinEDA_DrawFrame
{
public:
    WinEDAChoiceBox* m_SelPartBox;
    DrawSheetPath*   m_CurrentSheet; //which sheet we are presently working on.
private:
    wxMenu*          m_FilesMenu;

public:
    WinEDA_SchematicFrame( wxWindow* father, WinEDA_App* parent,
                           const wxString& title,
                           const wxPoint& pos, const wxSize& size,
                           long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_SchematicFrame();

    void OnCloseWindow( wxCloseEvent& Event );
    void Process_Special_Functions( wxCommandEvent& event );
    void Process_Config( wxCommandEvent& event );
    void Save_Config( wxWindow* displayframe );

    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void CreateScreens();
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void ReCreateOptToolbar();
    void ReCreateMenuBar();
    void SetToolbars();
    void OnHotKey( wxDC* DC,
                   int hotkey,
                   EDA_BaseStruct* DrawStruct );

    DrawSheetPath* GetSheet();
    virtual BASE_SCREEN* GetScreen();
    virtual void SetScreen(SCH_SCREEN* screen);
    virtual wxString GetScreenDesc();

    void InstallConfigFrame( const wxPoint& pos );

    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void OnSelectOptionToolbar( wxCommandEvent& event );
    void ToolOnRightClick( wxCommandEvent& event );
    int BestZoom(); // Retourne le meilleur zoom

    EDA_BaseStruct* SchematicGeneralLocateAndDisplay( bool IncludePin = TRUE );
    EDA_BaseStruct* SchematicGeneralLocateAndDisplay( const wxPoint& refpoint,
                                                      bool IncludePin );

    EDA_BaseStruct* FindComponentAndItem( const wxString& component_reference,
                                          bool Find_in_hierarchy,
                                          int SearchType,
                                          const wxString& text_to_find,
                                          bool mouseWarp );

    /* Cross probing with pcbnew */
    void SendMessageToPCBNEW( EDA_BaseStruct* objectToSync,
                              SCH_COMPONENT* LibItem );

    /* netlist generation */
    void* BuildNetListBase();

    // FUnctions used for hierarchy handling
    void InstallPreviousSheet();
    void InstallNextScreen( DrawSheetStruct* Sheet );

    void ToPlot_PS( wxCommandEvent& event );
    void ToPlot_HPGL( wxCommandEvent& event );
    void ToPostProcess( wxCommandEvent& event );

    void Save_File( wxCommandEvent& event );
    void SaveProject();
    int LoadOneEEProject( const wxString& FileName, bool IsNew );
    bool LoadOneEEFile(SCH_SCREEN* screen, const wxString& FullFileName );
    bool SaveEEFile( SCH_SCREEN* screen, int FileSave );
    SCH_SCREEN * CreateNewScreen(SCH_SCREEN * OldScreen, int TimeStamp);

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
    EDA_BaseStruct* FindSchematicItem( const wxString& pattern,
                                       int SearchType,
                                       bool mouseWarp = true );

    EDA_BaseStruct* FindMarker( int SearchType );

private:
    void Process_Move_Item( EDA_BaseStruct* DrawStruct, wxDC* DC );
    void OnExit( wxCommandEvent& event );
    void OnAnnotate ( wxCommandEvent& event );
    void OnErc( wxCommandEvent& event );
    void OnCreateNetlist( wxCommandEvent& event );
    void OnCreateBillOfMaterials( wxCommandEvent& event );
    void OnFindItems( wxCommandEvent& event );
    void OnLoadFile( wxCommandEvent& event );
    void OnNewProject( wxCommandEvent& event );
    void OnLoadProject( wxCommandEvent& event );
    void OnOpenPcbnew( wxCommandEvent& event );
    void OnOpenCvpcb( wxCommandEvent& event );
    void OnOpenLibraryViewer( wxCommandEvent& event );
    void OnOpenLibraryEditor( wxCommandEvent& event );


    // Bus Entry
    DrawBusEntryStruct* CreateBusEntry( wxDC* DC, int entry_type );
    void SetBusEntryShape( wxDC* DC,
                           DrawBusEntryStruct* BusEntry,
                           int entry_type );
    int GetBusEntryShape( DrawBusEntryStruct* BusEntry );
    void StartMoveBusEntry( DrawBusEntryStruct* DrawLibItem, wxDC* DC );

    // NoConnect
    EDA_BaseStruct* CreateNewNoConnectStruct( wxDC* DC );

    // Junction
    DrawJunctionStruct* CreateNewJunctionStruct( wxDC*  DC,
                                                 const wxPoint& pos,
                                                 bool PutInUndoList = FALSE );

    // Text ,label, glabel
    EDA_BaseStruct* CreateNewText( wxDC* DC, int type );
    void EditSchematicText( SCH_TEXT* TextStruct, wxDC* DC );
    void ChangeTextOrient( SCH_TEXT* TextStruct, wxDC* DC );
    void StartMoveTexte( SCH_TEXT* TextStruct, wxDC* DC );
    void ConvertTextType( SCH_TEXT* Text, wxDC* DC, int newtype );

    // Wire, Bus
    void BeginSegment( wxDC* DC, int type );
    void EndSegment( wxDC* DC );
    void DeleteCurrentSegment( wxDC* DC );
    void DeleteConnection( wxDC* DC, bool DeleteFullConnection );

    // graphic lines
    void Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void Drawing_SetNewWidth( DRAWSEGMENT* DrawSegm, wxDC* DC );
    void Delete_Drawings_All_Layer( DRAWSEGMENT* Segment, wxDC* DC );
    DRAWSEGMENT* Begin_Edge( DRAWSEGMENT* Segment, wxDC* DC );

    // Hierarchical Sheet & PinSheet
    void InstallHierarchyFrame( wxDC* DC, wxPoint& pos );
    DrawSheetStruct* CreateSheet( wxDC* DC );
    void ReSizeSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    bool EditSheet( DrawSheetStruct* Sheet, wxDC* DC );
    /** Function UpdateSheetNumberAndDate
     * Set a sheet number, the sheet count for sheets in the whole schematic
     * and update the date in all screens
     */
    void UpdateSheetNumberAndDate();

private:
    void StartMoveSheet( DrawSheetStruct* sheet, wxDC* DC );
    DrawSheetLabelStruct* Create_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );
    void Edit_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    void StartMove_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    void Place_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    DrawSheetLabelStruct* Import_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    void DeleteSheetLabel( wxDC* DC, DrawSheetLabelStruct* SheetLabelToDel );

private:

    // Component
    SCH_COMPONENT* Load_Component( wxDC*           DC,
                                            const wxString& libname,
                                            wxArrayString&  List,
                                            bool            UseLibBrowser );
    void StartMovePart( SCH_COMPONENT* DrawLibItem, wxDC* DC );

public:
    void CmpRotationMiroir( SCH_COMPONENT* DrawComponent,
                            wxDC* DC, int type_rotate );

private:
    void SelPartUnit( SCH_COMPONENT* DrawComponent,
                      int unit, wxDC* DC );
    void ConvertPart( SCH_COMPONENT* DrawComponent, wxDC* DC );
    void SetInitCmp( SCH_COMPONENT* DrawComponent, wxDC* DC );
    void EditComponentReference( SCH_COMPONENT* DrawLibItem,
                                 wxDC* DC );
    void EditComponentValue( SCH_COMPONENT* DrawLibItem, wxDC* DC );
    void EditComponentFootprint( SCH_COMPONENT* DrawLibItem,
                                 wxDC* DC );
    void StartMoveCmpField( PartTextStruct* Field, wxDC* DC );
    void EditCmpFieldText( PartTextStruct* Field, wxDC* DC );
    void RotateCmpField( PartTextStruct* Field, wxDC* DC );

    /* Operations sur bloc */
    void PasteStruct( wxDC* DC );

    /* Undo - redo */
public:
    void SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                             int             flag_type_command = 0 );

private:
    void PutDataInPreviousState( DrawPickedStruct* List );
    bool GetSchematicFromRedoList();
    bool GetSchematicFromUndoList();


public:
    void Key( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

    /* Gestion generale des operations sur block */
    int ReturnBlockCommand( int key );
    void InitBlockPasteInfos();
    void HandleBlockPlace( wxDC* DC );
    int HandleBlockEnd( wxDC* DC );
    void HandleBlockEndByPopUp( int Command, wxDC* DC );

    // Repetition automatique de placements
    void RepeatDrawItem( wxDC* DC );

    // Test des points de connexion en l'air (dangling ends)
    void TestDanglingEnds( EDA_BaseStruct* DrawList, wxDC* DC );
    LibDrawPin* LocatePinEnd( EDA_BaseStruct* DrawList, const wxPoint& pos );

    DECLARE_EVENT_TABLE()
};

#endif  /* _SCHFRAME_H_ */
