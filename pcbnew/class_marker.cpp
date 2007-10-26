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
    12, 12,                                 /* x and y sise of the bitmap */
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
/* Classe MARQUEUR */
/*******************/

MARQUEUR::MARQUEUR( BOARD_ITEM* StructFather ) :
    BOARD_ITEM( StructFather, TYPEMARQUEUR )
{
    m_Bitmap = NULL;
    m_Type   = 0;
    m_Color  = RED;
    m_Bitmap = Default_MarkerBitmap;
	m_Size.x = Default_MarkerBitmap[0]; 
    m_Size.y = Default_MarkerBitmap[1];
}


/* Effacement memoire de la structure */
MARQUEUR:: ~MARQUEUR()
{
}


/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void MARQUEUR::UnLink()
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->Type() != TYPEPCB )
        {
            Pback->Pnext = Pnext;
        }
        else /* Le chainage arriere pointe sur la structure "Pere" */
        {
            ( (BOARD*) Pback )->m_Drawings = (BOARD_ITEM*) Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


void MARQUEUR::Display_Infos( WinEDA_DrawFrame* frame )
{
    int      text_pos;

    frame->MsgPanel->EraseMsgBox();

    text_pos = 1;
    Affiche_1_Parametre( frame, text_pos, _( "Type" ), _("Marker"), DARKCYAN );

    text_pos = 12;
    Affiche_1_Parametre( frame, text_pos, _( "Marker Error Text" ), m_Diag, RED );
}


/**********************************************/
bool MARQUEUR::HitTest( const wxPoint& refPos )
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
    
    int dx = refPos.x - m_Pos.x;
    int dy = refPos.y - m_Pos.y;
	
	/* is refPos in the box: Marker size to right an bottom,
	or size/2 to left or top */
    if( dx <= TrueSize.x  && dy <= TrueSize.y &&
		dx >= -TrueSize.x/2  && dy >= -TrueSize.y/2 )
        return true;
    else
        return false;
}



/**********************************************************************/
void MARQUEUR::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode )
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

    px = GRMapX( m_Pos.x ); 
    py = GRMapY( m_Pos.y );

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
