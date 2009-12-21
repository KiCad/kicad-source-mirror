/*****************/
/* set_color.cpp */
/*****************/

/*Set up the items and layer colors and show/no show options
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

//#include "protos.h"

// temporary variable used to handle grid visibility:
bool s_showGrid;

#include "set_color.h" // Header file associated with this file

// Local variables:
const int COLOR_COUNT = 40;    // 40 = 29 (layers) + 11 (others)
int CurrentColor[COLOR_COUNT]; // Holds color for each layer while dialog box open


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
void DisplayColorSetupFrame( WinEDA_PcbFrame* parent,
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
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame( WinEDA_PcbFrame* parent,
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
    wxSize CorrectRowSize; // Used while specifying height of various spacers
    int ButtonHeight;      // Used while specifying height of other spacers

    OuterBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(OuterBoxSizer);

    MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(MainBoxSizer, 1, wxGROW|wxLEFT|wxRIGHT, 5);

    s_showGrid = m_Parent->m_Draw_Grid;

    // Add various items to the dialog box, as determined by the
    // details of each element contained within laytool_list[]
    for( lyr = 0, cln = 0; lyr < NB_BUTT; lyr++ )
    {
        // Look for the first set of controls within each column.
        if( lyr == 0 || lyr == laytool_index[cln]->m_Index + 1 )
        {
            if( lyr != 0 )
                cln++;

            // Specify a FlexGrid sizer with nineteen rows and one column.
            FlexColumnBoxSizer = new wxFlexGridSizer(19, 1, 0, 0);

            // Specify that all of the rows can be expanded.
            for( int ii = 0; ii < 19; ii++ )
            {
                FlexColumnBoxSizer->AddGrowableRow(ii);
            }

            // Specify that the column can also be expanded.
            FlexColumnBoxSizer->AddGrowableCol(0);

            MainBoxSizer->Add(FlexColumnBoxSizer, 1, wxGROW|wxLEFT|wxBOTTOM, 5);

            // Add a text string to identify the controls within this column.
            Label = new wxStaticText( this, wxID_STATIC, wxGetTranslation(laytool_index[cln]->m_Name),
                                      wxDefaultPosition, wxDefaultSize, 0 );

            // Make this text string bold (so that it stands out better).
            Label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxNORMAL_FONT->GetFamily(),
                                    wxNORMAL, wxBOLD, false, wxNORMAL_FONT->GetFaceName() ) );

            FlexColumnBoxSizer->Add(Label, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);
        }

        // Provide a sizer to accomodate the (bitmap button and) checkbox associated with the
        // current layer (while providing a spacer instead a button, when appropriate).
        RowBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        FlexColumnBoxSizer->Add(RowBoxSizer, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);

        butt_ID = ID_COLOR_SETUP + lyr;
        laytool_list[lyr]->m_Id = butt_ID;

        if( laytool_list[lyr]->m_Color )
        {
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
        }
        else
        {
            // Provide a spacer instead (rather than a bitmap button).
            RowBoxSizer->Add(BUTT_SIZE_X, BUTT_SIZE_Y, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5);
        }

        //---------------------------------------------------------------
        // Note: When setting texts, we must call wxGetTranslation( ) for all statically created texts
        // if we want them translated
        switch( laytool_list[lyr]->m_Type )
        {
            case type_layer:
                msg = m_Parent->GetBoard()->GetLayerName( laytool_list[lyr]->m_LayerNumber );
                break;

            case type_via:
                msg = wxGetTranslation( g_ViaType_Name[laytool_list[lyr]->m_LayerNumber] );
                break;

            default:
                msg = wxGetTranslation( laytool_list[lyr]->m_Title );
                break;
        }

        //---------------------------------------------------------------
        CheckBox = new wxCheckBox( this, ID_COLOR_CHECKBOX_ONOFF, msg,
                                   wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );

        laytool_list[lyr]->m_CheckBox = CheckBox;

        //---------------------------------------------------------------

        switch( laytool_list[lyr]->m_Type )
        {
            case type_layer:
                CheckBox->SetValue( g_DesignSettings.IsLayerVisible( laytool_list[lyr]->m_LayerNumber ));
                CheckBox->Enable(g_DesignSettings.IsLayerEnabled( laytool_list[lyr]->m_LayerNumber ) );
                break;

            case type_via:
            case type_element:
                CheckBox->SetValue( g_DesignSettings.IsElementVisible( laytool_list[lyr]->m_LayerNumber ));
                break;

            case type_visual:
                CheckBox->SetValue( *laytool_list[lyr]->m_NoDisplay );
                break;
        }

        RowBoxSizer->Add(CheckBox, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, 5);

        // Check whether the last (bitmap button and) checkbox has been provided for this column.
        if( lyr == laytool_index[cln]->m_Index )
        {
            // What happens now depends upon which column is currently being created. Another
            // two (standard/non-bitmap) buttons are provided within the first column, while
            // assorted spacers are added to each of the other two columns (so that if the
            // dialog box is resized by the user, each of the controls within each of the
            // columns will be repositioned in an aesthetically-acceptable manner).
            if( cln == 0 )
            {
                // Provide another two buttons; for aesthetic reasons, these will both be
                // made equally wide.

                int GoodWidth, width0;

                // Specify the relevent details for the first button, but in the first instance,
                // specify the caption which will be used by the second of these buttons (with
                // the objective being to determine how wide those buttons would need to be to
                // be able to accomodate the caption provided for either of them).
                Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_ON, _("Show None"),
                                       wxDefaultPosition, wxDefaultSize, 0 );

                // Note the height of this button, so that the same height can also be specified for
                // the spacers occupying the corresponding cells within each of the other two columns.
                // Also note the width of the button required for the initially specified caption.
                Button->GetSize( &width0, &ButtonHeight );

                // Now change the caption of this button to what is really wanted for it.
                Button->SetLabel( _("Show All") );

                // Also note the width of the button required for the updated caption.
                Button->GetSize( &GoodWidth, &ButtonHeight );

                // Upate the value of GoodWidth if required (as that variable will subsequently
                // be used to specify the (minimum) width for both of these buttons).
                if( GoodWidth < width0 )
                    GoodWidth = width0;

                // Complete the steps necessary for providing the first button.
                if (WinEDA_SetColorsFrame::ShowToolTips())
                    Button->SetToolTip( _("Switch on all of the copper layers") );
                Button->SetMinSize( wxSize( GoodWidth, ButtonHeight ) );
                FlexColumnBoxSizer->Add(Button, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5);

                // Now do everything required for providing the second button.
                Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_OFF, _("Show None"),
                                       wxDefaultPosition, wxDefaultSize, 0 );
                if (WinEDA_SetColorsFrame::ShowToolTips())
                    Button->SetToolTip( _("Switch off all of the copper layers") );
                Button->SetMinSize( wxSize( GoodWidth, ButtonHeight ) );
                FlexColumnBoxSizer->Add(Button, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5);
            }
            else
            {
                // Provide additional spacers within each other column to occupy any cells
                // that would otherwise remain unoccupied.
                //
                // Provide a spacer, of appropriate height, for each row that
                // would otherwise contain a (bitmap button and) checkbox.
                //
                // First determine what height is required for those spacers; i.e. the
                // larger of each bitmap button's height and each checkbox'es height.
                // (That only needs to be determined once, so do so after the last
                // bitmap button and checkbox have been provided for the second column.)
                if( cln == 1 )
                {
                    CorrectRowSize = CheckBox->GetSize();
                    if( CorrectRowSize.y < BUTT_SIZE_Y )
                        CorrectRowSize.y = BUTT_SIZE_Y;
                }

                // The first column contains 16 checkboxes, while each of the other two columns
                // contains a smaller number. Determine how many checkboxes have actually been
                // provided within each of the other columns, then provide an appropriate number
                // of sizers (to take the total count of checkboxes and substitute sizers to 16).
                for( int ii = lyr; ii < 16  + laytool_index[cln - 1]->m_Index
                                      + lyr - laytool_index[cln]->m_Index; ii++ )
                {
                    FlexColumnBoxSizer->Add(5, CorrectRowSize.y, 1, wxBOTTOM, 5);
                }

                // As the first column also contains two (standard/non-bitmap) buttons, while each of
                // the other two columns doesn't, also provide yet another two spacers, each of button
                // height, within each of the other two columns.
                FlexColumnBoxSizer->Add(5, ButtonHeight, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxTOP, 5);
                FlexColumnBoxSizer->Add(5, ButtonHeight, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxTOP, 5);
            }
        }
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


/**********************************************************************/
void WinEDA_SetColorsFrame::OnOkClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
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
    for( int lyr = 0; lyr < NB_BUTT; lyr++ )
    {
        if( laytool_list[lyr]->m_Color )
            *laytool_list[lyr]->m_Color = CurrentColor[lyr];
        switch( laytool_list[lyr]->m_Type )
        {
            case type_layer:
                g_DesignSettings.SetLayerVisibility( laytool_list[lyr]->m_LayerNumber,
                    laytool_list[lyr]->m_CheckBox->GetValue() );
                break;

            case type_via:
            case type_element:
                g_DesignSettings.SetElementVisibility( laytool_list[lyr]->m_LayerNumber,
                    laytool_list[lyr]->m_CheckBox->GetValue() );
                break;

            case type_visual:
                *laytool_list[lyr]->m_NoDisplay = laytool_list[lyr]->m_CheckBox->GetValue();
                break;
        }
    }

    // Additional command required for updating visibility of grid.
    m_Parent->m_Draw_Grid = s_showGrid;
}

/**********************************************************************/
void WinEDA_SetColorsFrame::ResetDisplayLayersCu( wxCommandEvent& event )
/**********************************************************************/
{
    bool NewState = ( event.GetId() == ID_COLOR_RESET_SHOW_LAYER_ON )
                    ? TRUE
                    : FALSE;

    for( int lyr = 0; lyr < 16; lyr++ )
    {
        if( ! laytool_list[lyr]->m_CheckBox->IsEnabled() )
            continue;
        laytool_list[lyr]->m_CheckBox->SetValue( NewState );
    }
}
