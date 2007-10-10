/* Set up the items and layer colors and show/no show options  */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/* Variables locales */
const int BUTT_SIZE_X = 25;
const int BUTT_SIZE_Y = 15;

const int COLOR_COUNT = 43;    // 43 = 29 (layers) + 11 (others) + 3 (headings)
    // Is there a better way to determine how many elements CurrentColor requires?
int CurrentColor[COLOR_COUNT]; // Holds color for each layer while dialog box open

/* Fonctions locales: */

/* Macro utile : */
#define ADR( numlayer )     &g_DesignSettings.m_LayerColor[(numlayer)]

enum col_sel_id {
    ID_COLOR_RESET_SHOW_LAYER_ON = 1800,
    ID_COLOR_RESET_SHOW_LAYER_OFF,
    ID_COLOR_CHECKBOX_ONOFF,
    ID_COLOR_SETUP
};

/**********************************/
/* Liste des menus de Menu_Layers */
/**********************************/
struct ColorButton
{
    const wxString m_Title;
    int            m_LayerNumber;
    int*           m_Color;             ///< pointer to color variable to manipulate
    bool           m_NoDisplayIsColor;  ///< TRUE if bit ITEM_NOT_SHOW of the color variable should be manipulated
    bool*          m_NoDisplay;         ///< pointer to the on/off display control variable, if it is not the color variable

    int             m_Id;
    wxBitmapButton* m_Button;
    int             m_State;
    wxCheckBox*     m_CheckBox;         ///< Display ON/OFF toggle
};

#include "set_color.h"  // Include description and list of tools and buttons


/*************************************************************/
/* classe derivee pour la frame de Configuration des couleurs*/
/*************************************************************/

class WinEDA_SetColorsFrame : public wxDialog
{
private:
    WinEDA_DrawFrame* m_Parent;

public:

    // Constructor and destructor
    WinEDA_SetColorsFrame( WinEDA_DrawFrame * parent, const wxPoint &framepos );
    ~WinEDA_SetColorsFrame() { };

private:
    void    SetColor( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );
    void    OnApplyClick( wxCommandEvent& event );
    void    UpdateLayerSettings();
    void    ResetDisplayLayersCu( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


/* Table des evenements pour WinEDA_SetColorsFrame */
BEGIN_EVENT_TABLE( WinEDA_SetColorsFrame, wxDialog )
    EVT_BUTTON( ID_COLOR_RESET_SHOW_LAYER_OFF, WinEDA_SetColorsFrame::ResetDisplayLayersCu )
    EVT_BUTTON( ID_COLOR_RESET_SHOW_LAYER_ON, WinEDA_SetColorsFrame::ResetDisplayLayersCu )
    EVT_BUTTON( wxID_OK, WinEDA_SetColorsFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SetColorsFrame::OnCancelClick )
    EVT_BUTTON( wxID_APPLY, WinEDA_SetColorsFrame::OnApplyClick )
    EVT_BUTTON( ID_COLOR_SETUP, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 1, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 2, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 3, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 4, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 5, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 6, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 7, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 8, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 9, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 10, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 11, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 12, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 13, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 14, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 15, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 16, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 17, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 18, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 19, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 20, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 21, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 22, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 23, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 24, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 25, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 26, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 27, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 28, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 29, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 30, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 31, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 32, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 33, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 34, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 35, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 36, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 37, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 38, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 39, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 40, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 41, WinEDA_SetColorsFrame::SetColor )
    EVT_BUTTON( ID_COLOR_SETUP + 42, WinEDA_SetColorsFrame::SetColor )
//  EVT_BUTTON( ID_COLOR_SETUP + 43, WinEDA_SetColorsFrame::SetColor )
//  EVT_BUTTON( ID_COLOR_SETUP + 44, WinEDA_SetColorsFrame::SetColor )
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


/**********************************************************************/
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame( 
             WinEDA_DrawFrame* parent, const wxPoint&    framepos ) :
    wxDialog( parent, -1, _( "Colors:" ), framepos, wxSize( -1, -1 ), DIALOG_STYLE )
/**********************************************************************/
{
#define START_Y 25
    wxBitmapButton* ButtonB;
    int             ii, yy, xx, butt_ID, buttcolor;
    wxPoint         pos;
    wxSize          winsize;
    wxString        msg;

    m_Parent = parent;
    SetFont( *g_DialogFont );

    pos.x = 5;
    pos.y = START_Y;

    for( ii = 0; laytool_list[ii] != NULL; ii++ )
    {
        if( !laytool_list[ii]->m_Color && !laytool_list[ii]->m_NoDisplay )
        {
            if( pos.y != START_Y )
            {
                pos.x += BUTT_SIZE_X + 120;
                pos.y = START_Y;
            }

            if( laytool_list[ii]->m_LayerNumber >= 0 )
            {
                if( laytool_list[ii]->m_Title == wxT( "*" ) )
                {
                    msg = g_ViaType_Name[laytool_list[ii]->m_LayerNumber];
                }
                else
                    msg = ReturnPcbLayerName( laytool_list[ii]->m_LayerNumber );
            }
            else
                msg = wxGetTranslation( laytool_list[ii]->m_Title.GetData() );

            new wxStaticText( this, -1, msg,
                              wxPoint( pos.x + 10, pos.y - 18 ), wxSize( -1, -1 ), 0 );

            continue;
        }

        if( laytool_list[ii]->m_Id == 0 )
            laytool_list[ii]->m_Id = ID_COLOR_SETUP + ii;

        butt_ID = laytool_list[ii]->m_Id;

        laytool_list[ii]->m_CheckBox = new wxCheckBox( this,
                                                       ID_COLOR_CHECKBOX_ONOFF, wxEmptyString,
                                                       pos );

        if( laytool_list[ii]->m_NoDisplayIsColor )
        {
            if( *laytool_list[ii]->m_Color & ITEM_NOT_SHOW )
                laytool_list[ii]->m_CheckBox->SetValue( FALSE );
            else
                laytool_list[ii]->m_CheckBox->SetValue( TRUE );
        }
        else if( laytool_list[ii]->m_NoDisplay )
            laytool_list[ii]->m_CheckBox->SetValue( *laytool_list[ii]->m_NoDisplay );

        xx = laytool_list[ii]->m_CheckBox->GetSize().x + 3;

        if( laytool_list[ii]->m_Color )
        {
            wxMemoryDC iconDC;
            wxBitmap ButtBitmap( BUTT_SIZE_X, BUTT_SIZE_Y );
            iconDC.SelectObject( ButtBitmap );
            buttcolor = *laytool_list[ii]->m_Color & MASKCOLOR;
            CurrentColor[ii] = buttcolor;
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

            ButtonB = new wxBitmapButton( this, butt_ID,
                                         ButtBitmap,
                                         wxPoint(pos.x + xx, pos.y),
                                         wxSize(BUTT_SIZE_X, BUTT_SIZE_Y) );
            laytool_list[ii]->m_Button = ButtonB;
            xx += BUTT_SIZE_X + 3;
        }

        if( laytool_list[ii]->m_LayerNumber >= 0 )
        {
            if( laytool_list[ii]->m_Title == wxT( "*" ) )
                msg = g_ViaType_Name[laytool_list[ii]->m_LayerNumber];
            else
                msg = ReturnPcbLayerName( laytool_list[ii]->m_LayerNumber );
        }
        else
            msg = wxGetTranslation( laytool_list[ii]->m_Title.GetData() );

        new wxStaticText( this, -1, msg,
                          wxPoint( pos.x + xx, pos.y + 1 ),
                          wxSize( -1, -1 ), 0 );

        yy     = BUTT_SIZE_Y + 5;
        pos.y += yy;
    }

    pos.x = 5;
    pos.y = 355;

    wxButton* Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_ON,
                                     _( "Show All" ), pos );
    Button->SetForegroundColour( wxColor( 0, 100, 0 ) );

    pos.x += Button->GetSize().x + 10;

    Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_OFF,
                           _( "Show None" ), pos );
    Button->SetForegroundColour( wxColor( 100, 0, 0 ) );

    pos.x = MAX( pos.x + 20, 480 - 3 * Button->GetSize().x );

    Button = new wxButton( this, wxID_OK, _("OK"), pos );
    Button->SetForegroundColour( *wxRED );

    pos.x += Button->GetSize().x + 10;

    Button = new wxButton( this, wxID_CANCEL, _("Cancel"), pos );
    Button->SetForegroundColour( *wxBLUE );

    pos.x += Button->GetSize().x + 10;

    Button = new wxButton( this, wxID_APPLY, _("Apply"), pos );

    winsize.x = MAX( 500, pos.x + Button->GetSize().x + 10 );
    winsize.y = pos.y + Button->GetSize().y + 5;
    SetClientSize( winsize );
}


/**********************************************************************/
void WinEDA_SetColorsFrame::OnOkClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    UpdateLayerSettings();
    m_Parent->ReDrawPanel();
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
    m_Parent->ReDrawPanel();
}


/**********************************************************/
void WinEDA_SetColorsFrame::SetColor( wxCommandEvent& event )
/**********************************************************/
{
    int ii;
    int id = event.GetId();
    int color;

    color = DisplayColorFrame( this, CurrentColor[id - ID_COLOR_SETUP] );
    if( color < 0 )
        return;

    for( ii = 0; laytool_list[ii] != NULL; ii++ )
    {
        if( laytool_list[ii]->m_Id != id )
            continue;

        if( laytool_list[ii]->m_Color == NULL )
            continue;

        if( CurrentColor[ii] == color )
            break;

        CurrentColor[ii] = color;
        wxMemoryDC      iconDC;

        wxBitmapButton* Button = laytool_list[ii]->m_Button;

        wxBitmap        ButtBitmap = Button->GetBitmapLabel();
        iconDC.SelectObject( ButtBitmap );
        int             buttcolor = CurrentColor[ii];
        wxBrush         Brush;
        iconDC.SetPen( *wxBLACK_PEN );
        Brush.SetColour(
            ColorRefs[buttcolor].m_Red,
            ColorRefs[buttcolor].m_Green,
            ColorRefs[buttcolor].m_Blue
            );
        Brush.SetStyle( wxSOLID );

        iconDC.SetBrush( Brush );
        iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );
        Button->SetBitmapLabel( ButtBitmap );
        Button->Refresh();
    }
    Refresh( FALSE );
}


/******************************************************************/
void WinEDA_SetColorsFrame::UpdateLayerSettings()
/******************************************************************/
{
    for( int ii = 0; laytool_list[ii] != NULL; ii++ )
    {
//      if( laytool_list[ii]->m_CheckBox == NULL )
//          continue;

        // Although some of the items listed within laytool_list[]
        // do not have any checkboxes associated with them, the
        // previous command is still not necessary (as those items
        // are processed satisfactorily by the following command).

        if( !laytool_list[ii]->m_NoDisplayIsColor
           && (laytool_list[ii]->m_NoDisplay == NULL) )
            continue;

        if( laytool_list[ii]->m_NoDisplayIsColor )
        {
            if( laytool_list[ii]->m_CheckBox->GetValue() )
                *laytool_list[ii]->m_Color = CurrentColor[ii] & ~ITEM_NOT_SHOW;
            else
                *laytool_list[ii]->m_Color = CurrentColor[ii] | ITEM_NOT_SHOW;
        }
        else
        {
            if( laytool_list[ii]->m_Color )
                *laytool_list[ii]->m_Color = CurrentColor[ii];

//          if( laytool_list[ii]->m_CheckBox )
//              *laytool_list[ii]->m_NoDisplay = laytool_list[ii]->m_CheckBox->GetValue();

            // As there is a checkbox associated with every layer listed
            // within this particular dialog box, the previous command can
            // be replaced with this following command.

            *laytool_list[ii]->m_NoDisplay = laytool_list[ii]->m_CheckBox->GetValue();
        }
    }
    // Additional command required for updating visibility of grid.
    m_Parent->m_Draw_Grid = g_ShowGrid;
}


/**********************************************************************/
void WinEDA_SetColorsFrame::ResetDisplayLayersCu( wxCommandEvent& event )
/**********************************************************************/
{
    bool NewState = (event.GetId() == ID_COLOR_RESET_SHOW_LAYER_ON) ? TRUE : FALSE;

    for( int ii = 1; ii < 17; ii++ )
    {
        if( laytool_list[ii]->m_CheckBox == NULL )
            continue;
        laytool_list[ii]->m_CheckBox->SetValue( NewState );
    }
}
