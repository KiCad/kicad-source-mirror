/*****************/
/* set_color.cpp */
/*****************/

/*Set up the items and layer colors and show/no show options
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "gerbview.h"
#include "pcbnew.h"
#include "class_board_design_settings.h"

#include "protos.h"

// variable used to handle grid visibility:
bool s_showGrid;

#include "set_color.h" // Header file associated with this file

// Local variables:
int CurrentColor[NB_BUTT]; // Holds color for each layer while dialog box open


IMPLEMENT_DYNAMIC_CLASS( WinEDA_SetColorsFrame, wxDialog )

// Table of events for WinEDA_SetColorsFrame
BEGIN_EVENT_TABLE( WinEDA_SetColorsFrame, wxDialog )
    EVT_BUTTON( ID_COLOR_RESET_SHOW_LAYER_OFF, WinEDA_SetColorsFrame::ResetDisplayLayersCu )
    EVT_BUTTON( ID_COLOR_RESET_SHOW_LAYER_ON, WinEDA_SetColorsFrame::ResetDisplayLayersCu )
    EVT_COMMAND_RANGE( ID_COLOR_SETUP, ID_COLOR_SETUP + NB_BUTT - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( wxID_OK, WinEDA_SetColorsFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SetColorsFrame::OnCancelClick )
    EVT_BUTTON( wxID_APPLY, WinEDA_SetColorsFrame::OnApplyClick )
END_EVENT_TABLE()


/*****************************************************/
void DisplayColorSetupFrame( WinEDA_DrawFrame* parent,
                             const wxPoint&    framepos )
/*****************************************************/
{
    WinEDA_SetColorsFrame* frame =
        new WinEDA_SetColorsFrame( parent, framepos );

    frame->ShowModal();
    frame->Destroy();
}


// Default Constructor (whose provision is mandated by the inclusion
// of DECLARE_DYNAMIC_CLASS( WinEDA_SetColorsFrame ) within set_color.h)
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
    FlexColumnBoxSizer   = NULL;
    Label                = NULL;
    RowBoxSizer          = NULL;
    BitmapButton         = NULL;
    CheckBox             = NULL;
    Button               = NULL;
    Line                 = NULL;
    StdDialogButtonSizer = NULL;
}


/**********************************************************/
void WinEDA_SetColorsFrame::CreateControls()
/**********************************************************/
{
    int lyr, cln, butt_ID, buttcolor;
    wxString msg;
    wxSize CorrectSize; // Used while specifying sizes of buttons and spacers
    int ButtonHeight;   // Also used for the same reason

    OuterBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(OuterBoxSizer);

    MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(MainBoxSizer, 1, wxGROW|wxLEFT|wxRIGHT, 5);

    // Add various items to the dialog box, as determined by the
    // details of each element contained within laytool_list[]
    s_showGrid = m_Parent->m_Draw_Grid;
    for( lyr = 0, cln = 0; lyr < NB_BUTT; lyr++ )
    {
        // Look for the first set of controls within each column.
        if( lyr == 0 || lyr == laytool_index[cln]->m_Index + 1 )
        {
            if( lyr != 0 )
                cln++;

            // Specify a FlexGrid sizer with seventeen rows and one column.
            FlexColumnBoxSizer = new wxFlexGridSizer(17, 1, 0, 0);

            // Specify that all of the rows can be expanded.
            for( int ii = 0; ii < 17; ii++ )
            {
                FlexColumnBoxSizer->AddGrowableRow(ii);
            }

            // Specify that the column can also be expanded.
            FlexColumnBoxSizer->AddGrowableCol(0);

            MainBoxSizer->Add(FlexColumnBoxSizer, 1, wxGROW|wxLEFT, 5);

            // Add a text string to identify the controls within this column.
            Label = new wxStaticText( this, wxID_STATIC, laytool_index[cln]->m_Name,
                                      wxDefaultPosition, wxDefaultSize, 0 );

            // Make this text string bold (so that it stands out better).
            Label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxNORMAL_FONT->GetFamily(),
                                    wxNORMAL, wxBOLD, false, wxNORMAL_FONT->GetFaceName() ) );

            FlexColumnBoxSizer->Add(Label, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);
        }

        // Provide a sizer for each layer to accomodate its associated bitmap button and checkbox.
        RowBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        FlexColumnBoxSizer->Add(RowBoxSizer, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);

        butt_ID = ID_COLOR_SETUP + lyr;
        laytool_list[lyr]->m_Id = butt_ID;

        // Provide a bitmap button, and "paint" this with the appropriate color.
        wxMemoryDC iconDC;
        wxBitmap ButtBitmap( BUTT_SIZE_X, BUTT_SIZE_Y );
        iconDC.SelectObject( ButtBitmap );
        buttcolor = *laytool_list[lyr]->m_Color & MASKCOLOR;
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

        BitmapButton = new wxBitmapButton( this, butt_ID,
                                           ButtBitmap,
                                           wxDefaultPosition,
                                           wxSize(BUTT_SIZE_X, BUTT_SIZE_Y) );
        laytool_list[lyr]->m_Button = BitmapButton;

        RowBoxSizer->Add(BitmapButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

        // Provide a checkbox, and specify the appropriate caption and checked state.
        msg = wxGetTranslation( laytool_list[lyr]->m_Name.GetData() );

        CheckBox = new wxCheckBox( this, ID_COLOR_CHECKBOX_ONOFF, msg,
                                   wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );

        laytool_list[lyr]->m_CheckBox = CheckBox;

        if( laytool_list[lyr]->m_NoDisplayIsColor )
        {
            if( g_DesignSettings.IsLayerVisible( lyr ))
                CheckBox->SetValue( TRUE );
            else
                CheckBox->SetValue( FALSE );
        }
        else
            CheckBox->SetValue( *laytool_list[lyr]->m_NoDisplay );

        RowBoxSizer->Add(CheckBox, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5);
    }

    // Now provide two (standard/non-bitmap) buttons within the third column, along with a number
    // of spacers (so that if the dialog box is resized by the user, each of the controls within
    // each of the columns will be repositioned in an aesthetically-acceptable manner).
    //
    // Before adding either of those buttons, provide a spacer to properly separate them from the
    // bitmap buttons and checkboxes located above them. The height of that spacer should match
    // the height of each "RowBox" sizer that has already been provided (to accomodate a bitmap
    // button and checkbox), so that the top edge of the first button will line up with the top
    // edge of the fourth checkbox provided within each of the first and second columns. (Hence
    // that height is the larger of each bitmap button's height and each checkbox'es height.)
    CorrectSize = CheckBox->GetSize();
    if( CorrectSize.y < BUTT_SIZE_Y )
        CorrectSize.y = BUTT_SIZE_Y;

    FlexColumnBoxSizer->Add(5, CorrectSize.y, 1, wxBOTTOM, 5);

    // For aesthetic reasons, both of the buttons will be made equally wide; hence the width
    // required for each of those buttons needs to be determined before the appropriate width
    // for both of them can be specified.
    int width0;

    // Specify the relevent details for the first button, but in the first instance,
    // specify the caption which will be used by the second of these buttons.
    Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_ON, _("Show None"),
                           wxDefaultPosition, wxDefaultSize, 0 );

    // Note the width of the button required for the initially specified caption.
    // Also note the height of this button, as that detail will be required later while specifying
    // the height of yet more spacers that will subsequently be provided beneath both buttons.
    Button->GetSize( &width0, &ButtonHeight );

    // Now change the caption of this button to what is really wanted for it.
    Button->SetLabel( _("Show All") );

    // Also note the width of the button required for the updated caption.
    Button->GetSize( &CorrectSize.x, &ButtonHeight );

    // Upate the value of CorrectSize.x if required (as that value will subsequently
    // be used to specify the (minimum) width for both of these buttons).
    if( CorrectSize.x < width0 )
         CorrectSize.x = width0;

    // Complete the steps necessary for providing the first button.
    if (WinEDA_SetColorsFrame::ShowToolTips())
        Button->SetToolTip( _("Switch on all of the Gerber layers") );
    Button->SetMinSize( wxSize( CorrectSize.x, ButtonHeight ) );
    FlexColumnBoxSizer->Add(Button, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    // Now do everything required for providing the second button.
    Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_OFF, _("Show None"),
                           wxDefaultPosition, wxDefaultSize, 0 );
    if (WinEDA_SetColorsFrame::ShowToolTips())
        Button->SetToolTip( _("Switch off all of the Gerber layers") );
    Button->SetMinSize( wxSize( CorrectSize.x, ButtonHeight ) );
    FlexColumnBoxSizer->Add(Button, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);

    // As each column contains seventeen rows, and only six rows of the third column have been
    // occupied so far, spacers still need to be provided to occupy each of the remaining eleven
    // rows within that column. So determine the collective height required for those spacers,
    // so that the appropriate height for each of those spacers can subsequently be determined.
    //
    // Collective height required by the 11 spacers = 13 * CorrectSize.y - 2 * ButtonHeight
    //
    // As the height of a spacer is always an integral number, some of the spacers will probably
    // need to be one unit taller than the remaining spacers; thus the remainder (modulus) will
    // also determine what height should subsequently be assigned to each of those spacers.
    // (Reuse width0 to hold value of remainder, rather than defining another new variable.)
    width0 = (13 * CorrectSize.y - 2 * ButtonHeight) % 11;
    CorrectSize.y = (13 * CorrectSize.y - 2 * ButtonHeight) / 11;
    for( int ii = 1; ii < 12; ii++ )
    {
        if( ii <= width0 )
            FlexColumnBoxSizer->Add(5, CorrectSize.y + 1, 1, wxBOTTOM, 5);
        else
            FlexColumnBoxSizer->Add(5, CorrectSize.y, 1, wxBOTTOM, 5);
    }

    // Provide a line to separate the controls which have been provided so far from
    // the OK, Cancel, and Apply buttons (which will be provided after this line)
    Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add(Line, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    // Provide a StdDialogButtonSizer to accommodate the OK, Cancel, and Apply
    // buttons; using that type of sizer results in those buttons being
    // automatically located in positions appropriate for each (OS) version of KiCad.
    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add(StdDialogButtonSizer, 0, wxGROW|wxALL, 10);

    Button = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton(Button);

    Button = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton(Button);

    Button = new wxButton( this, wxID_APPLY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton(Button);

    StdDialogButtonSizer->Realize();

    // (Dialog now needs to be resized, but the associated command is provided elsewhere.)
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


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnOkClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    UpdateLayerSettings();
    m_Parent->DrawPanel->Refresh();
    EndModal( 1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnCancelClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    EndModal( -1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnApplyClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    UpdateLayerSettings();
    m_Parent->DrawPanel->Refresh();
}


/***********************************************************/
void WinEDA_SetColorsFrame::SetColor(wxCommandEvent& event)
/***********************************************************/
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
    for( int lyr = 0; lyr < NB_BUTT - 2; lyr++ )
    {
        g_DesignSettings.SetLayerVisibility( lyr, laytool_list[lyr]->m_CheckBox->GetValue() );
        *laytool_list[lyr]->m_Color = CurrentColor[lyr];
    }

    // (As a bitmap button and a checkbox have been provided for *every*
    // layer, it is not necessary to check whether each of those items
    // actually has been provided for each of those layers.)


    g_GridColor                 = CurrentColor[32];
    s_showGrid                  = laytool_list[32]->m_CheckBox->GetValue();

    g_ColorsSettings.SetItemColor(DCODES_VISIBLE, CurrentColor[33] );
    DisplayOpt.DisplayPadNum    = laytool_list[33]->m_CheckBox->GetValue();

    // Additional command required for updating visibility of grid.
    m_Parent->m_Draw_Grid = s_showGrid;
}


/***********************************************************************/
void WinEDA_SetColorsFrame::ResetDisplayLayersCu(wxCommandEvent& event)
/***********************************************************************/
{
    bool NewState = ( event.GetId() == ID_COLOR_RESET_SHOW_LAYER_ON )
                    ? TRUE
                    : FALSE;

    for( int lyr = 0; lyr < 32; lyr++ )
    {
        // (As a checkbox has been provided for *every* layer, it is not
        // necessary to check whether it actually has been provided for
        // each of those layers.)
        laytool_list[lyr]->m_CheckBox->SetValue( NewState );
    }
}
