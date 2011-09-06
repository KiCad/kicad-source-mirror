/***************************************************/
/* class and functions to handle a graphic segment */
/****************************************************/

#include "fctsys.h"
#include "wxstruct.h"
#include "gr_basic.h"
#include "bezier_curves.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "colors_selection.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"

#include "trigo.h"
#include "protos.h"
#include "richio.h"

DRAWSEGMENT::DRAWSEGMENT( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype )
{
    m_Width = m_Flags = m_Type = m_Angle = 0;
    m_Shape = S_SEGMENT;
}


DRAWSEGMENT:: ~DRAWSEGMENT()
{
}


void DRAWSEGMENT::Copy( DRAWSEGMENT* source )
{
    if( source == NULL )
        return;

    m_Type         = source->m_Type;
    m_Layer        = source->m_Layer;
    m_Width        = source->m_Width;
    m_Start        = source->m_Start;
    m_End          = source->m_End;
    m_Shape        = source->m_Shape;
    m_Angle        = source->m_Angle;
    m_TimeStamp    = source->m_TimeStamp;
    m_BezierC1     = source->m_BezierC1;
    m_BezierC2     = source->m_BezierC1;
    m_BezierPoints = source->m_BezierPoints;
}


void DRAWSEGMENT::Rotate( const wxPoint& aRotCentre, int aAngle )
{
    RotatePoint( &m_Start, aRotCentre, aAngle );
    RotatePoint( &m_End, aRotCentre, aAngle );
}


void DRAWSEGMENT::Flip( const wxPoint& aCentre )
{
    m_Start.y  = aCentre.y - (m_Start.y - aCentre.y);
    m_End.y  = aCentre.y - (m_End.y - aCentre.y);
    if( m_Shape == S_ARC )
    {
        NEGATE( m_Angle );
    }
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


bool DRAWSEGMENT::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "$DRAWSEGMENT\n" ) != sizeof("$DRAWSEGMENT\n") - 1 )
        return false;

    fprintf( aFile, "Po %d %d %d %d %d %d\n",
             m_Shape,
             m_Start.x, m_Start.y,
             m_End.x, m_End.y, m_Width );

    if( m_Type != S_CURVE )
    {
        fprintf( aFile, "De %d %d %d %lX %X\n",
                 m_Layer, m_Type, m_Angle,
                 m_TimeStamp, ReturnStatus() );
    }
    else
    {
        fprintf( aFile, "De %d %d %d %lX %X %d %d %d %d\n",
                 m_Layer, m_Type, m_Angle,
                 m_TimeStamp, ReturnStatus(),
                 m_BezierC1.x,m_BezierC1.y,
                 m_BezierC2.x,m_BezierC2.y);
    }

    if( fprintf( aFile, "$EndDRAWSEGMENT\n" ) != sizeof("$EndDRAWSEGMENT\n") - 1 )
        return false;

    return true;
}


bool DRAWSEGMENT::ReadDrawSegmentDescr( LINE_READER* aReader )
{
    char* Line;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

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
            char* token = 0;

            token = strtok( Line," " );

            for( int i = 0; (token = strtok( NULL," " )) != NULL; i++ )
            {
                switch( i )
                {
                case 0:
                    sscanf( token,"%d",&m_Layer );
                    break;
                case 1:
                    sscanf( token,"%d",&m_Type );
                    break;
                case 2:
                    sscanf( token,"%d",&m_Angle );
                    break;
                case 3:
                    sscanf( token,"%lX",&m_TimeStamp );
                    break;
                case 4:
                    sscanf( token,"%X",&status );
                    break;
                    /* Bezier Control Points*/
                case 5:
                    sscanf( token,"%d",&m_BezierC1.x );
                    break;
                case 6:
                    sscanf( token,"%d",&m_BezierC1.y );
                    break;
                case 7:
                    sscanf( token,"%d",&m_BezierC2.x );
                    break;
                case 8:
                    sscanf( token,"%d",&m_BezierC2.y );
                    break;
                default:
                    break;
                }
            }

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
    wxPoint endPoint;         // start of arc

    switch( m_Shape )
    {
    case S_ARC:
        // rotate the starting point of the arc, given by m_End, through the
        // angle m_Angle to get the ending point of the arc.
        // m_Start is the arc centre
        endPoint  = m_End;         // m_End = start point of arc
        RotatePoint( &endPoint, m_Start, -m_Angle );
        return endPoint;   // after rotation, the end of the arc.
        break;

    case S_SEGMENT:
    default:
        return m_End;
    }
}


MODULE* DRAWSEGMENT::GetParentModule() const
{
    if( m_Parent->Type() != TYPE_MODULE )
        return NULL;

    return (MODULE*) m_Parent;
}


void DRAWSEGMENT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode, const wxPoint& aOffset )
{
    int ux0, uy0, dx, dy;
    int l_piste;
    int color, mode;
    int rayon;

    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( GetLayer() ) == false )
        return;

    color = brd->GetLayerColor( GetLayer() );

    GRSetDrawMode( DC, draw_mode );
    l_piste = m_Width >> 1;  /* half trace width */

    // Line start point or Circle and Arc center
    ux0 = m_Start.x + aOffset.x;
    uy0 = m_Start.y + aOffset.y;

    // Line end point or circle and arc start point
    dx = m_End.x + aOffset.x;
    dy = m_End.y + aOffset.y;

    mode = DisplayOpt.DisplayDrawItems;
    if( m_Flags & FORCE_SKETCH )
        mode = SKETCH;

    if( l_piste < DC->DeviceToLogicalXRel( L_MIN_DESSIN ) )
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
        int StAngle, EndAngle;
        rayon    = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        StAngle  = (int) ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;

        if ( ! panel->m_PrintIsMirrored)
        {
            if( StAngle > EndAngle )
                EXCHG( StAngle, EndAngle );
        }
        else    // Mirrored mode: arc orientation is reversed
        {
            if( StAngle < EndAngle )
                EXCHG( StAngle, EndAngle );
        }


        if( mode == FILAIRE )
            GRArc( &panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
                   rayon, color );

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
        break;
    case S_CURVE:
            m_BezierPoints = Bezier2Poly(m_Start,m_BezierC1, m_BezierC2, m_End);

            for (unsigned int i=1; i < m_BezierPoints.size(); i++) {
                if( mode == FILAIRE )
                    GRLine( &panel->m_ClipBox, DC,
                            m_BezierPoints[i].x, m_BezierPoints[i].y,
                            m_BezierPoints[i-1].x, m_BezierPoints[i-1].y, 0,
                            color );
                else if( mode == SKETCH )
                {
                    GRCSegm( &panel->m_ClipBox, DC,
                            m_BezierPoints[i].x, m_BezierPoints[i].y,
                            m_BezierPoints[i-1].x, m_BezierPoints[i-1].y,
                             m_Width, color );
                }
                else
                {
                    GRFillCSegm( &panel->m_ClipBox, DC,
                                m_BezierPoints[i].x, m_BezierPoints[i].y,
                                m_BezierPoints[i-1].x, m_BezierPoints[i-1].y,
                                 m_Width, color );
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
void DRAWSEGMENT::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString msg;
    wxString coords;

    BOARD*   board = (BOARD*) m_Parent;
    wxASSERT( board );

    frame->ClearMsgPanel();

    msg = wxT( "DRAWING" );

    frame->AppendMsgPanel( _( "Type" ), msg, DARKCYAN );

    wxString    shape = _( "Shape" );

    switch( m_Shape ) {
        case S_CIRCLE:
            frame->AppendMsgPanel( shape, _( "Circle" ), RED );
            break;

        case S_ARC:
            frame->AppendMsgPanel( shape, _( "Arc" ), RED );

            msg.Printf( wxT( "%.1f" ), (double)m_Angle/10 );
            frame->AppendMsgPanel( _("Angle"), msg, RED );
            break;
        case S_CURVE:
            frame->AppendMsgPanel( shape, _( "Curve" ), RED );
            break;

        default:
            frame->AppendMsgPanel( shape, _( "Segment" ), RED );
    }
    wxString start;
    start << GetStart();

    wxString end;
    end << GetEnd();

    frame->AppendMsgPanel( start, end, DARKGREEN );

    frame->AppendMsgPanel( _( "Layer" ),
                         board->GetLayerName( m_Layer ), DARKBROWN );

    valeur_param( (unsigned) m_Width, msg );
    frame->AppendMsgPanel( _( "Width" ), msg, DARKCYAN );
}


EDA_RECT DRAWSEGMENT::GetBoundingBox() const
{
    EDA_RECT bbox;

    bbox.SetOrigin( m_Start );

    switch( m_Shape )
    {
    case S_SEGMENT:
        bbox.SetEnd( m_End );
        bbox.Inflate( (m_Width / 2) + 1 );
        break;

    case S_CIRCLE:
        bbox.Inflate( GetRadius() + 1 );
        break;

    case S_ARC:
    {
        bbox.Inflate( GetRadius() + 1 );
    }
    break;

    case S_POLYGON:
    {
        wxPoint p_end;
        MODULE* module = GetParentModule();

        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        {
            wxPoint pt = m_PolyPoints[ii];

            if( module ) // Transform, if we belong to a module
            {
                RotatePoint( &pt, module->m_Orient );
                pt += module->m_Pos;
            }

            if( ii == 0 )
                p_end = pt;
            bbox.m_Pos.x = MIN( bbox.m_Pos.x, pt.x );
            bbox.m_Pos.y = MIN( bbox.m_Pos.y, pt.y );
            p_end.x   = MAX( p_end.x, pt.x );
            p_end.y   = MAX( p_end.y, pt.y );
        }

        bbox.SetEnd(p_end);
        bbox.Inflate( 1 );
        break;
    }
    }

    bbox.Inflate( (m_Width+1) / 2 );
    return bbox;
}


bool DRAWSEGMENT::HitTest( const wxPoint& aRefPos )
{
    /* Calculate coordinates to test relative to segment origin. */
    wxPoint relPos = aRefPos - m_Start;

    switch( m_Shape )
    {
    case S_CIRCLE:
    case S_ARC:
    {
        int rayon = GetRadius();
        int dist  = (int) hypot( (double) relPos.x, (double) relPos.y );

        if( abs( rayon - dist ) <= ( m_Width / 2 ) )
        {
            if( m_Shape == S_CIRCLE )
                return true;

            int mouseAngle = ArcTangente( relPos.y, relPos.x );
            int stAngle    = ArcTangente( m_End.y - m_Start.y, m_End.x - m_Start.x );
            int endAngle   = stAngle + m_Angle;

            if( endAngle > 3600 )
            {
                stAngle  -= 3600;
                endAngle -= 3600;
            }

            if( mouseAngle >= stAngle && mouseAngle <= endAngle )
                return true;
        }
    }
    break;

    case S_CURVE:
        for( unsigned int i= 1; i < m_BezierPoints.size(); i++)
        {
            if( TestSegmentHit( aRefPos,m_BezierPoints[i-1],
                                m_BezierPoints[i-1], m_Width / 2 ) )
                return true;
        }
        break;

    case S_SEGMENT:
        if( TestSegmentHit( aRefPos, m_Start, m_End, m_Width / 2 ) )
            return true;
        break;

    default:
        wxASSERT( 0 );
        break;
    }
    return false;
}


bool DRAWSEGMENT::HitTest( EDA_RECT& refArea )
{
    switch( m_Shape )
    {
        case S_CIRCLE:
        {
            int radius = GetRadius();
            // Text if area intersects the circle:
            EDA_RECT area = refArea;
            area.Inflate( radius );
            if( area.Contains( m_Start ) )
                return true;
        }
        break;

        case S_ARC:
        case S_SEGMENT:
            if( refArea.Contains( GetStart() ) )
                return true;
            if( refArea.Contains( GetEnd() ) )
                return true;
            break;
    }
    return false;
}


wxString DRAWSEGMENT::GetSelectMenuText() const
{
    wxString text;
    wxString temp;

    text << _( "Pcb Graphic" ) << wxT(": ") << ShowShape( (Track_Shapes)m_Shape )
         << wxChar(' ') << _( "Length:" ) << valeur_param( GetLength(), temp )
         << _( " on " ) << GetLayerName();

    return text;
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
