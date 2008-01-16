/***************************************************/
/* class and functions to handle a graphic segment */
/****************************************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "trigo.h"

/* DRAWSEGMENT: constructor */
DRAWSEGMENT::DRAWSEGMENT( BOARD_ITEM* StructFather, KICAD_T idtype ) :
    BOARD_ITEM( StructFather, idtype )
{
    m_Width = m_Flags = m_Shape = m_Type = m_Angle = 0;
}


/* destructor */
DRAWSEGMENT:: ~DRAWSEGMENT()
{
}


void DRAWSEGMENT::UnLink()

/**
 * Function UnLink
 * remove item from linked list.
 */
{
    /* ereas back link */
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

    /* erase forward link */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/*******************************************/
void DRAWSEGMENT::Copy( DRAWSEGMENT* source )
/*******************************************/
{
    m_Type      = source->m_Type;
    m_Layer     = source->m_Layer;
    m_Width     = source->m_Width;
    m_Start     = source->m_Start;
    m_End       = source->m_End;
    m_Shape     = source->m_Shape;
    m_Angle     = source->m_Angle;
    m_TimeStamp = source->m_TimeStamp;
}


bool DRAWSEGMENT::Save( FILE* aFile ) const
{
    if( GetState( DELETED ) )
        return true;

    bool rc = false;

    if( fprintf( aFile, "$DRAWSEGMENT\n" ) != sizeof("$DRAWSEGMENT\n") - 1 )
        goto out;

    fprintf( aFile, "Po %d %d %d %d %d %d\n",
             m_Shape,
             m_Start.x, m_Start.y,
             m_End.x, m_End.y, m_Width );

    fprintf( aFile, "De %d %d %d %lX %X\n",
            m_Layer, m_Type, m_Angle,
            m_TimeStamp, ReturnStatus() );

    if( fprintf( aFile, "$EndDRAWSEGMENT\n" ) != sizeof("$EndDRAWSEGMENT\n") - 1 )
        goto out;

    rc = true;

out:
    return rc;
}


/******************************************************************/
bool DRAWSEGMENT::ReadDrawSegmentDescr( FILE* File, int* LineNum )
/******************************************************************/

/* Read a DRAWSEGMENT from a file
 */
{
    char Line[2048];

    while( GetLine( File, Line, LineNum ) != NULL )
    {
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return TRUE; /* End of description */
        if( Line[0] == 'P' )
        {
            sscanf( Line + 2, " %d %d %d %d %d %d",
                    &m_Shape, &m_Start.x, &m_Start.y,
                    &m_End.x, &m_End.y, &m_Width );
            if( m_Width < 0 )
                m_Width = 0;
        }

        if( Line[0] == 'D' )
        {
            int status;
            sscanf( Line + 2, " %d %d %d %lX %X",
                    &m_Layer, &m_Type, &m_Angle,
                    &m_TimeStamp, &status );

            if( m_Layer < FIRST_NO_COPPER_LAYER )
                m_Layer = FIRST_NO_COPPER_LAYER;
            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;

            SetState( status, ON );
        }
    }

    return FALSE;
}


// see pcbstruct.h
void DRAWSEGMENT::Display_Infos( WinEDA_DrawFrame* frame )
{
    int      itype;
    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    itype = m_Type & 0x0F;

    msg = wxT( "DRAWING" );

    Affiche_1_Parametre( frame, 1, _( "Type" ), msg, DARKCYAN );

    Affiche_1_Parametre( frame, 16, _( "Shape" ), wxEmptyString, RED );

    if( m_Shape == S_CIRCLE )
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Circle" ), RED );

    else if( m_Shape == S_ARC )
    {
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "  Arc  " ), RED );
        msg.Printf( wxT( "%d" ), m_Angle );
        Affiche_1_Parametre( frame, 32, wxT( " l.arc " ), msg, RED );
    }
    else
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Segment" ), RED );

    Affiche_1_Parametre( frame, 48, _( "Layer" ),
                         ReturnPcbLayerName( m_Layer ), BROWN );

    valeur_param( (unsigned) m_Width, msg );
    Affiche_1_Parametre( frame, 60, _( "Width" ), msg, DARKCYAN );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool DRAWSEGMENT::HitTest( const wxPoint& ref_pos )
{
    int ux0 = m_Start.x;
    int uy0 = m_Start.y;

    /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
    int dx = m_End.x - ux0;
    int dy = m_End.y - uy0;

    int spot_cX = ref_pos.x - ux0;
    int spot_cY = ref_pos.y - uy0;

    if( m_Shape==S_CIRCLE || m_Shape==S_ARC )
    {
        int rayon, dist, stAngle, endAngle, mouseAngle;

        rayon = (int) hypot( (double) (dx), (double) (dy) );
        dist  = (int) hypot( (double) (spot_cX), (double) (spot_cY) );

        if( abs( rayon - dist ) <= (m_Width / 2) )
        {
            if( m_Shape == S_CIRCLE )
                return true;

            /* pour un arc, controle complementaire */
            mouseAngle = (int) ArcTangente( spot_cY, spot_cX );
            stAngle    = (int) ArcTangente( dy, dx );
            endAngle   = stAngle + m_Angle;

            if( endAngle > 3600 )
            {
                stAngle  -= 3600;
                endAngle -= 3600;
            }

            if( mouseAngle >= stAngle  &&  mouseAngle <= endAngle )
                return true;
        }
    }
    else
    {
        if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
            return true;
    }
    return false;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * For now, an ending point must be inside this rect.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool DRAWSEGMENT::HitTest( EDA_Rect& refArea )
{
    if( refArea.Inside( m_Start ) )
        return true;
    if( refArea.Inside( m_End ) )
        return true;
    return false;
}
