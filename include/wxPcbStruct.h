/***********************************************************/
/*                      wxPcbStruct.h:                     */
/*      Classes used in pcbnew, cvpcb and gerbview         */
/***********************************************************/

#ifndef  WXPCB_STRUCT_H
#define  WXPCB_STRUCT_H


#include <vector>


#ifndef PCB_INTERNAL_UNIT
#define PCB_INTERNAL_UNIT       10000
#endif


/*  Forward declarations of classes. */
class WinEDA_DrawPanel;
class WinEDA_DrawFrame;

#include "base_struct.h"

class PCB_SCREEN;
class WinEDA_GerberFrame;       // GERBER viewer main frame
class WinEDA_Toolbar;
class WinEDA_CvpcbFrame;
class WinEDA_PcbFrame;
class WinEDA_ModuleEditFrame;

// Used but not defined here:
class BOARD;
class TEXTE_PCB;
class MODULE;
class TRACK;
class SEGZONE;
class SEGVIA;
class D_PAD;
class TEXTE_MODULE;
class MIREPCB;
class COTATION;
class EDGE_MODULE;
class WinEDA3D_DrawFrame;
class DRC;
class ZONE_CONTAINER;


/******************************************************************/
/* class WinEDA_BasePcbFrame: Basic class for pcbnew and gerbview */
/******************************************************************/

class WinEDA_BasePcbFrame : public WinEDA_DrawFrame
{
public:
    BOARD* m_Pcb;

    bool   m_DisplayPadFill;        // How show pads
    bool   m_DisplayPadNum;         // show pads numbers

    int    m_DisplayModEdge;        // How show module drawings
    int    m_DisplayModText;        // How show module texts
    bool   m_DisplayPcbTrackFill;   /* FALSE : tracks are show in sketch mode, TRUE = filled */
    WinEDA3D_DrawFrame* m_Draw3DFrame;

protected:
    GENERAL_COLLECTOR*  m_Collector;


public:
    WinEDA_BasePcbFrame( wxWindow* father, WinEDA_App* parent, int idtype,
                         const wxString& title,
                         const wxPoint& pos, const wxSize& size,
                         long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_BasePcbFrame();

    /**
     * Function SetBOARD
     * sets the m_Pcb member in such as way as to ensure deleting any previous
     * BOARD.
     * @param aBoard The BOARD to put into the frame.
     */
    void                SetBOARD( BOARD* aBoard );

    // General
    virtual void    OnCloseWindow( wxCloseEvent& Event ) = 0;
    virtual void    Process_Special_Functions( wxCommandEvent& event ) = 0;
    virtual void    RedrawActiveWindow( wxDC* DC, bool EraseBg ) = 0;
    virtual void    ReCreateHToolbar() = 0;
    virtual void    ReCreateVToolbar() = 0;
    virtual void    OnLeftClick( wxDC* DC, const wxPoint& MousePos )  = 0;
    virtual void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) = 0;
    virtual void    ReCreateMenuBar();

    virtual BASE_SCREEN* GetScreen() { return (BASE_SCREEN*) m_CurrentScreen; }
    int             BestZoom();

    void            Show3D_Frame( wxCommandEvent& event );

    virtual void    GeneralControle( wxDC* DC, wxPoint Mouse );

    // Undo and redo functions
public:
    virtual void    SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                        int             flag_type_command = 0 );

private:
    virtual void    GetComponentFromUndoList();
    virtual void    GetComponentFromRedoList();


public:

    // Read/write fonctions:
    EDA_BaseStruct*             ReadDrawSegmentDescr( FILE* File, int* LineNum );
    int                         ReadListeSegmentDescr( FILE* File,
                                                       TRACK* PtSegm, int StructType,
                                                       int* LineNum, int NumSegm );

    int                         ReadSetup( FILE* File, int* LineNum );
    int                         ReadGeneralDescrPcb( FILE* File, int* LineNum );

    // PCB handling
    bool                        Clear_Pcb( bool query );

    /**
     * Function PcbGeneralLocateAndDisplay
     * searches for an item under the mouse cursor.
     * Items are searched first on the current working layer.
     * If nothing found, an item will be searched without layer restriction.  If
     * more than one item is found meeting the current working layer criterion, then
     * a popup menu is shown which allows the user to pick which item he/she is
     * interested in.  Once an item is chosen, then it is make the "current item"
     * and the status window is updated to reflect this.
     *
     * @param aHotKeyCode The hotkey which relates to the caller and determines the
     *  type of search to be performed.  If zero, then the mouse tools will be
     *  tested instead.
     */
    BOARD_ITEM*                 PcbGeneralLocateAndDisplay( int aHotKeyCode = 0 );

    void                        ProcessItemSelection( wxCommandEvent& event );

    /**
     * Function SetCurItem
     * sets the currently selected item and displays it in the MsgPanel.
     * If the given item is NULL then the MsgPanel is erased and there is no
     * currently selected item. This function is intended to make the process
     * of "selecting" an item more formal, and to indivisibly tie the operation
     * of selecting an item to displaying it using BOARD_ITEM::Display_Infos().
     * @param aItem The BOARD_ITEM to make the selected item or NULL if none.
     */
    void                        SetCurItem( BOARD_ITEM* aItem );
    BOARD_ITEM*                 GetCurItem();

    /**
     * Function GetCollectorsGuide
     * @return GENERAL_COLLECTORS_GUIDE - that considers the global configuration options.
     */
    GENERAL_COLLECTORS_GUIDE    GetCollectorsGuide();


    /**
     * Function CursorGoto
     * positions the cursor at a given coordinate and reframes the drawing if the
     * requested point is out of view.
     * @param aPos The point to go to.
     */
    void                        CursorGoto(  const wxPoint& aPos );


    /* Place un repere sur l'ecran au point de coordonnees PCB pos */
    void                        place_marqueur( wxDC* DC, const wxPoint& pos, char* pt_bitmap,
                                                int DrawMode, int color, int type );

    // Gestion des modules
    void                        InstallModuleOptionsFrame( MODULE* Module,
                                                           wxDC* DC, const wxPoint& pos );
    MODULE*                     Copie_Module( MODULE* module );
    MODULE*                     Exchange_Module( wxWindow* winaff,
                                                 MODULE*                       old_module,
                                                 MODULE*                       new_module );
    int                         Save_1_Module( const wxString& LibName, MODULE* Module,
                                               bool Overwrite, bool DisplayDialog );
    void                        Archive_Modules( const wxString& LibName, bool NewModulesOnly );
    MODULE*                     Select_1_Module_From_BOARD( BOARD* Pcb );
    MODULE*                     GetModuleByName();

    // Modules (footprints)
    MODULE*                     Create_1_Module( wxDC* DC, const wxString& module_name );
    void                        Edit_Module( MODULE* module, wxDC* DC );
    void                        Rotate_Module( wxDC*   DC,
                                               MODULE* module,
                                               int     angle,
                                               bool    incremental );
    void                        Place_Module( MODULE* module, wxDC* DC );
    void                        InstallExchangeModuleFrame( MODULE* ExchangeModuleModule,
                                                            wxDC* DC, const wxPoint& pos );

    // module texts
    void                        RotateTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void                        DeleteTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void                        PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    void                        StartMoveTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    TEXTE_MODULE*               CreateTextModule( MODULE* Module, wxDC* DC );

    void                        InstallPadOptionsFrame( D_PAD* pad, wxDC* DC, const wxPoint& pos );
    void                        InstallTextModOptionsFrame( TEXTE_MODULE* TextMod,
                                                            wxDC* DC, const wxPoint& pos );

    // Pads sur modules
    void                        AddPad( MODULE* Module, wxDC* DC );
    void                        DeletePad( D_PAD* Pad, wxDC* DC );
    void                        StartMovePad( D_PAD* Pad, wxDC* DC );
    void                        RotatePad( D_PAD* Pad, wxDC* DC );
    void                        PlacePad( D_PAD* Pad, wxDC* DC );
    void                        Export_Pad_Settings( D_PAD* pt_pad );
    void                        Import_Pad_Settings( D_PAD* pt_pad, wxDC* DC );
    void                        Global_Import_Pad_Settings( D_PAD* Pad, wxDC* DC );


    // loading footprints
    MODULE*                     Get_Librairie_Module( wxWindow* winaff,
                                                      const wxString&               library,
                                                      const wxString&               ModuleName,
                                                      bool                          show_msg_err );

    wxString                    Select_1_Module_From_List(
        WinEDA_DrawFrame* active_window, const wxString& Library,
        const wxString& Mask, const wxString& KeyWord );

    MODULE*                     Load_Module_From_Library( const wxString& library, wxDC* DC );

    //  ratsnest functions
    void                        Compile_Ratsnest( wxDC* DC, bool affiche ); /* Recalcul complet du chevelu */
    void                        ReCompile_Ratsnest_After_Changes( wxDC* DC );
    int                         Test_1_Net_Ratsnest( wxDC* DC, int net_code );
    char*                       build_ratsnest_module( wxDC* DC, MODULE* Module );
    void                        trace_ratsnest_module( wxDC* DC );
    void                        Build_Board_Ratsnest( wxDC* DC );
    void                        DrawGeneralRatsnest( wxDC* DC, int net_code = 0 );
    void                        trace_ratsnest_pad( wxDC* DC );
    void                        recalcule_pad_net_code(); /* compute and update the PAD net codes */
    void                        build_liste_pads();
    int*                        build_ratsnest_pad( EDA_BaseStruct* ref,
                                                    const wxPoint&                         refpos,
                                                    bool                                   init );

    void                        Tst_Ratsnest( wxDC* DC, int ref_netcode );
    void                        test_connexions( wxDC* DC );
    void                        test_1_net_connexion( wxDC* DC, int net_code );
    void                        reattribution_reference_piste( int affiche );

    // Plotting
    void                        ToPlotter( wxCommandEvent& event );
    void                        Plot_Serigraphie( int format_plot, FILE* File, int masque_layer );
    void                        Genere_GERBER( const wxString& FullFileName, int Layer,
                                               bool PlotOriginIsAuxAxis );
    void                        Genere_HPGL( const wxString& FullFileName, int Layer );
    void                        Genere_PS( const wxString& FullFileName, int Layer, bool useA4 );
    void                        Plot_Layer_HPGL( FILE* File, int masque_layer,
                                                 int garde, int tracevia, int modetrace );
    void                        Plot_Layer_GERBER( FILE* File, int masque_layer,
                                                   int garde, int tracevia );
    int                         Gen_D_CODE_File( FILE* file );
    void                        Plot_Layer_PS( FILE* File, int masque_layer,
                                               int garde, int tracevia, int modetrace );

    /* Block operations: */
    void                        Block_Delete( wxDC* DC );
    void                        Block_Rotate( wxDC* DC );
    void                        Block_Invert( wxDC* DC );
    void                        Block_Move( wxDC* DC );
    void                        Block_Duplicate( wxDC* DC );



    // layerhandling:
    // (See pcbnew/sel_layer.cpp for description of why null_layer parameter is provided)
    int                         SelectLayer( int default_layer, int min_layer, int max_layer,
                                             bool null_layer = false );
    void                        SelectLayerPair();
    virtual void                SwitchLayer( wxDC* DC, int layer );

    // divers
    void                        AddHistory( int value, KICAD_T type ); // Add value in data list history
    void                        InstallGridFrame( const wxPoint& pos );

    DECLARE_EVENT_TABLE()
};


/*****************************************************/
/* class WinEDA_PcbFrame: public WinEDA_BasePcbFrame */
/*****************************************************/
class WinEDA_PcbFrame : public WinEDA_BasePcbFrame
{
public:
    WinEDAChoiceBox* m_SelLayerBox;
    WinEDAChoiceBox* m_SelTrackWidthBox;
    WinEDAChoiceBox* m_SelViaSizeBox;

private:
    bool             m_SelTrackWidthBox_Changed;
    bool             m_SelViaSizeBox_Changed;
    wxMenu*          m_FilesMenu;

    DRC*             m_drc;         ///< the DRC controller, see drc.cpp


    // we'll use lower case function names for private member functions.
    void    createPopUpMenuForZones( ZONE_CONTAINER* edge_zone, wxMenu* aPopMenu );
    void    createPopUpMenuForFootprints( MODULE* aModule, wxMenu* aPopMenu );
    void    createPopUpMenuForFpTexts( TEXTE_MODULE* aText, wxMenu* aPopMenu );
    void    createPopUpMenuForFpPads( D_PAD* aPad, wxMenu* aPopMenu );
    void    createPopupMenuForTracks( TRACK* aTrack, wxMenu* aPopMenu );
    void    createPopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu );
    void    createPopUpBlockMenu( wxMenu* menu );

public:
    WinEDA_PcbFrame( wxWindow* father, WinEDA_App* parent, const wxString& title,
                     const wxPoint& pos, const wxSize& size,
                     long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_PcbFrame();

    // Configurations:
    void                InstallConfigFrame( const wxPoint& pos );
    void                Process_Config( wxCommandEvent& event );
    void                Update_config( wxWindow* displayframe );
    void                OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );
    bool                OnHotkeyDeleteItem( wxDC* DC, EDA_BaseStruct* DrawStruct );

    void                OnCloseWindow( wxCloseEvent& Event );
    void                Process_Special_Functions( wxCommandEvent& event );

    void                ProcessMuWaveFunctions( wxCommandEvent& event );
    void                MuWaveCommand( wxDC* DC, const wxPoint& MousePos );

    void                RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void                ReCreateHToolbar();
    void                ReCreateAuxiliaryToolbar();
    void                ReCreateVToolbar();
    void                ReCreateAuxVToolbar();
    void                ReCreateOptToolbar();
    void                ReCreateMenuBar();
    WinEDAChoiceBox*    ReCreateLayerBox( WinEDA_Toolbar* parent );
    void                PrepareLayerIndicator();
    void                OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void                OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    /**
     * Function OnRightClick
     * populates a popup menu with the choices appropriate for the current context.
     * The caller will add the ZOOM menu choices afterwards.
     * @param aMousePos The current mouse position
     * @param aPopMenu The menu to add to.
     */
    bool                OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu );

    void                OnSelectOptionToolbar( wxCommandEvent& event );
    void                ToolOnRightClick( wxCommandEvent& event );

    /* Gestion generale des operations sur block */
    int                 ReturnBlockCommand( int key );
    void                HandleBlockPlace( wxDC* DC );
    int                 HandleBlockEnd( wxDC* DC );

    void                SetToolbars();
    void                Process_Settings( wxCommandEvent& event );
    void                InstallPcbOptionsFrame( const wxPoint& pos, wxDC* DC, int id );
    void                InstallPcbGlobalDeleteFrame( const wxPoint& pos );

    void                GenModulesPosition( wxCommandEvent& event );
    void                GenModuleReport( wxCommandEvent& event );
    void                InstallDrillFrame( wxCommandEvent& event );
    void                ToPostProcess( wxCommandEvent& event );

    void                Files_io( wxCommandEvent& event );
    int                 LoadOnePcbFile( const wxString& FileName, wxDC* DC, bool Append );
    int                 ReadPcbFile( wxDC* DC, FILE* File, bool Append );
    bool                SavePcbFile( const wxString& FileName );
    int                 SavePcbFormatAscii( FILE* File );
    bool                WriteGeneralDescrPcb( FILE* File );
    bool                RecreateCmpFileFromBoard();

    void                ExportToGenCAD( wxCommandEvent& event );

    /**
     * Function ExporttoSPECCTRA
     * will export the current BOARD to a specctra dsn file.  See
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void                ExportToSpecctra( wxCommandEvent& event );

    /**
     * Function ImportSpecctraSession
     * will import a specctra *.ses file and use it to relocate MODULEs and
     * to replace all vias and tracks in an existing and loaded BOARD.
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void                ImportSpecctraSession( wxCommandEvent& event );

    /**
     * Function ImportSpecctraDesign
     * will import a specctra *.dsn file and use it to replace an entire BOARD.
     * The new board will not have any graphics, only components, tracks and vias.
     * See http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf for the
     * specification.
     */
    void                ImportSpecctraDesign( wxCommandEvent& event );

    /**
     * Function Access_to_External_Tool
     * Run an external tool (like freeroute )
    */
    void Access_to_External_Tool( wxCommandEvent& event );

    /* Fonctions specifiques */
    MODULE*             ListAndSelectModuleName();
    void                Liste_Equipot( wxCommandEvent& event );
    void                Swap_Layers( wxCommandEvent& event );
    void                Install_Test_DRC_Frame( wxDC* DC );
    void                Trace_Pcb( wxDC* DC, int mode );

    // Handling texts on the board
    void                Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    TEXTE_PCB*          Create_Texte_Pcb( wxDC* DC );
    void                Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB,
                                                    wxDC* DC, const wxPoint& pos );

    // Graphic Segments type DRAWSEGMENT
    void                Start_Move_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );
    void                Place_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );

    // Footprint edition (see also WinEDA_BasePcbFrame)
    void                StartMove_Module( MODULE* module, wxDC* DC );
    bool                Delete_Module( MODULE* module, wxDC* DC );

    // loading modules: see WinEDA_BasePcbFrame

    // Board handling
    void                Erase_Zones( bool query );
    void                Erase_Segments_Pcb( bool is_edges, bool query );
    void                Erase_Pistes( wxDC* DC, int masque_type, bool query );
    void                Erase_Modules( bool query );
    void                Erase_Textes_Pcb( bool query );
    void                Erase_Marqueurs();
    void                UnDeleteItem( wxDC* DC );
    void                RemoveStruct( BOARD_ITEM* Item, wxDC* DC );
    void                Via_Edit_Control( wxDC* DC, int command_type, SEGVIA* via );

    // Hightlight functions:
    int                 Select_High_Light( wxDC* DC );
    void                Hight_Light( wxDC* DC );
    void                DrawHightLight( wxDC* DC, int NetCode );

    // Track and via edition:
    void                DisplayTrackSettings();

    /**
     * Function Other_Layer_Route
     * operates in one of two ways.  If argument track is NULL, then swap the active
     * layer between m_Route_Layer_TOP and m_Route_Layer_BOTTOM.  If a track is
     * in progress (track is not NULL), and if DRC allows it, place a via on the end
     * of the current track, and then swap the current active layer and start a new
     * segment on the new layer.
     * @param track A TRACK* to append the via to or NULL.
     * @param DC A device context to draw on.
     * @return bool - true if the operation was successful, else false such as
     *   the case where DRC would not allow a via.
     */
    bool                Other_Layer_Route( TRACK* track, wxDC* DC );
    void                Affiche_PadsNoConnect( wxDC* DC );
    void                Affiche_Status_Net( wxDC* DC );
    TRACK*              Delete_Segment( wxDC* DC, TRACK* Track );
    void                Delete_Track( wxDC* DC, TRACK* Track );
    void                Delete_net( wxDC* DC, TRACK* Track );
    void                Remove_One_Track( wxDC* DC, TRACK* pt_segm );
    bool                Resize_Pistes_Vias( wxDC* DC, bool Track, bool Via );
    void                Edit_Net_Width( wxDC* DC, int Netcode );
    void                Edit_Track_Width( wxDC* DC, TRACK* Track );
    int                 Edit_TrackSegm_Width( wxDC* DC, TRACK* segm );
    TRACK*              Begin_Route( TRACK* track, wxDC* DC );
    void                End_Route( TRACK* track, wxDC* DC );
    void                ExChange_Track_Layer( TRACK* pt_segm, wxDC* DC );
    void                Attribut_Segment( TRACK* track, wxDC* DC, bool Flag_On );
    void                Attribut_Track( TRACK* track, wxDC* DC, bool Flag_On );
    void                Attribut_net( wxDC* DC, int net_code, bool Flag_On );
    void                Start_MoveOneNodeOrSegment( TRACK* track, wxDC* DC, int command );
    bool                PlaceDraggedTrackSegment( TRACK* Track, wxDC* DC );
    bool				MergeCollinearTracks( TRACK* track, wxDC* DC, int end );
    void                Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC* DC );
    void                SwitchLayer( wxDC* DC, int layer );
    int                 Add_45_degrees_Segment( wxDC* DC, TRACK* pt_segm );
    bool                Genere_Pad_Connexion( wxDC* DC, int layer );

    // zone handling

    /** Function Delete_Zone_Fill
     * Remove the zone filling which include the segment aZone, or the zone which have the given time stamp.
     *  A zone is a group of segments which have the same TimeStamp
     * @param DC = current Device Context (can be NULL)
     * @param aZone = zone segment within the zone to delete. Can be NULL
     * @param aTimestamp = Timestamp for the zone to delete, used if aZone == NULL
     */
    void                Delete_Zone_Fill( wxDC* DC, SEGZONE* Track, long aTimestamp = 0 );



    /** Function Delete_LastCreatedCorner
     * Used only while creating a new zone outline
     * Remove and delete the current outline segment in progress
     * @return 0 if no corner in list, or corner number
     */
    int Delete_LastCreatedCorner( wxDC* DC);

    /**
     * Function Begin_Zone
     * initiates a zone edge creation process,
     * or terminates the current zone edge and creates a new zone edge stub
     */
    int          Begin_Zone( wxDC* DC );

    /**
     * Function End_Zone
     * terminates (if no DRC error ) the zone edge creation process
     * @param DC = current Device Context
     * @return true if Ok, false if DRC error
     */
    bool                End_Zone( wxDC* DC );

    /** Function Fill_Zone()
     *  Calculate the zone filling for the outline zone_container
     *  The zone outline is a frontier, and can be complex (with holes)
     *  The filling starts from starting points like pads, tracks.
     * If exists the old filling is removed
     * @param DC = current Device Context
     * @param zone_container = zone to fill
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int                 Fill_Zone( wxDC* DC, ZONE_CONTAINER* zone_container, bool verbose = TRUE );

    /** Function Fill_All_Zones()
     *  Fill all zones on the board
     * The old fillings are removed
     * @param frame = reference to the main frame
     * @param DC = current Device Context
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int                 Fill_All_Zones( wxDC* DC, bool verbose = TRUE );


    /**
     * Function Add_Zone_Cutout
     * Add a cutout zone to a given zone outline
     * @param DC = current Device Context
     * @param zone_container = parent zone outline
     */
    void                Add_Zone_Cutout( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Add_Similar_Zone
     * Add a zone to a given zone outline.
     * if the zones are overlappeing they will be merged
     * @param DC = current Device Context
     * @param zone_container = parent zone outline
     */
    void                Add_Similar_Zone( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Edit_Zone_Params
     * Edit params (layer, clearance, ...) for a zone outline
     */
    void                Edit_Zone_Params( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Start_Move_Zone_Corner
     * Prepares a move corner in a zone outline,
     * called from a move corner command (IsNewCorner = false),
     * or a create new cornet command (IsNewCorner = true )
     */
    void                Start_Move_Zone_Corner( wxDC*           DC,
                                                ZONE_CONTAINER* zone_container,
                                                int             corner_id,
                                                bool            IsNewCorner );

    /**
     * Function Start_Move_Zone_Corner
     * Prepares a drag edge in an existing zone outline,
     */
    void                Start_Move_Zone_Drag_Outline_Edge( wxDC*           DC,
                                                ZONE_CONTAINER* zone_container,
                                                int             corner_id );

    /**
     * Function End_Move_Zone_Corner_Or_Outlines
     * Terminates a move corner in a zone outline, or a move zone outlines
     * @param DC = current Device Context (can be NULL)
     * @param zone_container: the given zone
     */
    void                End_Move_Zone_Corner_Or_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function End_Move_Zone_Corner_Or_Outlines
     * Remove the currently selected corner in a zone outline
     * the .m_CornerSelection is used as corner selection
     */
    void                Remove_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container );

    /** Function Delete_Zone
     * Remove the zone which include the segment aZone, or the zone which have the given time stamp.
     *  A zone is a group of segments which have the same TimeStamp
     * @param DC = current Device Context (can be NULL)
     * @param zone_container = zone to modify
     *  the member .m_CornerSelection is used to find the outline to remove.
     * if the outline is the main outline, all the zone is removed
     * otherwise, the hole is deleted
     */
    void                Delete_Zone_Contour( wxDC* DC, ZONE_CONTAINER* zone_container );

    /**
     * Function Start_Move_Zone_Outlines
     * Initialise parametres to move an existing zone outlines.
     * @param DC = current Device Context (can be NULL)
     * @param zone_container: the given zone to move
     */
    void 				Start_Move_Zone_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container );

   // Target handling
    MIREPCB*            Create_Mire( wxDC* DC );
    void                Delete_Mire( MIREPCB* MirePcb, wxDC* DC );
    void                StartMove_Mire( MIREPCB* MirePcb, wxDC* DC );
    void                Place_Mire( MIREPCB* MirePcb, wxDC* DC );
    void                InstallMireOptionsFrame( MIREPCB* MirePcb, wxDC* DC, const wxPoint& pos );

    // Graphic segments type DRAWSEGMENT handling:
    DRAWSEGMENT*        Begin_DrawSegment( DRAWSEGMENT* Segment, int shape, wxDC* DC );
    void                End_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void                Drawing_SetNewWidth( DRAWSEGMENT* DrawSegm, wxDC* DC );
    void                Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void                Delete_Drawings_All_Layer( DRAWSEGMENT* Segment, wxDC* DC );

    // Dimension handling:
    void                Install_Edit_Cotation( COTATION* Cotation, wxDC* DC, const wxPoint& pos );
    COTATION*           Begin_Cotation( COTATION* Cotation, wxDC* DC );
    void                Delete_Cotation( COTATION* Cotation, wxDC* DC );


    // netlist  handling:
    void                InstallNetlistFrame( wxDC* DC, const wxPoint& pos );

    // Autoplacement:
    void                AutoPlace( wxCommandEvent& event );
    void                ReOrientModules( const wxString& ModuleMask, int Orient,
                                         bool include_fixe, wxDC* DC );
    void                FixeModule( MODULE* Module, bool Fixe );
    void                AutoMoveModulesOnPcb( wxDC* DC, bool PlaceModulesHorsPcb );
    bool                SetBoardBoundaryBoxFromEdgesOnly();
    void                AutoPlaceModule( MODULE* Module, int place_mode, wxDC* DC );
    int                 RecherchePlacementModule( MODULE* Module, wxDC* DC );
    void                GenModuleOnBoard( MODULE* Module );
    float               Compute_Ratsnest_PlaceModule( wxDC* DC );
    int                 GenPlaceBoard();
    void                DrawInfoPlace( wxDC* DC );

    // Autorouting:
    int                 Solve( wxDC* DC, int two_sides );
    void                Reset_Noroutable( wxDC* DC );
    void                Autoroute( wxDC* DC, int mode );
    void                ReadAutoroutedTracks( wxDC* DC );
    void                GlobalRoute( wxDC* DC );

    // divers
    void                Show_1_Ratsnest( EDA_BaseStruct* item, wxDC* DC );
    void                Ratsnest_On_Off( wxDC* DC );
    void                Clean_Pcb( wxDC* DC );
    BOARD_ITEM*         SaveItemEfface( BOARD_ITEM* PtItem, int nbitems );

    void                InstallFindFrame( const wxPoint& pos, wxDC* DC );

    /**
     * Function SendMessageToEESCHEMA
     * sends a message to the schematic editor so that it may move its cursor
     * to a part with the same reference as the objectToSync
     * @param objectToSync The object whose reference is used to syncronize eeschema.
     */
    void                SendMessageToEESCHEMA( BOARD_ITEM* objectToSync );

    /* Micro waves functions */
    void                Edit_Gap( wxDC* DC, MODULE* Module );
    MODULE*             Create_MuWaveBasicShape( wxDC* DC, const wxString& name, int pad_count );
    MODULE*             Create_MuWaveComponent( wxDC* DC, int shape_type );
    MODULE*             Create_MuWavePolygonShape( wxDC* DC );
    void                Begin_Self( wxDC* DC );
    MODULE*             Genere_Self( wxDC* DC );

    DECLARE_EVENT_TABLE()
};


/****************************************************/
/* class WinEDA_GerberFrame: public WinEDA_PcbFrame */
/****************************************************/

class WinEDA_GerberFrame : public WinEDA_BasePcbFrame
{
public:
    WinEDAChoiceBox* m_SelLayerBox;
    WinEDAChoiceBox* m_SelLayerTool;
private:
    wxMenu*          m_FilesMenu;

public:
    WinEDA_GerberFrame( wxWindow* father, WinEDA_App* parent, const wxString& title,
                        const wxPoint& pos, const wxSize& size,
                        long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_GerberFrame();

    void            Update_config();
    void            OnCloseWindow( wxCloseEvent& Event );
    void            Process_Special_Functions( wxCommandEvent& event );
    void            RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void            ReCreateHToolbar();
    void            ReCreateVToolbar();
    void            ReCreateOptToolbar();
    void            ReCreateMenuBar();
    void            OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void            OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool            OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int             BestZoom(); // Retourne le meilleur zoom
    void            OnSelectOptionToolbar( wxCommandEvent& event );
    void            OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );
    PCB_SCREEN*		GetPCBScreen(){ return (PCB_SCREEN*)GetScreen(); }

    EDA_BaseStruct* GerberGeneralLocateAndDisplay();
    EDA_BaseStruct* Locate( int typeloc );


    void            SetToolbars();
    void            Process_Settings( wxCommandEvent& event );
    void            Process_Config( wxCommandEvent& event );
    void            InstallConfigFrame( const wxPoint& pos );
    void            InstallPcbOptionsFrame( const wxPoint& pos, int id );
    void            InstallPcbGlobalDeleteFrame( const wxPoint& pos );

    /* handlers for block commands */
    int             ReturnBlockCommand( int key );
    virtual void    HandleBlockPlace( wxDC* DC );
    virtual int     HandleBlockEnd( wxDC* DC );

    void            InstallDrillFrame( wxCommandEvent& event );
    void            ToPostProcess( wxCommandEvent& event );
    void            Genere_HPGL( const wxString& FullFileName, int Layers );
    void            Genere_GERBER( const wxString& FullFileName, int Layers );
    void            Genere_PS( const wxString& FullFileName, int Layers );
    void            Plot_Layer_HPGL( FILE* File, int masque_layer,
                                     int garde, int tracevia, int modetrace );
    void            Plot_Layer_GERBER( FILE* File, int masque_layer,
                                       int garde, int tracevia );
    int             Gen_D_CODE_File( const wxString& Name_File );
    void            Plot_Layer_PS( FILE* File, int masque_layer,
                                   int garde, int tracevia, int modetrace );

    void            Files_io( wxCommandEvent& event );
    int             LoadOneGerberFile( const wxString& FileName, wxDC* DC, int mode );
    int             ReadGerberFile( wxDC* DC, FILE* File, bool Append );
    bool            Read_GERBER_File( wxDC*           DC,
                                      const wxString& GERBER_FullFileName,
                                      const wxString& D_Code_FullFileName );
    bool            SaveGerberFile( const wxString& FileName, wxDC* DC );
    int             Read_D_Code_File( const wxString& D_Code_FullFileName );
    void            CopyDCodesSizeToItems();
    void            Liste_D_Codes( wxDC* DC );

    /* Fonctions specifiques */
    void            Trace_Gerber( wxDC* DC, int draw_mode, int printmasklayer );

    // Copper texts
    void            Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    TEXTE_PCB*      Create_Texte_Pcb( wxDC* DC );
    void            Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void            StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void            Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );

    // PCB handling
    bool            Clear_Pcb( bool query );
    void            Erase_Current_Layer( bool query );
    void            Erase_Zones( bool query );
    void            Erase_Segments_Pcb( bool is_edges, bool query );
    void            Erase_Pistes( int masque_type, bool query );
    void            Erase_Textes_Pcb( bool query );
    void            UnDeleteItem( wxDC* DC );
    void            Delete_DCode_Items( wxDC* DC, int dcode_value, int layer_number );

    TRACK*          Begin_Route( TRACK* track, wxDC* DC );
    void            End_Route( TRACK* track, wxDC* DC );
    TRACK*          Delete_Segment( wxDC* DC, TRACK* Track );
    int             Edit_TrackSegm_Width( wxDC* DC, TRACK* segm );

    // Conversion function
    void            ExportDataInPcbnewFormat( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


/*********************************************************/
/* class WinEDA_ModuleEditFrame: public WinEDA_DrawFrame */
/* Class for the footprint editor                        */
/*********************************************************/

class WinEDA_ModuleEditFrame : public WinEDA_BasePcbFrame
{
public:
    MODULE*  CurrentModule;
    wxString m_CurrentLib;

public:
    WinEDA_ModuleEditFrame( wxWindow* father, WinEDA_App* parent,
                            const wxString& title,
                            const wxPoint& pos, const wxSize& size,
                            long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_ModuleEditFrame();

    void            InstallOptionsFrame( const wxPoint& pos );

    void            OnCloseWindow( wxCloseEvent& Event );
    void            Process_Special_Functions( wxCommandEvent& event );
    void            RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void            ReCreateHToolbar();
    void            ReCreateVToolbar();
    void            ReCreateOptToolbar();
    void            ReCreateAuxiliaryToolbar();
    void            OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void            OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool            OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void            SetToolbars();
    void            ReCreateMenuBar();
    void            ToolOnRightClick( wxCommandEvent& event );
    void            OnSelectOptionToolbar( wxCommandEvent& event );
    void            OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

    /* handlers for block commands */
    int             ReturnBlockCommand( int key );
    virtual void    HandleBlockPlace( wxDC* DC );
    virtual int     HandleBlockEnd( wxDC* DC );

    BOARD_ITEM*     ModeditLocateAndDisplay( int aHotKeyCode = 0 );

    /* Undo and redo functions */
public:
    void            SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy, int flag_type_command = 0 );

private:
    void            GetComponentFromUndoList();
    void            GetComponentFromRedoList();

public:

    // Footprint edition
    void            Place_Ancre( MODULE* module, wxDC* DC );
    void            RemoveStruct( EDA_BaseStruct* Item, wxDC* DC );
    void            Transform( MODULE* module, wxDC* DC, int transform );

    // loading Footprint
    MODULE*         Import_Module( wxDC* DC );
    void            Export_Module( MODULE* ptmod, bool createlib );
    void            Load_Module_Module_From_BOARD( MODULE* Module );

    // functions to edit footprint edges
    void            Edit_Edge_Width( EDGE_MODULE* Edge, wxDC* DC );
    void            Edit_Edge_Layer( EDGE_MODULE* Edge, wxDC* DC );
    void            Delete_Edge_Module( EDGE_MODULE* Edge, wxDC* DC );
    EDGE_MODULE*    Begin_Edge_Module( EDGE_MODULE* Edge, wxDC* DC, int type_edge );
    void            End_Edge_Module( EDGE_MODULE* Edge, wxDC* DC );
    void            Enter_Edge_Width( EDGE_MODULE* Edge, wxDC* DC );
    void            Start_Move_EdgeMod( EDGE_MODULE* drawitem, wxDC* DC );
    void            Place_EdgeMod( EDGE_MODULE* drawitem, wxDC* DC );

    // handlers for libraries:
    void            Delete_Module_In_Library( const wxString& libname );
    int             Create_Librairie( const wxString& LibName );
    void            Select_Active_Library();

    DECLARE_EVENT_TABLE()
};

#endif  /* WXPCB_STRUCT_H */
