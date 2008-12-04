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
DRAWSEGMENT::DRAWSEGMENT( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype )
{
    m_Width = m_Flags = m_Shape = m_Type = m_Angle = 0;
}


/* destructor */
DRAWSEGMENT:: ~DRAWSEGMENT()
{
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


wxPoint DRAWSEGMENT::GetStart() const
{
    switch( m_Shape )
    {
    case S_ARC:
        return m_End;  // the start of the arc is held in field m_End, center point is in m_Start.

    case S_SEGMENT:
    default:
        return m_Start;
    }
}


wxPoint DRAWSEGMENT::GetEnd() const
{
    switch( m_Shape )
    {
    case S_ARC:
        {
            // rotate the starting point of the arc, given by m_End, through the angle m_Angle
            // to get the ending point of the arc.
            wxPoint center = m_Start;       // center point of the arc
            wxPoint start  = m_End;         // start of arc

            RotatePoint( &start.x, &start.y, center.x, center.y, -m_Angle );

            return start;   // after rotation, the end of the arc.
        }
        break;

    case S_SEGMENT:
    default:
        return m_End;
    }
}



void DRAWSEGMENT::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                      int draw_mode, const wxPoint& notUsed )
{
    int ux0, uy0, dx, dy;
    int l_piste;
    int color, mode;
    int zoom;
    int rayon;

    color = g_DesignSettings.m_LayerColor[GetLayer()];
    if( color & ITEM_NOT_SHOW )
        return;

    if( panel )
        zoom = panel->GetZoom();
    else
        zoom = ActiveScreen->GetZoom();

    GRSetDrawMode( DC, draw_mode );
    l_piste = m_Width >> 1;  /* l_piste = demi largeur piste */

    /* coord de depart */
    ux0 = m_Start.x;
    uy0 = m_Start.y;

    /* coord d'arrivee */
    dx = m_End.x;
    dy = m_End.y;

    mode = DisplayOpt.DisplayDrawItems;
    if( m_Flags & FORCE_SKETCH )
        mode = SKETCH;
    if( l_piste < (L_MIN_DESSIN * zoom) )
        mode = FILAIRE;

    switch( m_Shape )
    {
    case S_CIRCLE:
        rayon = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        if( mode == FILAIRE )
        {
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon, color );
        }
        else if( mode == SKETCH )
        {
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon - l_piste, color );
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon + l_piste, color );
        }
        else
        {
            GRCircle( &panel->m_ClipBox, DC, ux0, uy0, rayon, m_Width, color );
        }
        break;

    case S_ARC:
        {
            int StAngle, EndAngle;
            rayon    = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
            StAngle  = (int) ArcTangente( dy - uy0, dx - ux0 );
            EndAngle = StAngle + m_Angle;

            if ( ! panel->m_PrintIsMirrored)
            {
                if( StAngle > EndAngle )
                    EXCHG( StAngle, EndAngle );
            }
            else    //Mirrored mode: arc orientation is reversed
            {
                if( StAngle < EndAngle )
                    EXCHG( StAngle, EndAngle );
            }


            if( mode == FILAIRE )
                GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon, color );

            else if( mode == SKETCH )
            {
                GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                       rayon - l_piste, color );
                GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                       rayon + l_piste, color );
            }
            else
            {
                GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                       rayon, m_Width, color );
            }
        }
        break;

    default:
        if( mode == FILAIRE )
            GRLine( &panel->m_ClipBox, DC, ux0, uy0, dx, dy, 0, color );
        else if( mode == SKETCH )
        {
            GRCSegm( &panel->m_ClipBox, DC, ux0, uy0, dx, dy,
                     m_Width, color );
        }
        else
        {
            GRFillCSegm( &panel->m_ClipBox, DC, ux0, uy0, dx, dy,
                         m_Width, color );
        }
        break;
    }
}


// see pcbstruct.h
void DRAWSEGMENT::Display_Infos( WinEDA_DrawFrame* frame )
{
    int      itype;
    wxString msg;
    wxString coords;

    BOARD*   board = (BOARD*) m_Parent;
    wxASSERT( board );

    frame->MsgPanel->EraseMsgBox();

    itype = m_Type & 0x0F;

    msg = wxT( "DRAWING" );

    Affiche_1_Parametre( frame, 1, _( "Type" ), msg, DARKCYAN );

    wxString    shape = _( "Shape" );

    if( m_Shape == S_CIRCLE )
        Affiche_1_Parametre( frame, 10, shape, _( "Circle" ), RED );

    else if( m_Shape == S_ARC )
    {
        Affiche_1_Parametre( frame, 10, shape, _( "Arc" ), RED );

        msg.Printf( wxT( "%d.%d" ), m_Angle/10, m_Angle % 10 );
        Affiche_1_Parametre( frame, 18, _("Angle"), msg, RED );
    }
    else
        Affiche_1_Parametre( frame, 10, shape, _( "Segment" ), RED );

    wxString start;
    start << GetStart();

    wxString end;
    end << GetEnd();

    Affiche_1_Parametre( frame, 22, start, end, BLACK );

    Affiche_1_Parametre( frame, 36, _( "Layer" ),
                         board->GetLayerName( m_Layer ), BROWN );

    valeur_param( (unsigned) m_Width, msg );

    Affiche_1_Parametre( frame, 50, _( "Width" ), msg, DARKCYAN );
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


#if defined(DEBUG)
/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void DRAWSEGMENT::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<

    " shape=\"" << m_Shape << '"' <<
/*
    " layer=\"" << GetLayer() << '"' <<
    " width=\"" << m_Width << '"' <<
    " angle=\"" << m_Angle << '"' <<  // Used only for Arcs: Arc angle in 1/10 deg
*/
    '>' <<
    "<start" << m_Start << "/>" <<
    "<end"   << m_End << "/>"
    "<GetStart" << GetStart() << "/>" <<
    "<GetEnd"   << GetEnd() << "/>"
    ;

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif
