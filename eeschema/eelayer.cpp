/***************/
/* eelayer.cpp */
/***************/

/* Set up color Layers for EESchema
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "eelayer.h"
#endif

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "id.h"
#include "class_drawpanel.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

#include "eelayer.h" // Header file associated with this file

// Local variables:
int CurrentColor[NB_BUTT]; // Holds color for each layer while dialog box open


IMPLEMENT_DYNAMIC_CLASS( WinEDA_SetColorsFrame, wxDialog )

// Table of events for WinEDA_SetColorsFrame
BEGIN_EVENT_TABLE( WinEDA_SetColorsFrame, wxDialog )
    EVT_COMMAND_RANGE( ID_COLOR_SETUP, ID_COLOR_SETUP + NB_BUTT - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( wxID_OK, WinEDA_SetColorsFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SetColorsFrame::OnCancelClick )
    EVT_BUTTON( wxID_APPLY, WinEDA_SetColorsFrame::OnApplyClick )
END_EVENT_TABLE()


/**************************************************************/
void DisplayColorSetupFrame( WinEDA_DrawFrame* parent,
                             const wxPoint&    framepos )
/**************************************************************/
{
    WinEDA_SetColorsFrame* frame =
        new WinEDA_SetColorsFrame( parent, framepos );

    frame->ShowModal();
    frame->Destroy();
}


// Default Constructor (whose provision is mandated by the inclusion
// of DECLARE_DYNAMIC_CLASS( WinEDA_SetColorsFrame ) within eelayer.h)
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame()
{
    Init();
}


// Standard Constructor
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame( WinEDA_DrawFrame* parent,
                                              const wxPoint& framepos )
{
    m_Parent = parent;
    Init();
    Create( parent,
            SYMBOL_WINEDA_SETCOLORSFRAME_IDNAME,
            SYMBOL_WINEDA_SETCOLORSFRAME_TITLE,
            framepos,
            wxDefaultSize,
            SYMBOL_WINEDA_SETCOLORSFRAME_STYLE );
}


// Destructor
WinEDA_SetColorsFrame::~WinEDA_SetColorsFrame() { }


/**********************************************************/
bool WinEDA_SetColorsFrame::Create( wxWindow* parent, wxWindowID id,
                              const wxString& caption, const wxPoint& pos,
                              const wxSize& size, long style )
/**********************************************************/
{
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    return true;
}


/**********************************************************/
void WinEDA_SetColorsFrame::Init()
/**********************************************************/
{
    OuterBoxSizer        = NULL;
    MainBoxSizer         = NULL;
    ColumnBoxSizer       = NULL;
    RowBoxSizer          = NULL;
    Label                = NULL;
    BitmapButton         = NULL;
    m_ShowGrid           = NULL;
    m_SelBgColor         = NULL;
    Line                 = NULL;
    StdDialogButtonSizer = NULL;
    Button               = NULL;
}


/**********************************************************/
void WinEDA_SetColorsFrame::CreateControls()
/**********************************************************/
{
    int lyr, grp, butt_ID, buttcolor;

    SetFont( *g_DialogFont );

    OuterBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(OuterBoxSizer);

    MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(MainBoxSizer, 1, wxGROW|wxLEFT|wxRIGHT, 5);

    // Add various items to the dialog box, as determined by the
    // details of each element contained within laytool_list[]
    for( lyr = 0, grp = 0; lyr < NB_BUTT; lyr++ )
    {
        // Look for the initial button of each group of controls.
        if( lyr == 0 || lyr == laytool_index[grp]->m_Index + 1 )
        {
            if( lyr != 0 )
                grp++;

            // Add another column sizer, unless the current value of
            // grp is BUTTON_GROUPS - 1. (The very last group of controls
            // differs from the previous groups in that its associated
            // controls are located in the same column as the controls
            // associated with the preceeding group.)
            if( grp < BUTTON_GROUPS - 1 )
            {
                ColumnBoxSizer = new wxBoxSizer(wxVERTICAL);
                MainBoxSizer->Add(ColumnBoxSizer, 1, wxALIGN_TOP|wxLEFT|wxTOP, 5);
            }
            else
            {
                // Add a spacer to better separate the text string (which is
                // about to be added) from the items located above it.
                ColumnBoxSizer->AddSpacer(5);
            }

            RowBoxSizer = new wxBoxSizer(wxHORIZONTAL);
            ColumnBoxSizer->Add(RowBoxSizer, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

            // Add a text string to identify the following set of controls
            Label = new wxStaticText( this, -1, laytool_index[grp]->m_Name,
                                      wxDefaultPosition, wxDefaultSize, 0 );
            // Make this text string bold (so that it stands out better)
            Label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxNORMAL_FONT->GetFamily(),
                            wxNORMAL, wxBOLD, false, wxNORMAL_FONT->GetFaceName() ) );

            RowBoxSizer->Add(Label, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);
        }

        RowBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        ColumnBoxSizer->Add(RowBoxSizer, 0, wxGROW|wxALL, 0);

        butt_ID = ID_COLOR_SETUP + lyr;
        laytool_list[lyr]->m_Id = butt_ID;
        wxMemoryDC iconDC;
        wxBitmap ButtBitmap( BUTT_SIZE_X, BUTT_SIZE_Y );

        iconDC.SelectObject( ButtBitmap );
        buttcolor = *laytool_list[lyr]->m_Color;
        CurrentColor[lyr] = buttcolor;
        wxBrush Brush;
        iconDC.SelectObject( ButtBitmap );
        iconDC.SetPen( *wxBLACK_PEN );
        Brush.SetColour(
            ColorRefs[buttcolor].m_Red,
            ColorRefs[buttcolor].m_Green,
            ColorRefs[buttcolor].m_Blue
            );
        Brush.SetStyle( wxSOLID );

        iconDC.SetBrush( Brush );
        iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );

        BitmapButton = new wxBitmapButton( this, butt_ID, ButtBitmap, wxDefaultPosition, wxSize(BUTT_SIZE_X, BUTT_SIZE_Y) );
        RowBoxSizer->Add(BitmapButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

        laytool_list[lyr]->m_Button = BitmapButton;

        // Add a text string, unless the current value of lyr is NB_BUTT - 1
        if( lyr < NB_BUTT - 1 )
        {
            Label = new wxStaticText( this, wxID_STATIC, wxGetTranslation( laytool_list[lyr]->m_Name ),
                                      wxDefaultPosition, wxDefaultSize, 0 );
            RowBoxSizer->Add(Label, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5);
        }
        else
        {
            // Special case; provide a checkbox instead (rather than a text string).
            m_ShowGrid = new wxCheckBox( this, ID_CHECKBOX_SHOW_GRID, _("Grid"), wxDefaultPosition, wxDefaultSize, 0 );
            m_ShowGrid->SetValue( m_Parent->m_Draw_Grid );
            RowBoxSizer->Add(m_ShowGrid, 1, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5);
        }
    }

    // Add a spacer to improve appearance.
    ColumnBoxSizer->AddSpacer(5);

    wxArrayString m_SelBgColorStrings;
    m_SelBgColorStrings.Add(_("White"));
    m_SelBgColorStrings.Add(_("Black"));
    m_SelBgColor = new wxRadioBox( this, ID_RADIOBOX_BACKGROUND_COLOR, _("Background Color:"),
                                   wxDefaultPosition, wxDefaultSize, m_SelBgColorStrings, 1, wxRA_SPECIFY_COLS );
    m_SelBgColor->SetSelection( ( g_DrawBgColor == BLACK ) ? 1 : 0 );
    ColumnBoxSizer->Add(m_SelBgColor, 1, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 5);

    // Provide a line to separate all of the controls added so far from the
    // "OK", "Cancel", and "Apply" buttons (which will be added after that line).
    Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add(Line, 0, wxGROW|wxALL, 5);

    // Provide a StdDialogButtonSizer to accommodate the OK, Cancel, and Apply
    // buttons; using that type of sizer results in those buttons being
    // automatically located in positions appropriate for each (OS) version of KiCad.
    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add(StdDialogButtonSizer, 0, wxGROW|wxALL, 10);

    Button = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxRED );
    StdDialogButtonSizer->AddButton(Button);

    Button = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxBLUE );
    StdDialogButtonSizer->AddButton(Button);
    Button->SetFocus();

    Button = new wxButton( this, wxID_APPLY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton(Button);

    StdDialogButtonSizer->Realize();

    // (Dialog now needs to be resized, but the associated command is found elsewhere.)
}


/**********************************************************/
bool WinEDA_SetColorsFrame::ShowToolTips()
/**********************************************************/
{
    return true;
}


/**********************************************************/
wxBitmap WinEDA_SetColorsFrame::GetBitmapResource( const wxString& name )
/**********************************************************/
{
    wxUnusedVar(name);
    return wxNullBitmap;
}


/**********************************************************/
wxIcon WinEDA_SetColorsFrame::GetIconResource( const wxString& name )
/**********************************************************/
{
    wxUnusedVar(name);
    return wxNullIcon;
}


/**********************************************************/
void WinEDA_SetColorsFrame::SetColor( wxCommandEvent& event )
/**********************************************************/
{
    int id = event.GetId();
    int color;

    wxBitmapButton* Button;

    color = DisplayColorFrame( this,
            CurrentColor[id - ID_COLOR_SETUP] );

    if( color < 0 )
        return;

    if( CurrentColor[id - ID_COLOR_SETUP] == color )
        return;

    CurrentColor[id - ID_COLOR_SETUP] = color;
    wxMemoryDC      iconDC;

    Button = laytool_list[id - ID_COLOR_SETUP]->m_Button;

    wxBitmap        ButtBitmap = Button->GetBitmapLabel();
    iconDC.SelectObject( ButtBitmap );
    wxBrush         Brush;
    iconDC.SetPen( *wxBLACK_PEN );
    Brush.SetColour(
        ColorRefs[color].m_Red,
        ColorRefs[color].m_Green,
        ColorRefs[color].m_Blue
        );
    Brush.SetStyle( wxSOLID );

    iconDC.SetBrush( Brush );
    iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );
    Button->SetBitmapLabel( ButtBitmap );
    Button->Refresh();

    Refresh( FALSE );
}


/******************************************************************/
void WinEDA_SetColorsFrame::UpdateLayerSettings()
/******************************************************************/
{
    // Update colors for each layer
    for( int lyr = 0; lyr < NB_BUTT; lyr++ )
    {
        // (As a bitmap button has been provided for *every* layer,
        // it is not necessary to check whether it actually has been
        // provided for each of those layers.)
        *laytool_list[lyr]->m_Color = CurrentColor[lyr];
    }

    // Update whether grid is actually displayed or otherwise
//  m_Parent->m_Draw_Grid = g_ShowGrid = m_ShowGrid->GetValue();
    // The previous command compiles OK, but to prevent a warning
    // from being generated when the Linux version is being compiled,
    // the next two commands are provided instead.
    g_ShowGrid = m_ShowGrid->GetValue();
    m_Parent->m_Draw_Grid = g_ShowGrid;

    // Update color of background
    if( m_SelBgColor->GetSelection() == 0 )
        g_DrawBgColor = WHITE;
    else
        g_DrawBgColor = BLACK;
    m_Parent->SetDrawBgColor( g_DrawBgColor );
}


/**********************************************************************/
void WinEDA_SetColorsFrame::OnOkClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    UpdateLayerSettings();
    m_Parent->DrawPanel->Refresh();
    EndModal( 1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************/
{
    EndModal( -1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnApplyClick( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************/
{
    UpdateLayerSettings();
    m_Parent->DrawPanel->Refresh();
}




/*************************/
void SeedLayers()
/*************************/
{
    LayerStruct* LayerPointer = &g_LayerDescr;
    int          pt;

    LayerPointer->CommonColor = WHITE;
    LayerPointer->Flags = 0;
    pt = 0;
    LayerPointer->CurrentWidth = 1;

    /* seed Up the Layer colours, set all user layers off */
    for( pt = 0; pt < MAX_LAYERS; pt++ )
    {
        LayerPointer->LayerStatus[pt] = 0;
        LayerPointer->LayerColor[pt]  = DARKGRAY;
    }

    LayerPointer->NumberOfLayers = pt - 1;
    /* Couleurs specifiques: Mise a jour par la lecture de la config */
}


/***************************************/
EDA_Colors ReturnLayerColor( int Layer )
/****************************************/
{
    if( g_LayerDescr.Flags == 0 )
        return (EDA_Colors) g_LayerDescr.LayerColor[Layer];
    else
        return (EDA_Colors) g_LayerDescr.CommonColor;
}
