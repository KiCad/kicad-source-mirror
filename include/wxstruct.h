/***********************************************************/
/*                      wxstruct.h:                        */
/* descriptions des principales classes derivees utilisees */
/***********************************************************/

#ifndef  WXSTRUCT_H
#define  WXSTRUCT_H


#ifndef eda_global
#define eda_global extern
#endif

#include <wx/socket.h>
#include "wx/log.h"
#include "wx/config.h"
#include <wx/wxhtml.h>
#include <wx/laywin.h>
#include <wx/snglinst.h>

#include <vector>


#define INTERNAL_UNIT_TYPE      0        // Internal unit = inch

#ifndef EESCHEMA_INTERNAL_UNIT
#define EESCHEMA_INTERNAL_UNIT  1000
#endif

#ifndef PCB_INTERNAL_UNIT
#define PCB_INTERNAL_UNIT       10000
#endif

//  Option for dialog boxes
// #define DIALOG_STYLE wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT|wxSTAY_ON_TOP
#define DIALOG_STYLE wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT | MAYBE_RESIZE_BORDER

#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE|wxWANTS_CHARS
#define EDA_DRAW_PANEL wxScrolledWindow

class wxMyDialogModalData;

/*  Forward declarations of classes. */
class WinEDA_DrawPanel;
class WinEDA_DrawFrame;

#include "base_struct.h"

class WinEDA_App;
class WinEDA_MsgPanel;
class COMMAND;
class WinEDA_MainFrame;
class BASE_SCREEN;
class SCH_SCREEN;
class PCB_SCREEN;
class WinEDA_SchematicFrame;    // Schematic main frame
class WinEDA_LibeditFrame;      // Component creation and edition main frame
class WinEDA_ViewlibFrame;      // Component viewer main frame
class WinEDA_GerberFrame;       // GERBER viewer main frame
class WinEDA_Toolbar;
class WinEDA_CvpcbFrame;
class WinEDA_PcbFrame;
class WinEDA_ModuleEditFrame;
class WinEDAChoiceBox;
#define WinEDA_MenuBar  wxMenuBar
#define WinEDA_Menu     wxMenu
#define WinEDA_MenuItem wxMenuItem

// Used but not defined here:
class LibraryStruct;
class EDA_LibComponentStruct;
class LibEDA_BaseStruct;
class EDA_BaseStruct;
class DrawBusEntryStruct;
class DrawGlobalLabelStruct;
class DrawTextStruct;
class EDA_DrawLineStruct;
class DrawSheetStruct;
class DrawSheetLabelStruct;
class EDA_SchComponentStruct;
class LibDrawField;
class PartTextStruct;
class LibDrawPin;
class DrawJunctionStruct;
class BOARD;
class TEXTE_PCB;
class MODULE;
class TRACK;
class SEGZONE;
class SEGVIA;
class EDGE_ZONE;
class D_PAD;
class TEXTE_MODULE;
class MIREPCB;
class DRAWSEGMENT;
class COTATION;
class EDGE_MODULE;
class WinEDA3D_DrawFrame;
class PARAM_CFG_BASE;
class Ki_PageDescr;
class Ki_HotkeyInfo;
class GENERAL_COLLECTOR;
class GENERAL_COLLECTORS_GUIDE;


enum id_librarytype {
    LIBRARY_TYPE_EESCHEMA,
    LIBRARY_TYPE_PCBNEW,
    LIBRARY_TYPE_DOC
};

enum id_drawframe {
    NOT_INIT_FRAME = 0,
    SCHEMATIC_FRAME,
    LIBEDITOR_FRAME,
    VIEWER_FRAME,
    PCB_FRAME,
    MODULE_EDITOR_FRAME,
    CVPCB_FRAME,
    CVPCB_DISPLAY_FRAME,
    GERBER_FRAME,
    TEXT_EDITOR_FRAME,
    DISPLAY3D_FRAME,
    KICAD_MAIN_FRAME
};

enum id_toolbar {
    TOOLBAR_MAIN = 1,       // Main horizontal Toolbar
    TOOLBAR_TOOL,           // Rigth vertical Toolbar (list of tools)
    TOOLBAR_OPTION,         // Left vertical Toolbar (option toolbar
    TOOLBAR_AUX       		// Secondary horizontal Toolbar
};


/**********************/
/* Classes pour WXWIN */
/**********************/

#define MSG_PANEL_DEFAULT_HEIGHT  ( 28 )    // height of the infos display window

/**********************************************/
/*  Class representing the entire Application */
/**********************************************/
#include "appl_wxstruct.h"


/******************************************************************/
/* Basic frame for kicad, eeschema, pcbnew and gerbview.          */
/* not directly used: the real frames are derived from this class */
/******************************************************************/

class WinEDA_BasicFrame : public wxFrame
{
public:
    int             m_Ident;        // Id Type (pcb, schematic, library..)
    WinEDA_App*     m_Parent;
    wxPoint         m_FramePos;
    wxSize          m_FrameSize;
    int             m_MsgFrameHeight;

    WinEDA_MenuBar* m_MenuBar;      // menu du haut d'ecran
    WinEDA_Toolbar* m_HToolBar;     // Standard horizontal Toolbar
    bool            m_FrameIsActive;
    wxString        m_FrameName;    // name used for writting and reading setup
                                    // It is "SchematicFrame", "PcbFrame" ....

public:

    // Constructor and destructor
    WinEDA_BasicFrame( wxWindow* father, int idtype, WinEDA_App* parent,
                       const wxString& title,
                       const wxPoint& pos, const wxSize& size,
					   long style = KICAD_DEFAULT_DRAWFRAME_STYLE);
#ifdef KICAD_PYTHON
    WinEDA_BasicFrame( const WinEDA_BasicFrame& ) { }   // Should throw!!
    WinEDA_BasicFrame() { }                             // Should throw!!
#endif
    ~WinEDA_BasicFrame();

    void            GetKicadHelp( wxCommandEvent& event );
    void            GetKicadAbout( wxCommandEvent& event );
    void            PrintMsg( const wxString& text );
    void            GetSettings();
    void            SaveSettings();
    int             WriteHotkeyConfigFile( const wxString&                        Filename,
                                           struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                           bool                                   verbose );
    int             ReadHotkeyConfigFile( const wxString&                        Filename,
                                          struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                          bool                                   verbose );
    void            SetLanguage( wxCommandEvent& event );
    void            ProcessFontPreferences( int id );

    wxString        GetLastProject( int rang );
    void            SetLastProject( const wxString& FullFileName );
    void            DisplayActivity( int PerCent, const wxString& Text );
    virtual void    ReCreateMenuBar();
};


/*******************************************************/
/* Basic draw frame for eeschema, pcbnew and gerbview. */
/*******************************************************/

class WinEDA_DrawFrame : public WinEDA_BasicFrame
{
public:
    WinEDA_DrawPanel* DrawPanel;            // Draw area
    WinEDA_MsgPanel*  MsgPanel;             // Zone d'affichage de caracteristiques
    WinEDA_Toolbar*   m_VToolBar;           // Vertical (right side) Toolbar
    WinEDA_Toolbar*   m_AuxVToolBar;        // Auxiliary Vertical (right side) Toolbar
    WinEDA_Toolbar*   m_OptionsToolBar;     // Options Toolbar (left side)
    WinEDA_Toolbar*   m_AuxiliaryToolBar;   // Toolbar auxiliaire (utilis� dans pcbnew)

    WinEDAChoiceBox*  m_SelGridBox;         // Dialog box to choose the grid size
    WinEDAChoiceBox*  m_SelZoomBox;         // Dialog box to choose the Zoom value
    int m_ZoomMaxValue;                     // Max zoom value: Draw min scale is 1/m_ZoomMaxValue

    BASE_SCREEN*      m_CurrentScreen;      // current used SCREEN

    int     m_CurrentCursorShape;           // shape for cursor (0 = default cursor)
    int     m_ID_current_state;             // Id of active button on the vertical toolbar
    int     m_HTOOL_current_state;          // Id of active button on horizontal toolbar

    int     m_InternalUnits;                // nombre d'unites internes pour 1 pouce
                                            // = 1000 pour schema, = 10000 pour PCB

    int     m_UnitType;                     // Internal Unit type (0 = inch)
    bool    m_Draw_Axis;                    // TRUE pour avoir les axes dessines
    bool    m_Draw_Grid;                    // TRUE pour avoir la grille dessinee
    bool    m_Draw_Sheet_Ref;               // TRUE pour avoir le cartouche dessin�

    bool    m_Print_Sheet_Ref;              // TRUE pour avoir le cartouche imprim�
    bool    m_Draw_Auxiliary_Axis;          // TRUE pour avoir les axes auxiliaires dessines
    wxPoint m_Auxiliary_Axis_Position;  /* origine de l'axe auxiliaire (app:
                                         *  dans la generation les fichiers de positionnement
                                         *  des composants) */

public:

    // Constructor and destructor
    WinEDA_DrawFrame( wxWindow* father, int idtype, WinEDA_App* parent,
                      const wxString& title,
                      const wxPoint& pos, const wxSize& size,
					  long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_DrawFrame();

    BASE_SCREEN* GetScreen() { return m_CurrentScreen; }

    void            OnMenuOpen( wxMenuEvent& event );
    void            OnMouseEvent( wxMouseEvent& event );
    virtual void    OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );
    void            AddFontSelectionMenu( wxMenu* main_menu );
    void            ProcessFontPreferences( wxCommandEvent& event );


    void            Affiche_Message( const wxString& message );
    void            EraseMsgBox();
    void            Process_PageSettings( wxCommandEvent& event );
    void            SetDrawBgColor( int color_num );
    virtual void    SetToolbars();
    void            SetLanguage( wxCommandEvent& event );
    virtual void    ReCreateHToolbar() = 0;
    virtual void    ReCreateVToolbar() = 0;
    virtual void    ReCreateMenuBar();
    virtual void    ReCreateAuxiliaryToolbar();
    void            SetToolID( int id, int new_cursor_id, const wxString& title );

    virtual void    OnSelectGrid( wxCommandEvent& event );
    virtual void    OnSelectZoom( wxCommandEvent& event );

    virtual void    GeneralControle( wxDC* DC, wxPoint Mouse );
    virtual void    OnSize( wxSizeEvent& event );
    void            OnEraseBackground( wxEraseEvent& SizeEvent );

//  void OnChar(wxKeyEvent& event);
    void            SetToolbarBgColor( int color_num );
    void            OnZoom( int zoom_type );
    void            OnPanning( int direction );
    void            OnGrid( int grid_type );
    void            Recadre_Trace( bool ToMouse );
    void            PutOnGrid( wxPoint* coord ); /* set the coordiante "coord" to the nearest grid coordinate */
    void            Zoom_Automatique( bool move_mouse_cursor );

    /* Set the zoom level to show the area Rect */
    void            Window_Zoom( EDA_Rect& Rect );

    /* Return the zoom level which displays the full page on screen */
    virtual int     BestZoom() = 0;

    /* Return the current zoom level */
	int             GetZoom(void);

    void            ToPrinter( wxCommandEvent& event );
    void            SVG_Print( wxCommandEvent& event );

    void            OnActivate( wxActivateEvent& event );
    void            ReDrawPanel();
    void            TraceWorkSheet( wxDC* DC, BASE_SCREEN* screen, int line_width );
    void            DisplayToolMsg( const wxString msg );
    void            Process_Zoom( wxCommandEvent& event );
    void            Process_Grid( wxCommandEvent& event );
    virtual void    RedrawActiveWindow( wxDC* DC, bool EraseBg ) = 0;
    virtual void    Process_Special_Functions( wxCommandEvent& event ) = 0;
    virtual void    OnLeftClick( wxDC* DC, const wxPoint& MousePos )   = 0;
    virtual void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    virtual bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) = 0;
    virtual void    ToolOnRightClick( wxCommandEvent& event );
    void            AdjustScrollBars();
    void            Affiche_Status_Box(); /* Affichage des coord curseur, zoom .. */
    void            DisplayUnitsMsg();

    /* Handlers for block commands */
    virtual int     ReturnBlockCommand( int key );
    virtual void    InitBlockPasteInfos();
    virtual bool    HandleBlockBegin( wxDC* DC, int cmd_type, const wxPoint& startpos );
    virtual void    HandleBlockPlace( wxDC* DC );
    virtual int     HandleBlockEnd( wxDC* DC );

    void            CopyToClipboard( wxCommandEvent& event );

    /* interprocess communication */
    void            OnSockRequest( wxSocketEvent& evt );
    void            OnSockRequestServer( wxSocketEvent& evt );
};

#define COMMON_EVENTS_DRAWFRAME \
    EVT_MOUSEWHEEL( WinEDA_DrawFrame::OnMouseEvent ) \
    EVT_MENU_OPEN( WinEDA_DrawFrame::OnMenuOpen ) \
    EVT_ACTIVATE( WinEDA_DrawFrame::OnActivate )


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

    PCB_SCREEN* GetScreen() { return (PCB_SCREEN*) m_CurrentScreen; }
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
    int                         ReadListeSegmentDescr( wxDC* DC, FILE* File,
                                                       TRACK* PtSegm, int StructType,
                                                       int* LineNum, int NumSegm );

    int                         ReadSetup( FILE* File, int* LineNum );
    int                         ReadGeneralDescrPcb( wxDC* DC, FILE* File, int* LineNum );

    // PCB handling
    bool                        Clear_Pcb( wxDC* DC, bool query );

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

    BOARD_ITEM*                 Locate( int typeloc, int LayerSearch );
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

    /* Place un repere sur l'ecran au point de coordonnees PCB pos */
    void                        place_marqueur( wxDC* DC, const wxPoint& pos, char* pt_bitmap,
                                                int DrawMode, int color, int type );

    // Gestion des modules
    void            InstallModuleOptionsFrame( MODULE* Module,
                                               wxDC* DC, const wxPoint& pos );
    MODULE*         Copie_Module( MODULE* module );
    MODULE*         Exchange_Module( wxWindow* winaff, MODULE* old_module, MODULE* new_module );
    int             Save_1_Module( const wxString& LibName, MODULE* Module,
                                   bool Overwrite, bool DisplayDialog );
    void            Archive_Modules( const wxString& LibName, bool NewModulesOnly );
    MODULE*         Select_1_Module_From_BOARD( BOARD* Pcb );
    MODULE*         GetModuleByName();

    // Modules (footprints)
    MODULE*         Create_1_Module( wxDC* DC, const wxString& module_name );
    void            Edit_Module( MODULE* module, wxDC* DC );
    void            Rotate_Module( wxDC* DC, MODULE* module, int angle, bool incremental );
    void            Change_Side_Module( MODULE* Module, wxDC* DC );
    void            Place_Module( MODULE* module, wxDC* DC );
    void            InstallExchangeModuleFrame( MODULE* ExchangeModuleModule,
                                                wxDC* DC, const wxPoint& pos );

    // module texts
    void            RotateTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void            DeleteTextModule( TEXTE_MODULE* Text, wxDC* DC );
    void            PlaceTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    void            StartMoveTexteModule( TEXTE_MODULE* Text, wxDC* DC );
    TEXTE_MODULE*   CreateTextModule( MODULE* Module, wxDC* DC );

    void            InstallPadOptionsFrame( D_PAD* pad, wxDC* DC, const wxPoint& pos );
    void            InstallTextModOptionsFrame( TEXTE_MODULE* TextMod,
                                                wxDC* DC, const wxPoint& pos );

    // Pads sur modules
    void            AddPad( MODULE* Module, wxDC* DC );
    void            DeletePad( D_PAD* Pad, wxDC* DC );
    void            StartMovePad( D_PAD* Pad, wxDC* DC );
    void            RotatePad( D_PAD* Pad, wxDC* DC );
    void            PlacePad( D_PAD* Pad, wxDC* DC );
    void            Export_Pad_Settings( D_PAD* pt_pad );
    void            Import_Pad_Settings( D_PAD* pt_pad, wxDC* DC );
    void            Global_Import_Pad_Settings( D_PAD* Pad, wxDC* DC );


    // loading footprints
    MODULE*         Get_Librairie_Module( wxWindow* winaff, const wxString& library,
                                          const wxString& ModuleName, bool show_msg_err );

    wxString        Select_1_Module_From_List(
        WinEDA_DrawFrame* active_window, const wxString& Library,
        const wxString& Mask, const wxString& KeyWord );

    MODULE*         Load_Module_From_Library( const wxString& library, wxDC* DC );

    //  ratsnest functions
    void            Compile_Ratsnest( wxDC* DC, bool affiche ); /* Recalcul complet du chevelu */
    void            ReCompile_Ratsnest_After_Changes( wxDC* DC );
    int             Test_1_Net_Ratsnest( wxDC* DC, int net_code );
    char*           build_ratsnest_module( wxDC* DC, MODULE* Module );
    void            trace_ratsnest_module( wxDC* DC );
    void            Build_Board_Ratsnest( wxDC* DC );
    void            DrawGeneralRatsnest( wxDC* DC, int net_code = 0 );
    void            trace_ratsnest_pad( wxDC* DC );
    void            recalcule_pad_net_code(); /* compute and update the PAD net codes */
    void            build_liste_pads();
    int*            build_ratsnest_pad( EDA_BaseStruct* ref, const wxPoint& refpos, bool init );

    void            Tst_Ratsnest( wxDC* DC, int ref_netcode );
    void            test_connexions( wxDC* DC );
    void            test_1_net_connexion( wxDC* DC, int net_code );
    void            reattribution_reference_piste( int affiche );

    // Plotting
    void            ToPlotter( wxCommandEvent& event );
    void            Plot_Serigraphie( int format_plot, FILE* File, int masque_layer );
    void            Genere_GERBER( const wxString& FullFileName, int Layer,
                                   bool PlotOriginIsAuxAxis );
    void            Genere_HPGL( const wxString& FullFileName, int Layer );
    void            Genere_PS( const wxString& FullFileName, int Layer );
    void            Plot_Layer_HPGL( FILE* File, int masque_layer,
                                     int garde, int tracevia, int modetrace );
    void            Plot_Layer_GERBER( FILE* File, int masque_layer,
                                       int garde, int tracevia );
    int             Gen_D_CODE_File( FILE* file );
    void            Plot_Layer_PS( FILE* File, int masque_layer,
                                   int garde, int tracevia, int modetrace );

    /* Block operations: */
    void            Block_Delete( wxDC* DC );
    void            Block_Rotate( wxDC* DC );
    void            Block_Invert( wxDC* DC );
    void            Block_Move( wxDC* DC );
    void            Block_Duplicate( wxDC* DC );

    // zone handling:
    void            DelLimitesZone( wxDC* DC, bool Redraw );

    // layerhandling:
    // (See pcbnew/sel_layer.cpp for description of why null_layer parameter is provided)
    int             SelectLayer( int default_layer, int min_layer, int max_layer,
                                 bool null_layer = false );
    void            SelectLayerPair();
    virtual void    SwitchLayer( wxDC* DC, int layer );

    // divers
    void            AddHistory( int value, KICAD_T type ); // Add value in data list history
    void            InstallGridFrame( const wxPoint& pos );

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

    // we'll use lower case function names for private member functions.
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

    /* Fonctions specifiques */
    MODULE*             ListAndSelectModuleName();
    void                Liste_Equipot( wxCommandEvent& event );
    void                Swap_Layers( wxCommandEvent& event );
    int                 Test_DRC( wxDC* DC, bool TestPad2Pad, bool TestZone );
    void                Install_Test_DRC_Frame( wxDC* DC );
    void                Trace_Pcb( wxDC* DC, int mode );
    void                Trace_PcbEdges( wxDC* DC, int mode_color );

    // Gestion des textes sur pcb
    void                Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    TEXTE_PCB*          Create_Texte_Pcb( wxDC* DC );
    void                Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void                InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB,
                                                    wxDC* DC, const wxPoint& pos );

    // Gestion des Segments type DRAWITEM
    void                Start_Move_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );
    void                Place_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC );

    // Gestion des modules (voir egalement WinEDA_BasePcbFrame)
    void                StartMove_Module( MODULE* module, wxDC* DC );
    bool                Delete_Module( MODULE* module, wxDC* DC );

    // loading modules: see WinEDA_BasePcbFrame

    // Gestion du PCB
    void                Erase_Zones( wxDC* DC, bool query );
    void                Erase_Segments_Pcb( wxDC* DC, bool is_edges, bool query );
    void                Erase_Pistes( wxDC* DC, int masque_type, bool query );
    void                Erase_Modules( wxDC* DC, bool query );
    void                Erase_Textes_Pcb( wxDC* DC, bool query );
    void                Erase_Marqueurs();
    void                UnDeleteItem( wxDC* DC );
    void                RemoveStruct( EDA_BaseStruct* Item, wxDC* DC );
    void                Via_Edit_Control( wxDC* DC, int command_type, SEGVIA* via );

    // Hightlight functions:
    int                 Select_High_Light( wxDC* DC );
    void                Hight_Light( wxDC* DC );
    void                DrawHightLight( wxDC* DC, int NetCode );

    // Track and via edition:
    void                DisplayTrackSettings();
    void                Other_Layer_Route( TRACK* track, wxDC* DC );
    void                Affiche_PadsNoConnect( wxDC* DC );
    void                Affiche_Status_Net( wxDC* DC );
    TRACK*              Delete_Segment( wxDC* DC, TRACK* Track );
    void                Delete_Track( wxDC* DC, TRACK* Track );
    void                Delete_net( wxDC* DC, TRACK* Track );
    void                Delete_Zone( wxDC* DC, SEGZONE* Track );
    void                Supprime_Une_Piste( wxDC* DC, TRACK* pt_segm );
    bool                Resize_Pistes_Vias( wxDC* DC, bool Track, bool Via );
    void                Edit_Zone_Width( wxDC* DC, SEGZONE* pt_ref );
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
    void                Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC* DC );
    void                SwitchLayer( wxDC* DC, int layer );

    // zone handling
    EDGE_ZONE*          Del_SegmEdgeZone( wxDC* DC, EDGE_ZONE* edge_zone );
    void                CaptureNetName( wxDC* DC );
    EDGE_ZONE*          Begin_Zone();
    void                End_Zone( wxDC* DC );
    void                Fill_Zone( wxDC* DC );

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
    bool            Clear_Pcb( wxDC* DC, bool query );
    void            Erase_Current_Layer( wxDC* DC, bool query );
    void            Erase_Zones( wxDC* DC, bool query );
    void            Erase_Segments_Pcb( wxDC* DC, bool is_edges, bool query );
    void            Erase_Pistes( wxDC* DC, int masque_type, bool query );
    void            Erase_Textes_Pcb( wxDC* DC, bool query );
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
    WinEDAChoiceBox* m_SelPartBox;
private:
    wxMenu*          m_FilesMenu;

public:
    WinEDA_SchematicFrame( wxWindow* father, WinEDA_App* parent,
                           const wxString& title,
                           const wxPoint& pos, const wxSize& size,
						   long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_SchematicFrame();

    void                                    OnCloseWindow( wxCloseEvent& Event );
    void                                    Process_Special_Functions( wxCommandEvent& event );
    void                                    Process_Config( wxCommandEvent& event );
    void                                    Save_Config( wxWindow* displayframe );

    void                                    RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void                                    ReCreateHToolbar();
    void                                    ReCreateVToolbar();
    void                                    ReCreateOptToolbar();
    void                                    ReCreateMenuBar();
    void                                    SetToolbars();
    void                                    OnHotKey( wxDC*           DC,
                                                      int             hotkey,
                                                      EDA_BaseStruct* DrawStruct );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) m_CurrentScreen; }

    void                                    InstallConfigFrame( const wxPoint& pos );

    void                                    OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void                                    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool                                    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void                                    OnSelectOptionToolbar( wxCommandEvent& event );
    void                                    ToolOnRightClick( wxCommandEvent& event );
    int                                     BestZoom(); // Retourne le meilleur zoom

    EDA_BaseStruct*                         SchematicGeneralLocateAndDisplay(
        bool IncludePin = TRUE );
    EDA_BaseStruct*                         SchematicGeneralLocateAndDisplay(
        const wxPoint& refpoint,
        bool           IncludePin );

    EDA_BaseStruct*                         FindComponentAndItem(
        const wxString& component_reference, bool Find_in_hierarchy,
        int SearchType,
        const wxString& text_to_find,
        bool mouseWarp );

    /* Cross probing with pcbnew */
    void                    SendMessageToPCBNEW( EDA_BaseStruct* objectToSync,
							EDA_SchComponentStruct* LibItem	);

    /* netlist generation */
    void*                   BuildNetListBase();

    // FUnctions used for hierarchy handling
    void                    InstallPreviousScreen();
    void                    InstallNextScreen( DrawSheetStruct* Sheet );

    void                    ToPlot_PS( wxCommandEvent& event );
    void                    ToPlot_HPGL( wxCommandEvent& event );
    void                    ToPostProcess( wxCommandEvent& event );

    void                    Save_File( wxCommandEvent& event );
    int                     LoadOneEEProject( const wxString& FileName, bool IsNew );
    bool                    LoadOneEEFile( SCH_SCREEN* screen, const wxString& FullFileName );
    bool                    SaveEEFile( SCH_SCREEN* screen, int FileSave );
    bool                    LoadOneSheet( SCH_SCREEN* screen, const wxString& FullFileName );

    // General search:

    /**
     * Function FindSchematicItem
     * finds a string in the schematic.
     * @param pattern The text to search for, either in value, reference or elsewhere.
     * @param SearchType:  0 => Search is made in current sheet
     *                     1 => the whole hierarchy
     *                     2 => or for the next item
     * @param mouseWarp If true, then move the mouse cursor to the item.
     */
    EDA_BaseStruct*         FindSchematicItem( const wxString& pattern,
                                               int                     SearchType,
                                               bool                    mouseWarp = true );

    EDA_BaseStruct*         FindMarker( int SearchType );

private:
    void                    Process_Move_Item( EDA_BaseStruct* DrawStruct, wxDC* DC );

    // Bus Entry
    DrawBusEntryStruct*     CreateBusEntry( wxDC* DC, int entry_type );
    void                    SetBusEntryShape( wxDC*               DC,
                                              DrawBusEntryStruct* BusEntry,
                                              int                 entry_type );
    int                     GetBusEntryShape( DrawBusEntryStruct* BusEntry );
    void                    StartMoveBusEntry( DrawBusEntryStruct* DrawLibItem, wxDC* DC );

    // NoConnect
    EDA_BaseStruct*         CreateNewNoConnectStruct( wxDC* DC );

    // Junction
    DrawJunctionStruct*     CreateNewJunctionStruct( wxDC*      DC,
                                                     const wxPoint& pos,
                                                     bool           PutInUndoList = FALSE );

    // Text ,label, glabel
    EDA_BaseStruct*         CreateNewText( wxDC* DC, int type );
    void                    EditSchematicText( DrawTextStruct* TextStruct, wxDC* DC );
    void                    ChangeTextOrient( DrawTextStruct* TextStruct, wxDC* DC );
    void                    StartMoveTexte( DrawTextStruct* TextStruct, wxDC* DC );
    void                    ConvertTextType( DrawTextStruct* Text, wxDC* DC, int newtype );

    // Wire, Bus
    void                    BeginSegment( wxDC* DC, int type );
    void                    EndSegment( wxDC* DC );
    void                    DeleteCurrentSegment( wxDC* DC );
    void                    DeleteConnection( wxDC* DC, bool DeleteFullConnection );

    // graphic lines
    void                    Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC );
    void                    Drawing_SetNewWidth( DRAWSEGMENT* DrawSegm, wxDC* DC );
    void                    Delete_Drawings_All_Layer( DRAWSEGMENT* Segment, wxDC* DC );
    DRAWSEGMENT*            Begin_Edge( DRAWSEGMENT* Segment, wxDC* DC );

    // Hierarchical Sheet & PinSheet
    void                    InstallHierarchyFrame( wxDC* DC, wxPoint& pos );
    DrawSheetStruct*        CreateSheet( wxDC* DC );
    void                    ReSizeSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    bool                    EditSheet( DrawSheetStruct* Sheet, wxDC* DC );

private:
    void                    StartMoveSheet( DrawSheetStruct* sheet, wxDC* DC );
    DrawSheetLabelStruct*   Create_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );
    void                    Edit_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    void                    StartMove_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    void                    Place_PinSheet( DrawSheetLabelStruct* SheetLabel, wxDC* DC );
    DrawSheetLabelStruct*   Import_PinSheet( DrawSheetStruct* Sheet, wxDC* DC );

public:
    void                    DeleteSheetLabel( wxDC* DC, DrawSheetLabelStruct* SheetLabelToDel );

private:

    // Component
    EDA_SchComponentStruct* Load_Component( wxDC*           DC,
                                            const wxString& libname,
                                            wxArrayString&  List,
                                            bool            UseLibBrowser );
    void                    StartMovePart( EDA_SchComponentStruct* DrawLibItem, wxDC* DC );

public:
    void                    CmpRotationMiroir(
        EDA_SchComponentStruct* DrawComponent, wxDC* DC, int type_rotate );

private:
    void                    SelPartUnit( EDA_SchComponentStruct* DrawComponent, int unit, wxDC* DC );
    void                    ConvertPart( EDA_SchComponentStruct* DrawComponent, wxDC* DC );
    void                    SetInitCmp( EDA_SchComponentStruct* DrawComponent, wxDC* DC );
    void                    EditComponentReference( EDA_SchComponentStruct* DrawLibItem, wxDC* DC );
    void                    EditComponentValue( EDA_SchComponentStruct* DrawLibItem, wxDC* DC );
    void                    StartMoveCmpField( PartTextStruct* Field, wxDC* DC );
    void                    EditCmpFieldText( PartTextStruct* Field, wxDC* DC );
    void                    RotateCmpField( PartTextStruct* Field, wxDC* DC );

    /* Operations sur bloc */
    void                    PasteStruct( wxDC* DC );

    /* Undo - redo */
public:
    void                    SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                                int             flag_type_command = 0 );

private:
    void                    PutDataInPreviousState( DrawPickedStruct* List );
    bool                    GetSchematicFromRedoList();
    bool                    GetSchematicFromUndoList();


public:
    void                    Key( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

    /* Gestion generale des operations sur block */
    int                     ReturnBlockCommand( int key );
    void                    InitBlockPasteInfos();
    void                    HandleBlockPlace( wxDC* DC );
    int                     HandleBlockEnd( wxDC* DC );
    void                    HandleBlockEndByPopUp( int Command, wxDC* DC );

    // Repetition automatique de placements
    void                    RepeatDrawItem( wxDC* DC );

    // Test des points de connexion en l'air (dangling ends)
    void                    TestDanglingEnds( EDA_BaseStruct* DrawList, wxDC* DC );
    LibDrawPin*             LocatePinEnd( EDA_BaseStruct* DrawList, const wxPoint& pos );

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
    WinEDA_LibeditFrame( wxWindow* father, WinEDA_App* parent,
                         const wxString& title,
                         const wxPoint& pos, const wxSize& size,
						 long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_LibeditFrame();

    void                Process_Special_Functions( wxCommandEvent& event );
    void                DisplayLibInfos();
    void                RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void                OnCloseWindow( wxCloseEvent& Event );
    void                ReCreateHToolbar();
    void                ReCreateVToolbar();
    void                OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    bool                OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int                 BestZoom(); // Retourne le meilleur zoom
    void                SetToolbars();
    void                OnLeftDClick( wxDC* DC, const wxPoint& MousePos );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) m_CurrentScreen; }
    void                OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

private:

    // General:
    void                CreateNewLibraryPart();
    void                DeleteOnePart();
    void                SaveOnePartInMemory();
    void                SelectActiveLibrary();
    bool                LoadOneLibraryPart();
    void                SaveActiveLibrary();
    void                ImportOnePart();
    void                ExportOnePart( bool create_lib );
    int                 LoadOneLibraryPartAux( EDA_LibComponentStruct* LibEntry,
                                               LibraryStruct* Library, int noMsg = 0 );

    void                DisplayCmpDoc( const wxString& Name );
    void                InstallLibeditFrame( const wxPoint& pos );

    // General editing
public:
    void                SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy, int flag_type_command = 0 );

private:
    bool                GetComponentFromUndoList();
    bool                GetComponentFromRedoList();

    // Edition des Pins:
    void                CreatePin( wxDC* DC );
    void                DeletePin( wxDC*                   DC,
                                   EDA_LibComponentStruct* LibEntry,
                                   LibDrawPin*             Pin );
    void                StartMovePin( wxDC* DC );

    // Test des pins ( duplicates...)
    bool                TestPins( EDA_LibComponentStruct* LibEntry );

    // Edition de l'ancre
    void                PlaceAncre();

    // Edition des graphismes:
    LibEDA_BaseStruct*  CreateGraphicItem( wxDC* DC );
    void                GraphicItemBeginDraw( wxDC* DC );
    void                StartMoveDrawSymbol( wxDC* DC );
    void                EndDrawGraphicItem( wxDC* DC );
    void                LoadOneSymbol( wxDC* DC );
    void                SaveOneSymbol();
    void                EditGraphicSymbol( wxDC* DC, LibEDA_BaseStruct* DrawItem );
    void                EditSymbolText( wxDC* DC, LibEDA_BaseStruct* DrawItem );
    void                RotateSymbolText( wxDC* DC );
    void                DeleteDrawPoly( wxDC* DC );
    LibDrawField*       LocateField( EDA_LibComponentStruct* LibEntry );
    void                RotateField( wxDC* DC, LibDrawField* Field );
    void                PlaceField( wxDC* DC, LibDrawField* Field );
    void                EditField( wxDC* DC, LibDrawField* Field );
    void                StartMoveField( wxDC* DC, LibDrawField* field );

public:
    /* Block commands: */
    int                 ReturnBlockCommand( int key );
    void                HandleBlockPlace( wxDC* DC );
    int                 HandleBlockEnd( wxDC* DC );

    void                DeletePartInLib( LibraryStruct* Library, EDA_LibComponentStruct* Entry );
    void                PlacePin( wxDC* DC );
    void                InitEditOnePin();
    void                GlobalSetPins( wxDC* DC, LibDrawPin* MasterPin, int id );

    // Repetition automatique de placement de pins
    void                RepeatPinItem( wxDC* DC, LibDrawPin* Pin );

    DECLARE_EVENT_TABLE()
};


class LibraryStruct;
class WinEDA_ViewlibFrame : public WinEDA_DrawFrame
{
public:
    WinEDAChoiceBox* SelpartBox;

    wxListBox*       m_LibList;
    wxSize           m_LibListSize;
    wxListBox*       m_CmpList;
    wxSize           m_CmpListSize;
    wxSemaphore*     m_Semaphore; // != NULL if the frame must emulate a modal dialog

public:
    WinEDA_ViewlibFrame( wxWindow* father, WinEDA_App* parent,
                         LibraryStruct* Library = NULL,
                         wxSemaphore* semaphore = NULL );

    ~WinEDA_ViewlibFrame();

    void    OnSize( wxSizeEvent& event );
    void    ReCreateListLib();
    void    ReCreateListCmp();
    void    Process_Special_Functions( wxCommandEvent& event );
    void    DisplayLibInfos();
    void    RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void    OnCloseWindow( wxCloseEvent& Event );
    void    ReCreateHToolbar();
    void    ReCreateVToolbar();
    void    OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    int     BestZoom(); // Retourne le meilleur zoom
    void    ClickOnLibList( wxCommandEvent& event );
    void    ClickOnCmpList( wxCommandEvent& event );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) m_CurrentScreen; }

private:
    void    SelectCurrentLibrary();
    void    SelectAndViewLibraryPart( int option );
    void    ExportToSchematicLibraryPart( wxCommandEvent& event );
    void    ViewOneLibraryContent( LibraryStruct* Lib, int Flag );
    bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    DECLARE_EVENT_TABLE()
};


/****************************************************/
/* classe representant un ecran graphique de dessin */
/****************************************************/

#include "drawpanel_wxstruct.h"


/*********************************************************
class WinEDA_MsgPanel : this is a panel to display various infos
and messages on items in eeschema an pcbnew
*********************************************************/

/**
 * Struct MsgItem
 * is used privately by WinEDA_MsgPanel as the item type of its vector.
 * These items are the pairs of text strings shown in the MsgPanel.
 */
struct MsgItem
{
    int         m_X;
    int         m_UpperY;
    int         m_LowerY;
    wxString    m_UpperText;
    wxString    m_LowerText;
    int         m_Color;

    /**
     * Function operator=
     * overload the assignment operator so that the wxStrings get copied
     * properly when copying a MsgItem.
     * No, actually I'm not sure this needed, C++ compiler may auto-generate it.
     */
    MsgItem& operator=( const MsgItem& rv )
    {
        m_X         = rv.m_X;
        m_UpperY    = rv.m_UpperY;
        m_LowerY    = rv.m_LowerY;
        m_UpperText = rv.m_UpperText;   // overloaded operator=()
        m_LowerText = rv.m_LowerText;   // overloaded operator=()
        m_Color     = rv.m_Color;
        return *this;
    }
};


class WinEDA_MsgPanel : public wxPanel
{
protected:
    std::vector<MsgItem>    m_Items;    

    void showItem( wxWindowDC& dc, const MsgItem& aItem );
    
public:
    WinEDA_DrawFrame* m_Parent;
    int m_BgColor;          // couleur de fond

public:

    // Constructor and destructor
    WinEDA_MsgPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos, const wxSize& size );
    ~WinEDA_MsgPanel();

    void    OnPaint( wxPaintEvent& event );
    void    EraseMsgBox();
    void    EraseMsgBox( wxDC* DC );
    void    Affiche_1_Parametre( int pos_X, const wxString& texte_H,
                                 const wxString& texte_L, int color );


    DECLARE_EVENT_TABLE()
};


/************************************************/
/* Class to enter a line, is some dialog frames */
/************************************************/
class WinEDA_EnterText
{
public:
    bool          m_Modify;

private:
    wxString      m_NewText;
    wxTextCtrl*   m_FrameText;
    wxStaticText* m_Title;

public:

    // Constructor and destructor
    WinEDA_EnterText( wxWindow* parent, const wxString& Title,
                      const wxString& TextToEdit, wxBoxSizer* BoxSizer,
                      const wxSize& Size );

    ~WinEDA_EnterText()
    {
    }


    wxString    GetValue();
    void        GetValue( char* buffer, int lenmax );
    void        SetValue( const wxString& new_text );
    void        Enable( bool enbl );

    void SetFocus() { m_FrameText->SetFocus(); }
    void SetInsertionPoint( int n ) { m_FrameText->SetInsertionPoint( n ); }
    void SetSelection( int n, int m )
    {
        m_FrameText->SetSelection( n, m );
    }
};

/************************************************************************/
/* Class to edit/enter a graphic text and its dimension ( INCHES or MM )*/
/************************************************************************/
class WinEDA_GraphicTextCtrl
{
public:
    int           m_Units, m_Internal_Unit;

    wxTextCtrl*   m_FrameText;
    wxTextCtrl*   m_FrameSize;
private:
    wxStaticText* m_Title;

public:

    // Constructor and destructor
    WinEDA_GraphicTextCtrl( wxWindow* parent, const wxString& Title,
                            const wxString& TextToEdit, int textsize,
                            int units, wxBoxSizer* BoxSizer, int framelen = 200,
                            int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_GraphicTextCtrl();

    wxString    GetText();
    int         GetTextSize();
    void        Enable( bool state );
    void        SetTitle( const wxString& title );

    void SetFocus() { m_FrameText->SetFocus(); }
    void        SetValue( const wxString& value );
    void        SetValue( int value );
};


/*************************************************************************************/
/*Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in dialog boxes */
/*************************************************************************************/
class WinEDA_PositionCtrl
{
public:
    int           m_Units, m_Internal_Unit;
    wxPoint       m_Pos_To_Edit;

    wxTextCtrl*   m_FramePosX;
    wxTextCtrl*   m_FramePosY;
private:
    wxStaticText* m_TextX, * m_TextY;

public:

    // Constructor and destructor
    WinEDA_PositionCtrl( wxWindow* parent, const wxString& title,
                         const wxPoint& pos_to_edit,
                         int units, wxBoxSizer* BoxSizer,
                         int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_PositionCtrl();

    void    Enable( bool x_win_on, bool y_win_on );
    void    SetValue( int x_value, int y_value );
    wxPoint GetValue();
};

/*************************************************************
Class to edit/enter a size (pair of values for X and Y size)
( INCHES or MM ) in dialog boxes
***************************************************************/
class WinEDA_SizeCtrl : public WinEDA_PositionCtrl
{
public:

    // Constructor and destructor
    WinEDA_SizeCtrl( wxWindow* parent, const wxString& title,
                     const wxSize& size_to_edit,
                     int units, wxBoxSizer* BoxSizer,
                     int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_SizeCtrl() { }
    wxSize GetValue();
};


/****************************************************************/
/* Class to edit/enter a value ( INCHES or MM ) in dialog boxes */
/****************************************************************/

/* internal_unit est le nombre d'unites internes par inch
 *  - 1000 sur EESchema
 *  - 10000 sur PcbNew
 */
class WinEDA_ValueCtrl
{
public:
    int           m_Units;
    int           m_Value;
    wxTextCtrl*   m_ValueCtrl;
private:
    int           m_Internal_Unit;
    wxStaticText* m_Text;

public:

    // Constructor and destructor
    WinEDA_ValueCtrl( wxWindow* parent, const wxString& title, int value,
                      int units, wxBoxSizer* BoxSizer,
                      int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_ValueCtrl();

    int     GetValue();
    void    SetValue( int new_value );
    void    Enable( bool enbl );

    void SetToolTip( const wxString& text )
    {
        m_ValueCtrl->SetToolTip( text );
    }
};

/************************************************************************/
/* Class to edit/enter a  pair of float (double) values in dialog boxes */
/************************************************************************/
class WinEDA_DFloatValueCtrl
{
public:
    double        m_Value;
    wxTextCtrl*   m_ValueCtrl;
private:
    wxStaticText* m_Text;

public:

    // Constructor and destructor
    WinEDA_DFloatValueCtrl( wxWindow* parent, const wxString& title,
                            double value, wxBoxSizer* BoxSizer );

    ~WinEDA_DFloatValueCtrl();

    double  GetValue();
    void    SetValue( double new_value );
    void    Enable( bool enbl );

    void SetToolTip( const wxString& text )
    {
        m_ValueCtrl->SetToolTip( text );
    }
};


/*************************/
/* class WinEDA_Toolbar */
/*************************/

class WinEDA_Toolbar : public wxToolBar
{
public:
    wxWindow*       m_Parent;
    id_toolbar      m_Ident;
    WinEDA_Toolbar* Pnext;
    bool            m_Horizontal;
    int             m_Size;

public:
    WinEDA_Toolbar( id_toolbar type, wxWindow* parent,
                    wxWindowID id, bool horizontal );
    WinEDA_Toolbar* Next() { return Pnext; }
};


/***********************/
/* class WinEDAListBox */
/***********************/

class WinEDAListBox : public wxDialog
{
public:
    WinEDA_DrawFrame* m_Parent;
    wxListBox*        m_List;
    wxTextCtrl*       m_WinMsg;
    const wxChar**    m_ItemList;

private:
    void (*m_MoveFct)( wxString & Text );

public:
    WinEDAListBox( WinEDA_DrawFrame* parent, const wxString& title,
                   const wxChar** ItemList,
                   const wxString& RefText,
                   void(* movefct)(wxString& Text) = NULL,
                   const wxColour& colour = wxNullColour,
                   wxPoint dialog_position = wxDefaultPosition );
    ~WinEDAListBox();

    void        SortList();
    void        Append( const wxString& item );
    void        InsertItems( const wxArrayString& itemlist, int position = 0 );
    void        MoveMouseToOrigin();
    wxString    GetTextSelection();

private:
    void        OnClose( wxCloseEvent& event );
    void        OnCancelClick( wxCommandEvent& event );
    void        OnOkClick( wxCommandEvent& event );
    void        ClickOnList( wxCommandEvent& event );
    void        D_ClickOnList( wxCommandEvent& event );
    void        OnKeyEvent( wxKeyEvent& event );

    DECLARE_EVENT_TABLE()
};


/*************************/
/* class WinEDAChoiceBox */
/*************************/

/* class to display a choice list.
 *  This is a wrapper to wxComboBox (or wxChoice)
 *  but because they have some problems, WinEDAChoiceBox uses workarounds:
 *  - in wxGTK 2.6.2 wxGetSelection() does not work properly,
 *  - and wxChoice crashes if compiled in non unicode mode and uses utf8 codes
 */

#define EVT_KICAD_CHOICEBOX EVT_COMBOBOX
class WinEDAChoiceBox : public wxComboBox
{
public:
    WinEDAChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0, const wxString choices[] = NULL ) :
        wxComboBox( parent, id, wxEmptyString, pos, size,
                    n, choices, wxCB_READONLY )
    {
    }


    WinEDAChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos, const wxSize& size,
                     const wxArrayString& choices ) :
        wxComboBox( parent, id, wxEmptyString, pos, size,
                    choices, wxCB_READONLY )
    {
    }


    int GetChoice()
    {
        return GetCurrentSelection();
    }
};

#endif  /* WXSTRUCT_H */
