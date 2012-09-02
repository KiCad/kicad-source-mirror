/**
 * @file class_DisplayFootprintsFrame.h
 */


#include <wxBasePcbFrame.h>


class CVPCB_MAINFRAME;


/**
 * Class DISPLAY_FOOTPRINTS_FRAME
 * is used to display footprints.
 */
class DISPLAY_FOOTPRINTS_FRAME : public PCB_BASE_FRAME
{
public:
    DISPLAY_FOOTPRINTS_FRAME( CVPCB_MAINFRAME* father, const wxString& title,
                              const wxPoint& pos, const wxSize& size,
                              long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~DISPLAY_FOOTPRINTS_FRAME();

    void    OnCloseWindow( wxCloseEvent& Event );
    void    RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void    ReCreateHToolbar();
    void    ReCreateVToolbar();
    void    ReCreateOptToolbar();
    void    RecreateMenuBar();

    void OnSelectOptionToolbar( wxCommandEvent& event );

    void OnUpdateTextDrawMode( wxUpdateUIEvent& aEvent );
    void OnUpdateLineDrawMode( wxUpdateUIEvent& aEvent );

    /**
     * Function InitDisplay
     * Refresh the full display for this frame:
     * Set the title, the status line and redraw the canvas
     * Must be called after the footprint to display is modifed
     */
    void InitDisplay();

    /**
     * Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool IsGridVisible() const;

    /**
     * Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * if you want to store/retrieve the grid visibility in configuration.
     * @param aVisible = true if the grid must be shown
     */
    virtual void SetGridVisibility( bool aVisible );
    /**
     * Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual EDA_COLOR_T GetGridColor() const;

    void    OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void    OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    bool    OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void    GeneralControl( wxDC* DC, const wxPoint& aPosition, int aHotKey = 0 );
    void    InstallOptionsDisplay( wxCommandEvent& event );
    MODULE* Get_Module( const wxString& CmpName );

    void    Process_Settings( wxCommandEvent& event );
    void    Show3D_Frame( wxCommandEvent& event );

    /* SaveCopyInUndoList() virtual
     * currently: do nothing in CvPcb.
     * but but be defined because it is a pure virtual in PCB_BASE_FRAME
     */
    virtual void SaveCopyInUndoList( BOARD_ITEM* aItemToCopy,
                                     UNDO_REDO_T aTypeCommand = UR_UNSPECIFIED,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) )
    {
    }


    /**
     * Function SaveCopyInUndoList (overloaded).
     * Creates a new entry in undo list of commands.
     * add a list of pickers to handle a list of items
     * @param aItemsList = the list of items modified by the command to undo
     * @param aTypeCommand = command type (see enum UNDO_REDO_T)
     * @param aTransformPoint = the reference point of the transformation,
     *                          for commands like move
     */
    virtual void SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                     UNDO_REDO_T aTypeCommand,
                                     const wxPoint& aTransformPoint = wxPoint( 0, 0 ) )
    {
        // currently: do nothing in CvPcb.
    }


    DECLARE_EVENT_TABLE()
};
