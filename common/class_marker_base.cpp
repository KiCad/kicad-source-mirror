/**********************************************************************************
* class MARKER_BASE; markers are used to show something (usually a drc/erc problem)
* Markers in pcbnew and eeschema are derived from this basic class
**********************************************************************************/

/* file class_marker_base.cpp
*/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_base_screen.h"
#include "common.h"
#include "class_drawpanel.h"
#include "class_marker_base.h"


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
/* Classe MARKER_BASE */
/*******************/

void MARKER_BASE::init()
{
    m_Bitmap = NULL;
    m_MarkerType   = 0;
    m_Color  = RED;
    m_Bitmap = Default_MarkerBitmap;
    m_Size.x = Default_MarkerBitmap[0];
    m_Size.y = Default_MarkerBitmap[1];
}

MARKER_BASE::MARKER_BASE( )
{
    init();
}


MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
               const wxString& aText, const wxPoint& aPos,
               const wxString& bText, const wxPoint& bPos )
{
    init();

    SetData( aErrorCode,aMarkerPos,
         aText, aPos,
         bText, bPos );
}

MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos )
{
    init();

    SetData( aErrorCode, aMarkerPos, aText, aPos );
}


/* Effacement memoire de la structure */
MARKER_BASE::~MARKER_BASE()
{
}


void MARKER_BASE::SetData( int aErrorCode, const wxPoint& aMarkerPos,
         const wxString& aText, const wxPoint& aPos,
         const wxString& bText, const wxPoint& bPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aErrorCode,
             aText, bText, aPos, bPos );

    // @todo: switch on error code to set error code specific color, and possibly bitmap.
    m_Color = WHITE;
}


void MARKER_BASE::SetData( int aErrorCode, const wxPoint& aMarkerPos,
         const wxString& aText, const wxPoint& aPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aErrorCode,
             aText, aPos );

    // @todo: switch on error code to set error code specific color, and possibly bitmap.
    m_Color = WHITE;
}



/**********************************************/
bool MARKER_BASE::HitTestMarker( const wxPoint& refPos )
/**********************************************/
{
    // the MARKER_BASE is 12 pixels by 12 pixels, but is not resized with zoom, so
    // as zoom changes, the effective real size (in user units) of the MARKER_BASE changes.

    wxSize TrueSize = m_Size;
    if ( ActiveScreen )
    {
        ActiveScreen->Unscale( TrueSize );
    }

    wxPoint pos = m_Pos;

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
void MARKER_BASE::DrawMarker( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode,
                                                const wxPoint& offset )
/**********************************************************************/

/*
 *  Trace un repere sur l'ecran au point de coordonnees PCB pos
 *  Le marqueur est defini par un tableau de 2 + (lig*col) elements:
 *   1er element: dim nbre ligne
 *   2er element: dim nbre col
 *   suite: lig * col elements a 0 ou 1 : si 1 mise a color du pixel
 */
{
    int   ii, jj;
    char* pt_bitmap = m_Bitmap;

    if( pt_bitmap == NULL ) return;

    GRSetDrawMode( DC, DrawMode );

    wxPoint pos = m_Pos;
    pos.x = GRMapX( pos.x );
    pos.y = GRMapY( pos.y );

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
                             pos.x + ii, pos.y + jj, m_Color );
        }
    }
}
