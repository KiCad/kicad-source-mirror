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

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) delete (p); (p) = NULL; //C++ guarantees that operator delete checks its argument for null-ness
#endif

#define INTERNAL_UNIT_TYPE      0        // Internal unit = inch

#ifndef EESCHEMA_INTERNAL_UNIT
#define EESCHEMA_INTERNAL_UNIT  1000
#endif

//  Option for dialog boxes
// #define DIALOG_STYLE wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT|wxSTAY_ON_TOP
#define DIALOG_STYLE wxDEFAULT_DIALOG_STYLE | wxFRAME_FLOAT_ON_PARENT | MAYBE_RESIZE_BORDER

#define KICAD_DEFAULT_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE|wxWANTS_CHARS

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
class DRAWSEGMENT;
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
    wxPoint         m_FramePos;
    wxSize          m_FrameSize;
    int             m_MsgFrameHeight;

    WinEDA_MenuBar* m_MenuBar;      // menu du haut d'ecran
    WinEDA_Toolbar* m_HToolBar;     // Standard horizontal Toolbar
    bool            m_FrameIsActive;
    wxString        m_FrameName;    // name used for writting and reading setup
                                    // It is "SchematicFrame", "PcbFrame" ....
    wxString        m_AboutTitle;   // Name of program displayed in About.

public:

    // Constructor and destructor
    WinEDA_BasicFrame( wxWindow* father, int idtype,
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

private:
    BASE_SCREEN*    m_CurrentScreen;        ///< current used SCREEN

protected:
    void            SetBaseScreen( BASE_SCREEN* aScreen ) { m_CurrentScreen = aScreen; }

public:

    // Constructor and destructor
    WinEDA_DrawFrame( wxWindow* father, int idtype,
                      const wxString& title,
                      const wxPoint& pos, const wxSize& size,
                      long style = KICAD_DEFAULT_DRAWFRAME_STYLE );

    ~WinEDA_DrawFrame();

    virtual wxString	 GetScreenDesc();

    /**
     * Function GetBaseScreen
     * is virtual and returns a pointer to a BASE_SCREEN or one of its derivatives.
     * It may be overloaded by derived classes.
     */
    virtual BASE_SCREEN* GetBaseScreen() const  { return m_CurrentScreen; }

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
    virtual void    SetToolID( int id, int new_cursor_id,
                               const wxString& title );

    virtual void    OnSelectGrid( wxCommandEvent& event );
    virtual void    OnSelectZoom( wxCommandEvent& event );

    virtual void    GeneralControle( wxDC* DC, wxPoint Mouse ){ /* dummy */ }
    virtual void    OnSize( wxSizeEvent& event );
    void            OnEraseBackground( wxEraseEvent& SizeEvent );

//  void OnChar(wxKeyEvent& event);
    void            SetToolbarBgColor( int color_num );
    virtual void    OnZoom( wxCommandEvent& event );
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
    /** Function GetXYSheetReferences
     * Return the X,Y sheet references where the point position is located
     * @param aScreen = screen to use
     * @param aPosition = position to identify by YX ref
     * @return a wxString containing the message locator like A3 or B6 (or ?? if out of page limits)
     */
    wxString        GetXYSheetReferences( BASE_SCREEN* aScreen, const wxPoint& aPosition );

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
    virtual void    Affiche_Status_Box(); /* Affichage des coord curseur, zoom .. */
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

    DECLARE_EVENT_TABLE();
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
    int                     m_last_x;       ///< the last used x coordinate


    void    showItem( wxDC& dc, const MsgItem& aItem );

    void    erase( wxDC* DC );

public:
    WinEDA_DrawFrame* m_Parent;
    int m_BgColor;          // couleur de fond

public:

    // Constructor and destructor
    WinEDA_MsgPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos, const wxSize& size );
    ~WinEDA_MsgPanel();

    void    OnPaint( wxPaintEvent& event );
    void    EraseMsgBox();
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

    /**
     * Function FormatSize
     * formats a string containing the size in the desired units.
     */
    static wxString FormatSize( int internalUnit, int units, int textSize );

    static int ParseSize( const wxString& sizeText, int internalUnit, int units );
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
