/***************/
/* set_color.h */
/***************/

#ifndef SET_COLOR_H
#define SET_COLOR_H

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "set_color.cpp"
#endif

#include "wx/statline.h"

class wxBoxSizer;
class wxFlexGridSizer;
class wxStaticLine;
class wxStdDialogButtonSizer;


// Specify how many elements are contained within laytool_list[]
const int NB_BUTT = 34;

// Specify how many elements are contained within laytool_index[]
const int BUTTON_GROUPS = 3;

// Specify the numbers associated with assorted controls
enum col_sel_id {
    ID_DIALOG = 1800,
    ID_COLOR_RESET_SHOW_LAYER_ON,
    ID_COLOR_RESET_SHOW_LAYER_OFF,
    ID_COLOR_CHECKBOX_ONOFF,
    ID_COLOR_SETUP
};

// Control identifiers
// #define SYMBOL_WINEDA_SETCOLORSFRAME_STYLE wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER
#define SYMBOL_WINEDA_SETCOLORSFRAME_STYLE wxDEFAULT_DIALOG_STYLE|MAYBE_RESIZE_BORDER
#define SYMBOL_WINEDA_SETCOLORSFRAME_TITLE _("GerbView Layer Colors:")
#define SYMBOL_WINEDA_SETCOLORSFRAME_IDNAME ID_DIALOG
// #define SYMBOL_WINEDA_SETCOLORSFRAME_SIZE wxSize(400, 300)
// #define SYMBOL_WINEDA_SETCOLORSFRAME_POSITION wxDefaultPosition

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

// Specify the width and height of every (color-displaying / bitmap) button
const int BUTT_SIZE_X = 20;
const int BUTT_SIZE_Y = 16;

/* Macro utile : */
#define ADR(numlayer) &g_DesignSettings.m_LayerColor[(numlayer)]


/**********************************/
/* Liste des menus de Menu_Layers */
/**********************************/
struct ColorButton
{
    wxString m_Name;
    int * m_Color;              // Pointeur sur la variable couleur
    bool m_NoDisplayIsColor;    // TRUE si bit ITEM_NON_VISIBLE de la variable Color
    bool * m_NoDisplay;         // Pointeur sur la variable Display on/off si ce
                                // n'est pas la var Color
    int m_Id;
    wxBitmapButton * m_Button;  // Button to display/change color assigned to this layer
//  int m_State;                // (Commented out until when it is actually used.)
    wxCheckBox * m_CheckBox;    // Option Display ON/OFF
};

struct ButtonIndex
{
    wxString m_Name;            // Title
    int      m_Index;           // Index to last bitmap button in group
};


static ButtonIndex Msg_Layers_Cu =
{
    _( "Layers 1-16" ),     // Title
    15                      // Index to last bitmap button in group
};

static ColorButton Layer_1_Butt=
{
    _("Layer 1"),       // Title
    ADR(0),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_2_Butt=
{
    _("Layer 2"),       // Title
    ADR(1),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_3_Butt=
{
    _("Layer 3"),       // Title
    ADR(2),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_4_Butt=
{
    _("Layer 4"),       // Title
    ADR(3),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_5_Butt=
{
    _("Layer 5"),       // Title
    ADR(4),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_6_Butt=
{
    _("Layer 6"),       // Title
    ADR(5),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_7_Butt=
{
    _("Layer 7"),       // Title
    ADR(6),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_8_Butt=
{
    _("Layer 8"),       // Title
    ADR(7),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_9_Butt=
{
    _("Layer 9"),       // Title
    ADR(8),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_10_Butt=
{
    _("Layer 10"),      // Title
    ADR(9),             // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_11_Butt=
{
    _("Layer 11"),      // Title
    ADR(10),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_12_Butt=
{
    _("Layer 12"),      // Title
    ADR(11),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_13_Butt=
{
    _("Layer 13"),      // Title
    ADR(12),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_14_Butt=
{
    _("Layer 14"),      // Title
    ADR(13),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_15_Butt=
{
    _("Layer 15"),      // Title
    ADR(14),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_16_Butt=
{
    _("Layer 16"),      // Title
    ADR(15),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};


static ButtonIndex Msg_Layers_Tech =
{
    _( "Layers 17-32" ),    // Title
    31                      // Index to last bitmap button in group
};

static ColorButton Layer_17_Butt=
{
    _("Layer 17"),      // Title
    ADR(16),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_18_Butt=
{
    _("Layer 18"),      // Title
    ADR(17),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_19_Butt=
{
    _("Layer 19"),      // Title
    ADR(18),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_20_Butt=
{
    _("Layer 20"),      // Title
    ADR(19),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_21_Butt=
{
    _("Layer 21"),      // Title
    ADR(20),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_22_Butt=
{
    _("Layer 22"),      // Title
    ADR(21),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_23_Butt=
{
    _("Layer 23"),      // Title
    ADR(22),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_24_Butt=
{
    _("Layer 24"),      // Title
    ADR(23),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_25_Butt=
{
    _("Layer 25"),      // Title
    ADR(24),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_26_Butt=
{
    _("Layer 26"),      // Title
    ADR(25),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_27_Butt=
{
    _("Layer 27"),      // Title
    ADR(26),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_28_Butt=
{
    _("Layer 28"),      // Title
    ADR(27),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_29_Butt=
{
    _("Layer 29"),      // Title
    ADR(28),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_30_Butt=
{
    _("Layer 30"),      // Title
    ADR(29),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_31_Butt=
{
    _("Layer 31"),      // Title
    ADR(30),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};

static ColorButton Layer_32_Butt=
{
    _("Layer 32"),      // Title
    ADR(31),            // Address of optional parameter
    TRUE                // Toggle ITEM*NOT*SHOW bit of the color variable
};


static ButtonIndex Msg_Others_Items =
{
    _( "Others" ),          // Title
    33                      // Index to last bitmap button in group
};

static ColorButton Grid_Butt=
{
    _("Grid"),                  // Title
    &g_GridColor,               // Address of optional parameter
    FALSE,
    &s_showGrid                 // Address of boolean display control parameter to toggle
};

static ColorButton Show_DCodes_Butt=
{
    _("D codes id."),           // Title
    &g_DCodesColor,             // Address of optional parameter
    FALSE,
    &DisplayOpt.DisplayPadNum   // Address of boolean display control parameter to toggle
};


static ColorButton * laytool_list[] = {
    &Layer_1_Butt,
    &Layer_2_Butt,
    &Layer_3_Butt,
    &Layer_4_Butt,
    &Layer_5_Butt,
    &Layer_6_Butt,
    &Layer_7_Butt,
    &Layer_8_Butt,
    &Layer_9_Butt,
    &Layer_10_Butt,
    &Layer_11_Butt,
    &Layer_12_Butt,
    &Layer_13_Butt,
    &Layer_14_Butt,
    &Layer_15_Butt,
    &Layer_16_Butt,

    &Layer_17_Butt,
    &Layer_18_Butt,
    &Layer_19_Butt,
    &Layer_20_Butt,
    &Layer_21_Butt,
    &Layer_22_Butt,
    &Layer_23_Butt,
    &Layer_24_Butt,
    &Layer_25_Butt,
    &Layer_26_Butt,
    &Layer_27_Butt,
    &Layer_28_Butt,
    &Layer_29_Butt,
    &Layer_30_Butt,
    &Layer_31_Butt,
    &Layer_32_Butt,

    &Grid_Butt,
    &Show_DCodes_Butt,
};


static ButtonIndex* laytool_index[BUTTON_GROUPS] = {
    &Msg_Layers_Cu,
    &Msg_Layers_Tech,
    &Msg_Others_Items
};


/**************************************************************/
/* classe derivee pour la frame de Configuration des couleurs */
/**************************************************************/

class WinEDA_SetColorsFrame: public wxDialog
{
private:
    DECLARE_DYNAMIC_CLASS( WinEDA_SetColorsFrame )
    DECLARE_EVENT_TABLE()

    WinEDA_DrawFrame*       m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxFlexGridSizer*        FlexColumnBoxSizer;
    wxStaticText*           Label;
    wxBoxSizer*             RowBoxSizer;
    wxBitmapButton*         BitmapButton;
    wxCheckBox*             CheckBox;
    wxButton*               Button;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;

    // Creation
    bool Create( wxWindow* parent,
                 wxWindowID id = SYMBOL_WINEDA_SETCOLORSFRAME_IDNAME,
                 const wxString& caption = SYMBOL_WINEDA_SETCOLORSFRAME_TITLE,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = SYMBOL_WINEDA_SETCOLORSFRAME_STYLE );

    // Initialises member variables
    void Init();

    // Creates the controls and sizers
    void CreateControls();

    wxBitmap GetBitmapResource( const wxString& name );
    wxIcon GetIconResource( const wxString& name );
    static bool ShowToolTips();

    void    SetColor( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );
    void    OnApplyClick( wxCommandEvent& event );
    void    UpdateLayerSettings();
    void    ResetDisplayLayersCu( wxCommandEvent& event );

public:
    // Constructors and destructor
    WinEDA_SetColorsFrame();
    WinEDA_SetColorsFrame( WinEDA_DrawFrame* parent, const wxPoint& framepos );
    ~WinEDA_SetColorsFrame();
};

#endif
    // SET_COLOR_H
