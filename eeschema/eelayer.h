/*************/
/* eelayer.h */
/*************/

#ifndef _EELAYER_H_
#define _EELAYER_H_

#include "wx/statline.h"

class wxBoxSizer;
class wxStaticLine;
class wxStdDialogButtonSizer;


// Specify how many elements are contained within laytool_list[]
const int NB_BUTT = 24; // Includes an element associated with the grid

// Specify how many elements are contained within laytool_index[]
const int BUTTON_GROUPS = 5;

// Specify the numbers associated with assorted controls
enum col_sel_id {
    ID_DIALOG = 1800,
    ID_CHECKBOX_SHOW_GRID,
    ID_RADIOBOX_BACKGROUND_COLOR,
    ID_COLOR_SETUP
};

// #define SYMBOL_WINEDA_SETCOLORSFRAME_STYLE wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER
#define SYMBOL_WINEDA_SETCOLORSFRAME_STYLE wxDEFAULT_DIALOG_STYLE|MAYBE_RESIZE_BORDER
#define SYMBOL_WINEDA_SETCOLORSFRAME_TITLE _("EESchema Colors")
#define SYMBOL_WINEDA_SETCOLORSFRAME_IDNAME ID_DIALOG
// #define SYMBOL_WINEDA_SETCOLORSFRAME_SIZE wxSize(400, 300)
// #define SYMBOL_WINEDA_SETCOLORSFRAME_POSITION wxDefaultPosition

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

// Specify the width and height of every (color-displaying / bitmap) button
const int BUTT_SIZE_X = 30;
const int BUTT_SIZE_Y = 20;

// Macro utile :
#define ADR( numlayer ) & (g_LayerDescr.LayerColor[numlayer])


/********************/
/* Layer menu list. */
/********************/

struct ColorButton
{
    wxString        m_Name;
    int*            m_Color;
    int             m_Id;
    wxBitmapButton* m_Button;
};

struct ButtonIndex
{
    wxString m_Name;            // Title
    int      m_Index;           // Index to last bitmap button in group
};


static ColorButton Layer_Wire_Item =
{
    _( "Wire" ),                    // Title
    ADR( LAYER_WIRE )               // Adr of optional parameter
};

static ColorButton Layer_Bus_Item =
{
    _( "Bus" ),                     // Title
    ADR( LAYER_BUS )                // Adr of optional parameter
};

static ColorButton Layer_Junction_Item =
{
    _( "Junction" ),                // Title
    ADR( LAYER_JUNCTION )           // Adr of optional parameter
};

static ColorButton Layer_LocalLabel_Item =
{
    _( "Label" ),                   // Title
    ADR( LAYER_LOCLABEL )           // Adr of optional parameter
};

static ColorButton Layer_GlobLabel_Item =
{
    _( "GlobLabel" ),               // Title
    ADR( LAYER_GLOBLABEL )          // Adr of optional parameter
};

static ColorButton Layer_NetNam_Item =
{
    _( "Netname" ),                 // Title
    ADR( LAYER_NETNAM )             // Adr of optional parameter
};

static ColorButton Layer_Notes_Item =
{
    _( "Notes" ),                   // Title
    ADR( LAYER_NOTES )              // Adr of optional parameter
};

static ColorButton Layer_NoConnect_Item =
{
    _( "NoConn" ),                  // Title
    ADR( LAYER_NOCONNECT )          // Adr of optional parameter
};


static ColorButton Layer_BodyDevice_Item =
{
    _( "Body" ),                    // Title
    ADR( LAYER_DEVICE )             // Adr of optional parameter
};

static ColorButton Layer_BodyBackgroundDevice_Item =
{
    _( "Body Bg" ),                 // Title
    ADR( LAYER_DEVICE_BACKGROUND )  // Adr of optional parameter
};

static ColorButton Layer_Pin_Item =
{
    _( "Pin" ),                     // Title
    ADR( LAYER_PIN )                // Adr of optional parameter
};

static ColorButton Layer_PinNum_Item =
{
    _( "PinNum" ),                  // Title
    ADR( LAYER_PINNUM )             // Adr of optional parameter
};

static ColorButton Layer_PinNam_Item =
{
    _( "PinNam" ),                  // Title
    ADR( LAYER_PINNAM )             // Adr of optional parameter
};

static ColorButton Layer_Reference_Item =
{
    _( "Reference" ),               // Title
    ADR( LAYER_REFERENCEPART )      // Adr of optional parameter
};

static ColorButton Layer_Value_Item =
{
    _( "Value" ),                   // Title
    ADR( LAYER_VALUEPART )          // Adr of optional parameter
};

static ColorButton Layer_Fields_Item =
{
    _( "Fields" ),                  // Title
    ADR( LAYER_FIELDS )             // Adr of optional parameter
};


static ColorButton Layer_Sheet_Item =
{
    _( "Sheet" ),                   // Title
    ADR( LAYER_SHEET )              // Adr of optional parameter
};

static ColorButton Layer_SheetFileName_Item =
{
    _( "Sheetfile" ),               // Title
    ADR( LAYER_SHEETFILENAME )      // Adr of optional parameter
};

static ColorButton Layer_SheetName_Item =
{
    _( "SheetName" ),               // Title
    ADR( LAYER_SHEETNAME )          // Adr of optional parameter
};

static ColorButton Layer_SheetLabel_Item =
{
    _( "SheetLabel (Pin Sheet)" ),  // Title
    ADR( LAYER_SHEETLABEL )         // Adr of optional parameter
};

static ColorButton Layer_HierarchicalLabel_Item =
{
    _( "Hierarchical Label" ),       // Title
    ADR( LAYER_HIERLABEL )           // Adr of optional parameter
};

static ColorButton Layer_Erc_Warning_Item =
{
    _( "Erc Warning" ),             // Title
    ADR( LAYER_ERC_WARN )           // Adr of optional parameter
};

static ColorButton Layer_Erc_Error_Item =
{
    _( "Erc Error" ),               // Title
    ADR( LAYER_ERC_ERR )            // Adr of optional parameter
};


static ColorButton Layer_Grid_Item =
{
    _( "Grid" ),                    // Title
    &g_GridColor                    // Adr of optional parameter
};


static ColorButton* laytool_list[NB_BUTT] = {
    &Layer_Wire_Item,
    &Layer_Bus_Item,
    &Layer_Junction_Item,
    &Layer_LocalLabel_Item,
    &Layer_GlobLabel_Item,
    &Layer_NetNam_Item,
    &Layer_Notes_Item,
    &Layer_NoConnect_Item,

    &Layer_BodyDevice_Item,
    &Layer_BodyBackgroundDevice_Item,
    &Layer_Pin_Item,
    &Layer_PinNum_Item,
    &Layer_PinNam_Item,
    &Layer_Reference_Item,
    &Layer_Value_Item,
    &Layer_Fields_Item,

    &Layer_Sheet_Item,
    &Layer_SheetFileName_Item,
    &Layer_SheetName_Item,
    &Layer_SheetLabel_Item,
    &Layer_HierarchicalLabel_Item,

    &Layer_Erc_Warning_Item,
    &Layer_Erc_Error_Item,

    &Layer_Grid_Item
};


static ButtonIndex Msg_General =
{
    _( "General" ),                 // Title
    7                               // Index to first bitmap button in group
};

static ButtonIndex MsgDevice_Item =
{
    _( "Device" ),                  // Title
    15                              // Index to first bitmap button in group
};

static ButtonIndex Msg_Sheets =
{
    _( "Sheets" ),                  // Title
    20                              // Index to first bitmap button in group
};

static ButtonIndex Msg_ErcMarck =
{
    _( "Erc Mark" ),                // Title
    22                              // Index to first bitmap button in group
};

static ButtonIndex Msg_Other =
{
    _( "Other" ),                   // Title
    23                              // Index to first bitmap button in group
};


static ButtonIndex* laytool_index[BUTTON_GROUPS] = {
    &Msg_General,
    &MsgDevice_Item,
    &Msg_Sheets,
    &Msg_ErcMarck,
    &Msg_Other
};


/***********************************************/
/* Derived class for the frame color settings. */
/***********************************************/

class WinEDA_SetColorsFrame: public wxDialog
{
private:
    DECLARE_DYNAMIC_CLASS( WinEDA_SetColorsFrame )
    DECLARE_EVENT_TABLE()

    WinEDA_DrawFrame*       m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxBoxSizer*             ColumnBoxSizer;
    wxBoxSizer*             RowBoxSizer;
    wxStaticText*           Label;
    wxBitmapButton*         BitmapButton;
    wxCheckBox*             m_ShowGrid;
    wxRadioBox*             m_SelBgColor;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;
    wxButton*               Button;

    // Creation
    bool Create( wxWindow* parent,
                 wxWindowID id = SYMBOL_WINEDA_SETCOLORSFRAME_IDNAME,
                 const wxString& caption = SYMBOL_WINEDA_SETCOLORSFRAME_TITLE,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = SYMBOL_WINEDA_SETCOLORSFRAME_STYLE );

    // Initializes member variables
    void Init();

    // Creates the controls and sizers
    void CreateControls();

    wxBitmap GetBitmapResource( const wxString& name );
    wxIcon GetIconResource( const wxString& name );
    static bool ShowToolTips();

    void    UpdateLayerSettings();
    void    SetColor( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );
    void    OnApplyClick( wxCommandEvent& event );

public:
    // Constructors and destructor
    WinEDA_SetColorsFrame();
    WinEDA_SetColorsFrame( WinEDA_DrawFrame* parent, const wxPoint& framepos );
    ~WinEDA_SetColorsFrame();
};

#endif    // _EELAYER_H_
