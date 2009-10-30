/***********************************************************/
/*                      wxGerberStruct.h:                     */
/***********************************************************/

#ifndef  WX_GERBER_STRUCT_H
#define  WX_GERBER_STRUCT_H


#include "id.h"


/**
 * Command IDs for the gerber file viewer.
 *
 * Please add IDs that are unique to the gerber file viewer here and not in the
 * global id.h file.  This will prevent the entire project from being rebuilt
 * when adding new command to the gerber file viewer.
 */
enum id_gerbview_frm
{
    ID_GERBVIEW_SHOW_LIST_DCODES,
    ID_GERBVIEW_LOAD_DRILL_FILE,
    ID_GERBVIEW_LOAD_DCODE_FILE,
    ID_TOOLBARH_GERBER_SELECT_TOOL,
    ID_MENU_INC_LAYER_AND_APPEND_FILE,
    ID_INC_LAYER_AND_APPEND_FILE,
    ID_GERBVIEW_SHOW_SOURCE,
    ID_GERBVIEW_EXPORT_TO_PCBNEW,
    ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
};


/******************************************************************
class WinEDA_GerberFrame: this is the main window used in gerbview
******************************************************************/

class WinEDA_GerberFrame : public WinEDA_BasePcbFrame
{
public:
    WinEDAChoiceBox* m_SelLayerBox;
    WinEDAChoiceBox* m_SelLayerTool;

public:
    WinEDA_GerberFrame( wxWindow* father, const wxString& title,
                        const wxPoint& pos, const wxSize& size,
                        long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_GerberFrame();

    void         Update_config();
    void         OnCloseWindow( wxCloseEvent& Event );
    void         Process_Special_Functions( wxCommandEvent& event );
    void         RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void         ReCreateHToolbar();
    void         ReCreateVToolbar();
    void         ReCreateOptToolbar();
    void         ReCreateMenuBar();
    void         OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void         OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool         OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    int          BestZoom();    // Retourne le meilleur zoom
    void         OnSelectOptionToolbar( wxCommandEvent& event );
    void         OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct );

    BOARD_ITEM*  GerberGeneralLocateAndDisplay();
    BOARD_ITEM*  Locate( int typeloc );


    void         SetToolbars();
    void         Process_Settings( wxCommandEvent& event );
    void         Process_Config( wxCommandEvent& event );
    void         InstallConfigFrame( const wxPoint& pos );
    void         InstallGerberOptionsFrame( const wxPoint& pos, int id );
    void         InstallPcbGlobalDeleteFrame( const wxPoint& pos );

    /* handlers for block commands */
    int          ReturnBlockCommand( int key );
    virtual void HandleBlockPlace( wxDC* DC );
    virtual int  HandleBlockEnd( wxDC* DC );

    /* Block operations: */
    /**
     * Function Block_Delete
     * deletes all tracks and segments within the selected block.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void         Block_Delete( wxDC* DC );
    void         Block_Rotate( wxDC* DC );
    void         Block_Invert( wxDC* DC );

    /**
     * Function Block_Move
     * moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void         Block_Move( wxDC* DC );

    /**
     * Function Block_Mirror_X
     * mirrors all tracks and segments within the currently selected block
     * in the X axis.
     *
     * @param DC A device context to draw on.
     */
    void         Block_Mirror_X( wxDC* DC );
    /**
     * Function Block_Duplicate
     * copies-and-moves all tracks and segments within the selected block.
     * New location is determined by the current offset from the selected
     * block's original location.
     * Defined separately in pcbnew and gerbview
     *
     * @param DC A device context to draw on.
     */
    void         Block_Duplicate( wxDC* DC );

    void         InstallDrillFrame( wxCommandEvent& event );
    void         ToPostProcess( wxCommandEvent& event );
    void         Genere_HPGL( const wxString& FullFileName, int Layers );
    void         Genere_GERBER( const wxString& FullFileName, int Layers );
    void         Genere_PS( const wxString& FullFileName, int Layers );
    void         Plot_Layer_HPGL( FILE* File, int masque_layer,
                                  int garde, bool trace_via,
                                  GRTraceMode trace_mode );
    void         Plot_Layer_GERBER( FILE* File, int masque_layer,
                                    int garde, bool trace_via,
                                    GRTraceMode trace_mode );
    int          Gen_D_CODE_File( const wxString& Name_File );
    void         Plot_Layer_PS( FILE* File, int masque_layer,
                                int garde, bool trace_via,
                                GRTraceMode trace_mode );

    void         Files_io( wxCommandEvent& event );
    void         OnFileHistory( wxCommandEvent& event );
    bool         LoadOneGerberFile( const wxString& FileName, wxDC* DC,
                                    int mode );
    int          ReadGerberFile( wxDC* DC, FILE* File, bool Append );
    bool         Read_GERBER_File( wxDC*           DC,
                                   const wxString& GERBER_FullFileName,
                                   const wxString& D_Code_FullFileName );
    bool         SaveGerberFile( const wxString& FileName, wxDC* DC );

    void         GeneralControle( wxDC* DC, wxPoint Mouse );


    /**
     * Function Read_D_Code_File
     * reads in a dcode file assuming ALSPCB file format with ';' indicating
     * comments.
     * <p>
     * Format is like CSV but with optional ';' delineated comments:<br>
     * tool,     Horiz,       Vert,   drill, vitesse, acc. ,Type ; [DCODE (commentaire)]<br>
     * ex:     1,         12,       12,     0,        0,     0,   3 ; D10
     * <p>
     * Format:<br>
     * Ver,  Hor, Type, Tool [,Drill]<br>
     * example: 0.012, 0.012,  L   , D10<br>
     *
     * Categorize all found dcodes into a table of D_CODE instantiations.
     * @param D_CodeFullFileName The name of the file to read from.
     * @return int - <br>
     *                 -1 = file not found<br>
     *                 -2 = parsing problem<br>
     *                  0 = the \a D_Code_FullFileName is empty, no reading
     *                      is done but an empty GERBER is put into
     *                      g_GERBER_List[]<br>
     *                  1 = read OK<br>
     */
    int          Read_D_Code_File( const wxString& D_Code_FullFileName );
    void         CopyDCodesSizeToItems();
    void         Liste_D_Codes( wxDC* DC );

    /* Fonctions specifiques */
    void         Trace_Gerber( wxDC* DC, int draw_mode, int printmasklayer );

    // Copper texts
    void         Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    TEXTE_PCB*   Create_Texte_Pcb( wxDC* DC );
    void         Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void         StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC );
    void         Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC );

    // PCB handling
    bool         Clear_Pcb( bool query );
    void         Erase_Current_Layer( bool query );
    void         Erase_Zones( bool query );
    void         Erase_Segments_Pcb( bool is_edges, bool query );
    void         Erase_Pistes( int masque_type, bool query );
    void         Erase_Textes_Pcb( bool query );
    void         Delete_DCode_Items( wxDC* DC, int dcode_value, int layer_number );

    TRACK*       Begin_Route( TRACK* track, wxDC* DC );
    void         End_Route( TRACK* track, wxDC* DC );
    TRACK*       Delete_Segment( wxDC* DC, TRACK* Track );
    int          Edit_TrackSegm_Width( wxDC* DC, TRACK* segm );

    // Conversion function
    void         ExportDataInPcbnewFormat( wxCommandEvent& event );

    /* SaveCopyInUndoList() virtual
     * currently: do nothing in gerbview.
     * but but be defined because it is a pure virtual in WinEDA_BasePcbFrame
     */
    virtual void SaveCopyInUndoList(
        BOARD_ITEM* aItemToCopy,
        UndoRedoOpType aTypeCommand = UR_UNSPECIFIED,
        const wxPoint& aTransformPoint = wxPoint(0,0) ) { }

    /** Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UndoRedoOpType)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    virtual void SaveCopyInUndoList(
        PICKED_ITEMS_LIST& aItemsList,
        UndoRedoOpType aTypeCommand,
        const wxPoint& aTransformPoint = wxPoint(0,0) )
    {
        // currently: do nothing in gerbview.
    }


    DECLARE_EVENT_TABLE()
};

#endif  /* WX_GERBER_STRUCT_H */
