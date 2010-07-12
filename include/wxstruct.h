/****************/
/*  wxstruct.h  */
/****************/

#ifndef  WXSTRUCT_H
#define  WXSTRUCT_H


#include <vector>

#include <wx/socket.h>
#include "wx/log.h"
#include "wx/config.h"
#include <wx/wxhtml.h>
#include <wx/laywin.h>
#include <wx/aui/aui.h>

#include "colors.h"
#include "common.h"

//C++ guarantees that operator delete checks its argument for null-ness
#ifndef SAFE_DELETE
#define SAFE_DELETE( p ) delete (p); (p) = NULL;
#endif

#ifndef EESCHEMA_INTERNAL_UNIT
#define EESCHEMA_INTERNAL_UNIT 1000
#endif

//  Option for dialog boxes
#define DIALOG_STYLE wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT | \
    MAYBE_RESIZE_BORDER

#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS

class EDA_BaseStruct;
class EDA_Rect;
class WinEDA_DrawPanel;
class WinEDA_MsgPanel;
class BASE_SCREEN;
class WinEDA_Toolbar;
class WinEDAChoiceBox;
class PARAM_CFG_BASE;
class Ki_PageDescr;
class Ki_HotkeyInfo;
class PLOTTER;

enum id_librarytype {
    LIBRARY_TYPE_EESCHEMA,
    LIBRARY_TYPE_PCBNEW,
    LIBRARY_TYPE_DOC,
    LIBRARY_TYPE_SYMBOL
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
    TOOLBAR_TOOL,           // Right vertical Toolbar (list of tools)
    TOOLBAR_OPTION,         // Left vertical Toolbar (option toolbar
    TOOLBAR_AUX             // Secondary horizontal Toolbar
};


/***********************************************/
/* Classes for basic main frames used in kicad */
/***********************************************/


/******************************************************************/
/* Basic frame for kicad, eeschema, pcbnew and gerbview.          */
/* not directly used: the real frames are derived from this class */
/******************************************************************/

class WinEDA_BasicFrame : public wxFrame
{
public:
    int             m_Ident;        // Id Type (pcb, schematic, library..)
    wxPoint         m_FramePos;
    wxSize          m_FrameSize;
    int             m_MsgFrameHeight;

    WinEDA_Toolbar* m_HToolBar;     // Standard horizontal Toolbar
    bool            m_FrameIsActive;
    wxString        m_FrameName;    // name used for writing and reading setup
                                    // It is "SchematicFrame", "PcbFrame" ....
    wxString        m_AboutTitle;   // Name of program displayed in About.

    wxAuiManager   m_auimgr;

public:
    WinEDA_BasicFrame( wxWindow* father, int idtype,
                       const wxString& title,
                       const wxPoint& pos, const wxSize& size,
                       long style = KICAD_DEFAULT_DRAWFRAME_STYLE );
    ~WinEDA_BasicFrame();

    void         GetKicadHelp( wxCommandEvent& event );
    void         GetKicadAbout( wxCommandEvent& event );
    void         PrintMsg( const wxString& text );

    virtual void LoadSettings();
    virtual void SaveSettings();

    int          WriteHotkeyConfigFile( const wxString&                        Filename,
                                        struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                        bool                                   verbose );
    int          ReadHotkeyConfigFile( const wxString&                        Filename,
                                       struct Ki_HotkeyInfoSectionDescriptor* DescList,
                                       bool                                   verbose );
    /** function SetLanguage
     * called on a language menu selection
     * when using a derived function, do not forget to call this one
     */
    virtual void SetLanguage( wxCommandEvent& event );

    wxString     GetFileFromHistory( int cmdId, const wxString& type );
    void         SetLastProject( const wxString& FullFileName );
    void         DisplayActivity( int PerCent, const wxString& Text );
    virtual void ReCreateMenuBar();
};


/*******************************************************/
/* Basic draw frame for eeschema, pcbnew and gerbview. */
/*******************************************************/

class WinEDA_DrawFrame : public WinEDA_BasicFrame
{
public:
    WinEDA_DrawPanel* DrawPanel;            // Draw area
    WinEDA_MsgPanel*  MsgPanel;             // Panel used to display some
                                            //  info (bottom of the screen)
    WinEDA_Toolbar*   m_VToolBar;           // Vertical (right side) Toolbar
    WinEDA_Toolbar*   m_AuxVToolBar;        // Auxiliary Vertical (right side)
                                            // Toolbar
    WinEDA_Toolbar*   m_OptionsToolBar;     // Options Toolbar (left side)
    WinEDA_Toolbar*   m_AuxiliaryToolBar;   // Auxiliay Toolbar used in pcbnew

    WinEDAChoiceBox*  m_SelGridBox;         // Choice box to choose the grid
                                            // size
    WinEDAChoiceBox*  m_SelZoomBox;         // Choice box to choose the zoom
                                            // value

    int          m_CursorShape;             // shape for cursor (0 = default
                                            // cursor)
    int          m_ID_current_state;        // Id of active button on the
                                            // vertical toolbar
    int          m_ID_last_state;           // Id of previous active button
                                            // on the vertical toolbar
    int          m_HTOOL_current_state;     // Id of active button on
                                            // horizontal toolbar

    int          m_InternalUnits;           // Internal units count in 1 inch
                                            // = 1000 for eeschema, = 10000
                                            // for PCBnew and Gerbview

    bool         m_Draw_Axis;               // TRUE to show X and Y axis
    bool         m_Draw_Sheet_Ref;          // TRUE to show frame references

    bool         m_Print_Sheet_Ref;         // TRUE to print frame references
    bool         m_Draw_Auxiliary_Axis;     /* TRUE to show auxiliary axis.
                                             * Used in pcbnew: the auxiliary
                                             * axis is the origin of
                                             * coordinates for drill, gerber
                                             * and component position files
                                             */
    wxPoint      m_Auxiliary_Axis_Position; /* position of the auxiliary axis */

protected:
    Ki_HotkeyInfoSectionDescriptor * m_HotkeysZoomAndGridList;
    int          m_LastGridSizeId;
    bool         m_DrawGrid;                // hide/Show grid
    int          m_GridColor;               // Grid color

private:
    BASE_SCREEN* m_CurrentScreen;           ///< current used SCREEN

protected:
    void            SetBaseScreen( BASE_SCREEN* aScreen )
    {
        m_CurrentScreen = aScreen;
    }

public:
    WinEDA_DrawFrame( wxWindow* father, int idtype,
                      const wxString& title,
                      const wxPoint& pos, const wxSize& size,
                      long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_DrawFrame();

    virtual wxString GetScreenDesc();

    /**
     * Function GetBaseScreen
     * is virtual and returns a pointer to a BASE_SCREEN or one of its
     * derivatives.  It may be overloaded by derived classes.
     */
    virtual BASE_SCREEN* GetBaseScreen() const { return m_CurrentScreen; }

    void             OnMenuOpen( wxMenuEvent& event );
    void             OnMouseEvent( wxMouseEvent& event );
    virtual void     OnHotKey( wxDC* DC, int hotkey,
                               EDA_BaseStruct* DrawStruct );

    /** Function AddMenuZoomAndGrid (virtual)
     * Add standard zoom commands and submenu zoom and grid selection to a popup menu
     * uses zoom hotkeys info base to add hotkeys info to menu commands
     * @param aMasterMenu = the menu to populate.
     */
    virtual void     AddMenuZoomAndGrid( wxMenu* aMasterMenu );

    void             Affiche_Message( const wxString& message );
    void             EraseMsgBox();
    void             Process_PageSettings( wxCommandEvent& event );
    virtual void     SetToolbars();
    /** function SetLanguage
     * called on a language menu selection
     * when using a derived function, do not forget to call this one
     */
    virtual void     SetLanguage( wxCommandEvent& event );

    virtual void     ReCreateHToolbar() = 0;
    virtual void     ReCreateVToolbar() = 0;
    virtual void     ReCreateMenuBar();
    virtual void     ReCreateAuxiliaryToolbar();
    virtual void     SetToolID( int id, int new_cursor_id,
                                const wxString& title );

    /* Thes 4 functions provide a basic way to sho/hide grid
     * and /get/set grid color.
     * thes parameters are saved in kicad config for each main frame
     */
    /** Function IsGridVisible() , virtual
     * @return true if the grid must be shown
     */
    virtual bool     IsGridVisible()
    {
        return m_DrawGrid;
    }

    /** Function SetGridVisibility() , virtual
     * It may be overloaded by derived classes
     * @param aVisible = true if the grid must be shown
     */
    virtual void     SetGridVisibility(bool aVisible)
    {
        m_DrawGrid = aVisible;
    }

    /** Function GetGridColor() , virtual
     * @return the color of the grid
     */
    virtual int     GetGridColor()
    {
        return m_GridColor;
    }

    /** Function SetGridColor() , virtual
     * @param aColor = the new color of the grid
     */
    virtual void     SetGridColor(int aColor)
    {
        m_GridColor = aColor;
    }


    /**
     * Command event handler for selecting grid sizes.
     *
     * All commands that set the grid size should eventually end up here.
     * This is where the application setting is saved.  If you override
     * this method, make sure you call down to the base class.
     *
     * @param event - Command event passed by selecting grid size from the
     *                grid size combobox on the toolbar.
     */
    virtual void     OnSelectGrid( wxCommandEvent& event );
    virtual void     OnSelectZoom( wxCommandEvent& event );

    virtual void     GeneralControle( wxDC* DC, wxPoint Mouse ) { /* dummy */ }
    virtual void     OnSize( wxSizeEvent& event );
    void             OnEraseBackground( wxEraseEvent& SizeEvent );

    void             SetToolbarBgColor( int color_num );
    virtual void     OnZoom( wxCommandEvent& event );
    void             OnGrid( int grid_type );
    void             Recadre_Trace( bool ToMouse );
    void             PutOnGrid( wxPoint* coord ); /* set the coordinate to
                                                   * the nearest grid
                                                   * coordinate */
    void             Zoom_Automatique( bool move_mouse_cursor );

    /* Set the zoom level to show the area Rect */
    void             Window_Zoom( EDA_Rect& Rect );

    /* Return the zoom level which displays the full page on screen */
    virtual int      BestZoom() = 0;

    /* Return the current zoom level */
    int              GetZoom( void );

    void             SVG_Print( wxCommandEvent& event );

    void             TraceWorkSheet( wxDC* DC, BASE_SCREEN* screen, int line_width );
    void             PlotWorkSheet( PLOTTER *plotter, BASE_SCREEN* screen );

    /** Function GetXYSheetReferences
     * Return the X,Y sheet references where the point position is located
     * @param aScreen = screen to use
     * @param aPosition = position to identify by YX ref
     * @return a wxString containing the message locator like A3 or B6
     *         (or ?? if out of page limits)
     */
    wxString         GetXYSheetReferences( BASE_SCREEN* aScreen,
                                           const wxPoint& aPosition );

    void             DisplayToolMsg( const wxString& msg );
    void             Process_Zoom( wxCommandEvent& event );
    void             Process_Grid( wxCommandEvent& event );
    virtual void     RedrawActiveWindow( wxDC* DC, bool EraseBg ) = 0;
    virtual void     OnLeftClick( wxDC* DC, const wxPoint& MousePos ) = 0;
    virtual void     OnLeftDClick( wxDC* DC, const wxPoint& MousePos );
    virtual bool     OnRightClick( const wxPoint& MousePos,
                                   wxMenu* PopMenu ) = 0;
    virtual void     ToolOnRightClick( wxCommandEvent& event );
    void             AdjustScrollBars();

    /** OnActivate event function (virtual)
     * called when activating the frame.
     * in derived classes with a virtual OnActivate function,
     * do not forget to call the WinEDA_DrawFrame::OnActivate( event )  basic function
     */
    virtual void     OnActivate( wxActivateEvent& event );
    /**
     * Function UpdateStatusBar
     * updates the status bar information.
     *
     * The base method updates the absolute and relative coordinates and the
     * zoom information.  If you override this virtual method, make sure to call
     * this subclassed method.  The status bar can draw itself.  This is not
     * a drawing function per se, but rather updates lines of text held by
     * the components within the status bar which is owned by the wxFrame.
     * <p>
     * On a MAC, be careful about calling this function when there is an
     * existing wxDC in existence on a sibling window.
     */
    virtual void     UpdateStatusBar();

    void             DisplayUnitsMsg();

    /* Handlers for block commands */
    virtual int      ReturnBlockCommand( int key );
    virtual void     InitBlockPasteInfos();
    virtual bool     HandleBlockBegin( wxDC* DC, int cmd_type,
                                       const wxPoint& startpos );
    virtual void     HandleBlockPlace( wxDC* DC );
    virtual int      HandleBlockEnd( wxDC* DC );

    void             CopyToClipboard( wxCommandEvent& event );

    /* interprocess communication */
    void             OnSockRequest( wxSocketEvent& evt );
    void             OnSockRequestServer( wxSocketEvent& evt );

    virtual void     LoadSettings();
    virtual void     SaveSettings();

    /**
     * Append a message to the message panel.
     *
     * This helper method checks to make sure the message panel exists in
     * the frame and appends a message to it using the message panel
     * AppendMessage() method.
     *
     * @param textUpper - The message upper text.
     * @param textLower - The message lower text.
     * @param color - A color ID from the Kicad color list (see colors.h).
     * @param pad - Number of spaces to pad between messages (default = 4).
     */
    void AppendMsgPanel( const wxString& textUpper, const wxString& textLower,
                         int color, int pad = 6 );

    /**
     * Clear all messages from the message panel.
     */
    void ClearMsgPanel( void );

    /** Virtual function PrintPage
     * used to print a page
     * Print the page pointed by ActiveScreen, set by the calling print function
     * @param aDC = wxDC given by the calling print function
     * @param aPrint_Sheet_Ref = true to print page references
     * @param aPrintMask = not used here
     * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
     * @param aData = a pointer on an auxiliary data (not always used, NULL if not used)
     */
    virtual void PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref,
                    int aPrintMask, bool aPrintMirrorMode,
                    void * aData = NULL);

    DECLARE_EVENT_TABLE();
};


/*********************************************************
*   class WinEDA_MsgPanel : this is a panel to display various infos
*   and messages on items in eeschema an pcbnew
*********************************************************/

/**
 * Struct MsgItem
 * is used privately by WinEDA_MsgPanel as the item type of its vector.
 * These items are the pairs of text strings shown in the MsgPanel.
 */
struct MsgItem
{
    int      m_X;
    int      m_UpperY;
    int      m_LowerY;
    wxString m_UpperText;
    wxString m_LowerText;
    int      m_Color;

    /**
     * Function operator=
     * overload the assignment operator so that the wxStrings get copied
     * properly when copying a MsgItem.
     * No, actually I'm not sure this needed, C++ compiler may auto-generate it.
     */
    MsgItem& operator=( const MsgItem& rv )
    {
        m_X = rv.m_X;
        m_UpperY    = rv.m_UpperY;
        m_LowerY    = rv.m_LowerY;
        m_UpperText = rv.m_UpperText;   // overloaded operator=()
        m_LowerText = rv.m_LowerText;   // overloaded operator=()
        m_Color     = rv.m_Color;

        return * this;
    }
};


class WinEDA_MsgPanel : public wxPanel
{
protected:
    std::vector<MsgItem>    m_Items;
    int                     m_last_x;      ///< the last used x coordinate
    wxSize                  m_fontSize;


    void showItem( wxDC& dc, const MsgItem& aItem );

    void erase( wxDC* DC );

    /**
     * Function getFontSize
     * computes the height and width of a 'W' in the system font.
     */
    static wxSize computeFontSize();

    /**
     * Calculate the width and height of a text string using the system UI font.
     */
    wxSize computeTextSize( const wxString& text );

public:
    WinEDA_DrawFrame* m_Parent;
    int m_BgColor;

public:
    WinEDA_MsgPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos,
                     const wxSize& size );
    ~WinEDA_MsgPanel();


    /**
     * Function GetRequiredHeight
     * returns the required height (in pixels) of a WinEDA_MsgPanel.  This takes
     * into consideration the system gui font, wxSYS_DEFAULT_GUI_FONT.
     */
    static int GetRequiredHeight();

    void OnPaint( wxPaintEvent& event );
    void EraseMsgBox();
    void Affiche_1_Parametre( int pos_X, const wxString& texte_H,
                              const wxString& texte_L, int color );

    /**
     * Append a message to the message panel.
     *
     * This method automatically adjusts for the width of the text string.
     * Making consectutive calls to AppendMessage will append each message
     * to the right of the last message.  This message is not compatible
     * with Affiche_1_Parametre.
     *
     * @param textUpper - The message upper text.
     * @param textLower - The message lower text.
     * @param color - A color ID from the Kicad color list (see colors.h).
     * @param pad - Number of spaces to pad between messages (default = 4).
     */
    void AppendMessage( const wxString& textUpper, const wxString& textLower,
                        int color, int pad = 6 );

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
    WinEDA_EnterText( wxWindow* parent, const wxString& Title,
                      const wxString& TextToEdit, wxBoxSizer* BoxSizer,
                      const wxSize& Size, bool Multiline = false );

    ~WinEDA_EnterText()
    {
    }


    wxString GetValue();
    void     GetValue( char* buffer, int lenmax );
    void     SetValue( const wxString& new_text );
    void     Enable( bool enbl );

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
    UserUnitType  m_UserUnit;
    int           m_Internal_Unit;

    wxTextCtrl*   m_FrameText;
    wxTextCtrl*   m_FrameSize;
private:
    wxStaticText* m_Title;

public:
    WinEDA_GraphicTextCtrl( wxWindow* parent, const wxString& Title,
                            const wxString& TextToEdit, int textsize,
                            UserUnitType user_unit, wxBoxSizer* BoxSizer, int framelen = 200,
                            int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_GraphicTextCtrl();

    wxString        GetText();
    int             GetTextSize();
    void            Enable( bool state );
    void            SetTitle( const wxString& title );

    void SetFocus() { m_FrameText->SetFocus(); }
    void            SetValue( const wxString& value );
    void            SetValue( int value );

    /**
     * Function FormatSize
     * formats a string containing the size in the desired units.
     */
    static wxString FormatSize( int internalUnit, UserUnitType user_unit, int textSize );

    static int      ParseSize( const wxString& sizeText, int internalUnit,
                               UserUnitType user_unit );
};


/**************************************************************************/
/* Class to edit/enter a coordinate (pair of values) ( INCHES or MM ) in  */
/* dialog boxes,                                                          */
/**************************************************************************/
class WinEDA_PositionCtrl
{
public:
    UserUnitType  m_UserUnit;
    int           m_Internal_Unit;
    wxPoint       m_Pos_To_Edit;

    wxTextCtrl*   m_FramePosX;
    wxTextCtrl*   m_FramePosY;
private:
    wxStaticText* m_TextX, * m_TextY;

public:
    WinEDA_PositionCtrl( wxWindow* parent, const wxString& title,
                         const wxPoint& pos_to_edit,
                         UserUnitType user_unit, wxBoxSizer* BoxSizer,
                         int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_PositionCtrl();

    void    Enable( bool x_win_on, bool y_win_on );
    void    SetValue( int x_value, int y_value );
    wxPoint GetValue();
};


/*************************************************************
 *  Class to edit/enter a size (pair of values for X and Y size)
 *  ( INCHES or MM ) in dialog boxes
 ***************************************************************/
class WinEDA_SizeCtrl : public WinEDA_PositionCtrl
{
public:
    WinEDA_SizeCtrl( wxWindow* parent, const wxString& title,
                     const wxSize& size_to_edit,
                     UserUnitType user_unit, wxBoxSizer* BoxSizer,
                     int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_SizeCtrl() { }
    wxSize GetValue();
};


/****************************************************************/
/* Class to edit/enter a value ( INCHES or MM ) in dialog boxes */
/****************************************************************/
class WinEDA_ValueCtrl
{
public:
    UserUnitType  m_UserUnit;
    int           m_Value;
    wxTextCtrl*   m_ValueCtrl;
private:
    int           m_Internal_Unit;
    wxStaticText* m_Text;

public:
    WinEDA_ValueCtrl( wxWindow* parent, const wxString& title, int value,
                      UserUnitType user_unit, wxBoxSizer* BoxSizer,
                      int internal_unit = EESCHEMA_INTERNAL_UNIT );

    ~WinEDA_ValueCtrl();

    int  GetValue();
    void SetValue( int new_value );
    void Enable( bool enbl );

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
    WinEDA_DFloatValueCtrl( wxWindow* parent, const wxString& title,
                            double value, wxBoxSizer* BoxSizer );

    ~WinEDA_DFloatValueCtrl();

    double GetValue();
    void   SetValue( double new_value );
    void   Enable( bool enbl );

    void SetToolTip( const wxString& text )
    {
        m_ValueCtrl->SetToolTip( text );
    }
};


/*************************/
/* class WinEDA_Toolbar */
/*************************/

class WinEDA_Toolbar : public wxAuiToolBar
{
public:
    wxWindow*       m_Parent;
    id_toolbar      m_Ident;
    bool            m_Horizontal;       // some auxilary TB are horizontal, others vertical

public:
    WinEDA_Toolbar( id_toolbar type, wxWindow* parent,
                    wxWindowID id, bool horizontal );

    bool GetToolState( int toolId ) { return GetToolToggled(toolId); };

    void AddRadioTool( int             toolid,
                       const wxString& label,
                       const wxBitmap& bitmap,
                       const wxBitmap& bmpDisabled = wxNullBitmap,
                       const wxString& shortHelp = wxEmptyString,
                       const wxString& longHelp = wxEmptyString,
                       wxObject*       data = NULL )
    {
       AddTool( toolid, label, bitmap, bmpDisabled, wxITEM_CHECK,
                shortHelp, longHelp, data );
    };

    void SetToolNormalBitmap( int id, const wxBitmap& bitmap ) {};
    void SetRows( int nRows ) {};

    /** Function GetDimension
     * @return the dimension of this toolbar (Height if horizontal, Width if vertical.
     */
    int GetDimension( );
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
    void (*m_MoveFct)( wxString& Text );

public:
    WinEDAListBox( WinEDA_DrawFrame* parent, const wxString& title,
                   const wxChar** ItemList,
                   const wxString& RefText,
                   void(* movefct)(wxString& Text) = NULL,
                   const wxColour& colour = wxNullColour,
                   wxPoint dialog_position = wxDefaultPosition );
    ~WinEDAListBox();

    void     SortList();
    void     Append( const wxString& item );
    void     InsertItems( const wxArrayString& itemlist, int position = 0 );
    void     MoveMouseToOrigin();
    wxString GetTextSelection();

private:
    void     OnClose( wxCloseEvent& event );
    void     OnCancelClick( wxCommandEvent& event );
    void     OnOkClick( wxCommandEvent& event );
    void     ClickOnList( wxCommandEvent& event );
    void     D_ClickOnList( wxCommandEvent& event );
    void     OnKeyEvent( wxKeyEvent& event );

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
