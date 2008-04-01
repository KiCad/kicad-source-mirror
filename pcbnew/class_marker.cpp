/*****************************************************************************/
/* Functions to handle markers used to show somthing (usually a drc problem) */
/*****************************************************************************/

/* file class_marker.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "class_marker.h"


/* Routines Locales : */


/* Default bitmap shape for markers */
static char Default_MarkerBitmap[] =
{
    12, 12,                                 /* x and y size of the bitmap */
    1,  1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,    /* bitmap: 1 = color, 0 = notrace */
    1,  1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0,
    1,  1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0,
    1,  0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    1,  1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1,  1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    1,  1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0,  0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0,  0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
    0,  0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0,
    0,  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0
};


/*******************/
/* Classe MARKER */
/*******************/

void MARKER::init()
{
    m_Bitmap = NULL;
    m_Type   = 0;
    m_Color  = RED;
    m_Bitmap = Default_MarkerBitmap;
    m_Size.x = Default_MarkerBitmap[0];
    m_Size.y = Default_MarkerBitmap[1];
}

MARKER::MARKER( BOARD_ITEM* StructFather ) :
    BOARD_ITEM( StructFather, TYPEMARKER ),
    m_drc()
{
    init();
}


MARKER::MARKER( int aErrorCode, const wxPoint& aMarkerPos,
               const wxString& aText, const wxPoint& aPos,
               const wxString& bText, const wxPoint& bPos ) :
    BOARD_ITEM( NULL, TYPEMARKER )  // parent set during BOARD::Add()
{
    init();

    SetData( aErrorCode, aMarkerPos,
         aText, aPos,
         bText, bPos );
}

MARKER::MARKER( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos ) :
    BOARD_ITEM( NULL, TYPEMARKER )  // parent set during BOARD::Add()
{
    init();

    SetData( aErrorCode, aMarkerPos,
         aText, aPos );
}


/* Effacement memoire de la structure */
MARKER::~MARKER()
{
#if defined(DEBUG)
    printf("MARKER %p deleted\n", this );
#endif
}


void MARKER::SetData( int aErrorCode, const wxPoint& aMarkerPos,
         const wxString& aText, const wxPoint& aPos,
         const wxString& bText, const wxPoint& bPos )
{
    m_drc.SetData( aErrorCode, aMarkerPos,
             aText, bText,
             aPos, bPos );

    // @todo: switch on error code to set error code specific color, and possibly bitmap.
    m_Color = WHITE;
}


void MARKER::SetData( int aErrorCode, const wxPoint& aMarkerPos,
         const wxString& aText, const wxPoint& aPos )
{
    m_drc.SetData( aErrorCode, aMarkerPos,
             aText, aPos );

    // @todo: switch on error code to set error code specific color, and possibly bitmap.
    m_Color = WHITE;
}


/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void MARKER::UnLink()
{
    wxFAIL_MSG( wxT("MARKER::UnLink is deprecated") );
}


void MARKER::Display_Infos( WinEDA_DrawFrame* frame )
{
    int      text_pos;

    frame->MsgPanel->EraseMsgBox();

    const DRC_ITEM& rpt = m_drc;

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), _("Marker"), DARKCYAN );

    wxString errorTxt;

    errorTxt << _("ErrType") << wxT("(") << rpt.GetErrorCode() << wxT(")-  ") << rpt.GetErrorText() << wxT(":");

    text_pos = 5;
    Affiche_1_Parametre( frame, text_pos, errorTxt, wxEmptyString, RED );

    wxString txtA;
    txtA << DRC_ITEM::ShowCoord( rpt.GetPointA() ) << wxT(": ") << rpt.GetTextA();

    wxString txtB;
    if ( rpt.HasSecondItem() )
        txtB << DRC_ITEM::ShowCoord( rpt.GetPointB() ) << wxT(": ") << rpt.GetTextB();

    text_pos = 25;
    Affiche_1_Parametre( frame, text_pos, txtA, txtB, DARKBROWN );
}


/**********************************************/
bool MARKER::HitTest( const wxPoint& refPos )
/**********************************************/
{
    // the MARKER is 12 pixels by 12 pixels, but is not resized with zoom, so
    // as zoom changes, the effective real size (in user units) of the MARKER changes.

    wxSize TrueSize = m_Size;
    if ( ActiveScreen )
    {
        TrueSize.x *= ActiveScreen->GetZoom();
        TrueSize.y *= ActiveScreen->GetZoom();
    }

    wxPoint pos = GetPosition();

    int dx = refPos.x - pos.x;
    int dy = refPos.y - pos.y;

    /* is refPos in the box: Marker size to right an bottom,
    or size/2 to left or top */
    if( dx <= TrueSize.x  && dy <= TrueSize.y &&
        dx >= -TrueSize.x/2  && dy >= -TrueSize.y/2 )
        return true;
    else
        return false;
}



/**********************************************************************/
void MARKER::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode, const wxPoint& offset )
/**********************************************************************/

/*
 *  Trace un repere sur l'ecran au point de coordonnees PCB pos
 *  Le marqueur est defini par un tableau de 2 + (lig*col) elements:
 *   1er element: dim nbre ligne
 *   2er element: dim nbre col
 *   suite: lig * col elements a 0 ou 1 : si 1 mise a color du pixel
 */
{
    int   px, py;
    int   ii, jj;
    char* pt_bitmap = m_Bitmap;

    if( pt_bitmap == NULL ) return;

    GRSetDrawMode( DC, DrawMode );

    px = GRMapX( GetPosition().x );
    py = GRMapY( GetPosition().y );

    /* Get the bitmap size */
    m_Size.x = *(pt_bitmap++);
    m_Size.y = *(pt_bitmap++);

    /* Draw the bitmap */
    for( ii = 0; ii < m_Size.x; ii++ )
    {
        for( jj = 0; jj < m_Size.y; jj++, pt_bitmap++ )
        {
            if( *pt_bitmap )
                GRSPutPixel( &panel->m_ClipBox, DC,
                             px + ii, py + jj, m_Color );
        }
    }
}
