/* Set up color Layers */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"

#include "protos.h"


/* Variables locales */

/* Fonctions locales: */

/* Macro utile : */
#define ADR( numlayer ) & (g_LayerDescr.LayerColor[numlayer])

#define BUTT_SIZE_X 30
#define BUTT_SIZE_Y 20


enum col_sel_id {
    ID_COLOR_SETUP = 1800
};

/**********************************/
/* Liste des menus de Menu_Layers */
/**********************************/
struct ColorButton
{
    wxString        m_Name;
    int*            m_Color;
    int             m_Id;
    wxBitmapButton* m_Button;
    int             m_State;
};

static ColorButton Msg_General =
{
    _( "General" ),                    /* Title */
    NULL
};

static ColorButton Msg_Sheets =
{
    _( "Sheets" ),                 /* Title */
    NULL
};

static ColorButton Layer_Wire_Item =
{
    _( "Wire" ),                    /* Title */
    ADR( LAYER_WIRE )               /* adr du parametre optionnel */
};

static ColorButton Layer_Bus_Item =
{
    _( "Bus" ),                     /* Title */
    ADR( LAYER_BUS )                /* adr du parametre optionnel */
};

static ColorButton Layer_Jonction_Item =
{
    _( "Junction" ),                /* Title */
    ADR( LAYER_JUNCTION )           /* adr du parametre optionnel */
};

static ColorButton Layer_LocalLabel_Item =
{
    _( "Label" ),               /* Title */
    ADR( LAYER_LOCLABEL )       /* adr du parametre optionnel */
};

static ColorButton Layer_GlobLabel_Item =
{
    _( "GlobLabel" ),           /* Title */
    ADR( LAYER_GLOBLABEL )      /* adr du parametre optionnel */
};

static ColorButton Layer_PinNum_Item =
{
    _( "PinNum" ),              /* Title */
    ADR( LAYER_PINNUM )         /* adr du parametre optionnel */
};

static ColorButton Layer_PinNam_Item =
{
    _( "PinNam" ),              /* Title */
    ADR( LAYER_PINNAM )         /* adr du parametre optionnel */
};

static ColorButton Layer_Reference_Item =
{
    _( "Reference" ),               /* Title */
    ADR( LAYER_REFERENCEPART )      /* adr du parametre optionnel */
};

static ColorButton Layer_Value_Item =
{
    _( "Value" ),               /* Title */
    ADR( LAYER_VALUEPART )      /* adr du parametre optionnel */
};

static ColorButton Layer_Fields_Item =
{
    _( "Fields" ),              /* Title */
    ADR( LAYER_FIELDS )         /* adr du parametre optionnel */
};

static ColorButton Layer_BodyDevice_Item =
{
    _( "Body" ),                    /* Title */
    ADR( LAYER_DEVICE )             /* adr du parametre optionnel */
};

static ColorButton Layer_BodyBackgroundDevice_Item =
{
    _( "Body Bg" ),                         /* Title */
    ADR( LAYER_DEVICE_BACKGROUND )          /* adr du parametre optionnel */
};

static ColorButton MsgDevice_Item =
{
    _( "Device" ),                 /* Title */
    NULL
};

static ColorButton Layer_Notes_Item =
{
    _( "Notes" ),               /* Title */
    ADR( LAYER_NOTES )          /* adr du parametre optionnel */
};

static ColorButton Layer_NetNam_Item =
{
    _( "Netname" ),                 /* Title */
    ADR( LAYER_NETNAM )             /* adr du parametre optionnel */
};

static ColorButton Layer_Pin_Item =
{
    _( "Pin" ),                     /* Title */
    ADR( LAYER_PIN )                /* adr du parametre optionnel */
};

static ColorButton Layer_Sheet_Item =
{
    _( "Sheet" ),               /* Title */
    ADR( LAYER_SHEET )          /* adr du parametre optionnel */
};

static ColorButton Layer_SheetName_Item =
{
    _( "SheetName" ),           /* Title */
    ADR( LAYER_SHEETNAME )      /* adr du parametre optionnel */
};

static ColorButton Layer_SheetFileName_Item =
{
    _( "Sheetfile" ),           /* Title */
    ADR( LAYER_SHEETFILENAME )  /* adr du parametre optionnel */
};

static ColorButton Layer_SheetLabel_Item =
{
    _( "SheetLabel" ),          /* Title */
    ADR( LAYER_SHEETLABEL )     /* adr du parametre optionnel */
};

static ColorButton Layer_NoConnect_Item =
{
    _( "NoConn" ),              /* Title */
    ADR( LAYER_NOCONNECT )      /* adr du parametre optionnel */
};


static ColorButton Msg_ErcMarck =
{
    _( "Erc Mark" ),                   /* Title */
    NULL
};

static ColorButton Layer_Erc_Warning_Item =
{
    _( "Erc Warning" ),             /* Title */
    ADR( LAYER_ERC_WARN )           /* adr du parametre optionnel */
};

static ColorButton Layer_Erc_Error_Item =
{
    _( "Erc Error" ),               /* Title */
    ADR( LAYER_ERC_ERR )            /* adr du parametre optionnel */
};

#define NB_BUTT 26
static ColorButton* laytool_list[NB_BUTT + 1] = {
    &Msg_General,
    &Layer_Wire_Item,
    &Layer_Bus_Item,
    &Layer_Jonction_Item,
    &Layer_LocalLabel_Item,
    &Layer_GlobLabel_Item,
    &Layer_NetNam_Item,
    &Layer_Notes_Item,
    &Layer_NoConnect_Item,

    &MsgDevice_Item,
    &Layer_BodyDevice_Item,
    &Layer_BodyBackgroundDevice_Item,
    &Layer_Pin_Item,
    &Layer_PinNum_Item,
    &Layer_PinNam_Item,
    &Layer_Reference_Item,
    &Layer_Value_Item,
    &Layer_Fields_Item,

    &Msg_Sheets,
    &Layer_Sheet_Item,
    &Layer_SheetFileName_Item,
    &Layer_SheetName_Item,
    &Layer_SheetLabel_Item,

    &Msg_ErcMarck,
    &Layer_Erc_Warning_Item,
    &Layer_Erc_Error_Item,

    NULL
};

/*************************************************************/
/* classe derivee pour la frame de Configuration des couleurs*/
/*************************************************************/

class WinEDA_SetColorsFrame : public wxDialog
{
private:
    WinEDA_DrawFrame* m_Parent;
    wxRadioBox*       m_SelBgColor;

public:

    // Constructor and destructor
    WinEDA_SetColorsFrame( WinEDA_DrawFrame * parent, const wxPoint &framepos );
    ~WinEDA_SetColorsFrame() { };

private:
    void    SetColor( wxCommandEvent& event );
    void    BgColorChoice( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
/* Table des evenements pour WinEDA_SetColorsFrame */
BEGIN_EVENT_TABLE( WinEDA_SetColorsFrame, wxDialog )
    EVT_RADIOBOX( ID_SEL_BG_COLOR, WinEDA_SetColorsFrame::BgColorChoice )
    EVT_COMMAND_RANGE( ID_COLOR_SETUP, ID_COLOR_SETUP + 26,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SetColorsFrame::SetColor )
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


/**********************************************************************/
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame( WinEDA_DrawFrame* parent,
                                              const wxPoint&    framepos ) :
    wxDialog( parent, -1, _( "EESchema Preferences" ), framepos,
              wxSize( 500, 270 ), DIALOG_STYLE )
/**********************************************************************/
{
#define START_Y 15
    wxBitmapButton* Button;
    int             ii, yy, butt_ID, buttcolor;
    wxPoint         pos;
    int             w = BUTT_SIZE_X, h = BUTT_SIZE_Y;
    wxStaticText*   text;
    int             right, bottom, line_height;
    wxPoint         bg_color_pos;

    m_Parent = parent;
    SetFont( *g_DialogFont );

    pos.x = 10; pos.y = START_Y;
    right = pos.x; bottom = 0;
    line_height = h;
    for( ii = 0; laytool_list[ii] != NULL; ii++ )
    {
        if( laytool_list[ii]->m_Color == NULL )
        {
            if( pos.y != START_Y )
            {
                pos.x = right + 10;
                pos.y = START_Y;
                bg_color_pos = pos;
            }
            wxString msg = wxGetTranslation( laytool_list[ii]->m_Name );
            text = new wxStaticText( this, -1,
                                     msg,
                                     wxPoint ( pos.x, pos.y ),
                                     wxSize( -1, -1 ), 0 );

            line_height = MAX( line_height, text->GetRect().GetHeight() );
            pos.y += line_height;
            continue;
        }
        butt_ID = ID_COLOR_SETUP + ii;
        laytool_list[ii]->m_Id = butt_ID;
        wxMemoryDC iconDC;
        wxBitmap ButtBitmap( w, h );

        iconDC.SelectObject( ButtBitmap );
        buttcolor = *laytool_list[ii]->m_Color;
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
        iconDC.DrawRectangle( 0, 0, w, h );

        Button = new wxBitmapButton( this, butt_ID,
                                    ButtBitmap,
                                    wxPoint ( pos.x, pos.y - (h - line_height) / 2 ),
                                    wxSize (w, h) );

        laytool_list[ii]->m_Button = Button;

        wxString msg = wxGetTranslation( laytool_list[ii]->m_Name );
        text = new wxStaticText( this, -1,
                                 msg,
                                 wxPoint (pos.x + 5 + w, pos.y ),
                                 wxSize( -1, -1 ), 0 );

        wxPoint lowpos;
        lowpos.x = text->GetRect().GetRight();
        lowpos.y = text->GetRect().GetBottom();
        right    = MAX( right, lowpos.x );
        bottom   = MAX( bottom, lowpos.y );
        bg_color_pos.y = lowpos.y;

        yy     = line_height + 5;
        pos.y += yy;
    }

    bg_color_pos.x += 5; bg_color_pos.y += 25;
    
    static const wxString bg_choice[2] = { _( "White Background" ), _( "Black Background" ) };
    
    m_SelBgColor = new wxRadioBox( this, ID_SEL_BG_COLOR,
                                   _( "Background Colour" ), bg_color_pos,
                                   wxDefaultSize, 2, bg_choice, 1, wxRA_SPECIFY_COLS );

    m_SelBgColor->SetSelection( (g_DrawBgColor == BLACK) ? 1 : 0 );
    bottom = MAX( bottom, m_SelBgColor->GetRect().GetBottom() );
    right  = MAX( right, m_SelBgColor->GetRect().GetRight() );

    SetClientSize( wxSize( right + 10, bottom + 10 ) );
}


/***************************************************************/
void WinEDA_SetColorsFrame::SetColor( wxCommandEvent& event )
/***************************************************************/
{
    int ii;
    int id = event.GetId();
    int color;
    int w = BUTT_SIZE_X, h = BUTT_SIZE_Y;

    color = DisplayColorFrame( this,
                               *laytool_list[id - ID_COLOR_SETUP]->m_Color );
    if( color < 0 )
        return;

    for( ii = 0; laytool_list[ii] != NULL; ii++ )
    {
        if( laytool_list[ii]->m_Id != id )
            continue;

        if( *laytool_list[ii]->m_Color == color )
            break;

        *laytool_list[ii]->m_Color = color;
        wxMemoryDC      iconDC;

        wxBitmapButton* Button = laytool_list[ii]->m_Button;

        wxBitmap        ButtBitmap = Button->GetBitmapLabel();
        iconDC.SelectObject( ButtBitmap );
        int             buttcolor = *laytool_list[ii]->m_Color;
        wxBrush         Brush;
        iconDC.SelectObject( ButtBitmap );
        iconDC.SetPen( *wxBLACK_PEN );
        Brush.SetColour(
            ColorRefs[buttcolor].m_Red,
            ColorRefs[buttcolor].m_Green,
            ColorRefs[buttcolor].m_Blue
            );
        Brush.SetStyle( wxSOLID );

        iconDC.SetBrush( Brush );
        iconDC.DrawRectangle( 0, 0, w, h );
        Button->SetBitmapLabel( ButtBitmap );
        if( m_Parent->GetScreen() )
            m_Parent->GetScreen()->SetRefreshReq();
    }

    Refresh( FALSE );
}


/***************************************************************/
void WinEDA_SetColorsFrame::BgColorChoice( wxCommandEvent& event )
/***************************************************************/
{
    int color;

    if( m_SelBgColor->GetSelection() == 0 )
        color = WHITE;
    else
        color = BLACK;

    if( color != g_DrawBgColor )
    {
        g_DrawBgColor = color;
        m_Parent->SetDrawBgColor( g_DrawBgColor );
        m_Parent->ReDrawPanel();
    }
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


/*******************************/
int ReturnLayerColor( int Layer )
/*******************************/
{
    if( g_LayerDescr.Flags==0 )
        return g_LayerDescr.LayerColor[Layer];
    else
        return g_LayerDescr.CommonColor;
}
