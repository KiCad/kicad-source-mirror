/**********************/
/* Class SCH_POLYLINE */
/**********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "common.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "sch_polyline.h"


SCH_POLYLINE::SCH_POLYLINE( int layer ) :
    SCH_ITEM( NULL, SCH_POLYLINE_T )
{
    m_Width = 0;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
    case LAYER_NOTES:
    case LAYER_BUS:
        m_Layer = layer;
        break;
    }
}


SCH_POLYLINE::SCH_POLYLINE( const SCH_POLYLINE& aPolyLine ) :
    SCH_ITEM( aPolyLine )
{
    m_Width = aPolyLine.m_Width;
    m_PolyPoints = aPolyLine.m_PolyPoints;
}


SCH_POLYLINE::~SCH_POLYLINE()
{
}


EDA_ITEM* SCH_POLYLINE::doClone() const
{
    return new SCH_POLYLINE( *this );
}


bool SCH_POLYLINE::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";

    if( GetLayer() == LAYER_BUS )
        layer = "Bus";

    if( fprintf( aFile, "Poly %s %s %d\n", width, layer, GetCornerCount() ) == EOF )
    {
        return false;
    }

    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        if( fprintf( aFile, "\t%-4d %-4d\n", m_PolyPoints[ii ].x, m_PolyPoints[ii].y ) == EOF )
        {
            success = false;
            break;
        }
    }

    return success;
}


bool SCH_POLYLINE::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char Name1[256];
    char Name2[256];
    wxPoint pt;
    int ii;
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %s %d", Name1, Name2, &ii ) != 3 )
    {
        aErrorMsg.Printf( wxT( "EESchema file polyline struct error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_NOTES;

    if( Name2[0] == 'W' )
        m_Layer = LAYER_WIRE;

    if( Name2[0] == 'B' )
        m_Layer = LAYER_BUS;

    for( unsigned jj = 0; jj < (unsigned)ii; jj++ )
    {
        wxPoint point;

        if( !aLine.ReadLine() || sscanf( ((char*) aLine), "%d %d", &pt.x, &pt.y ) != 2 )
        {
            aErrorMsg.Printf( wxT( "EESchema file polyline struct error at line %d, aborted" ),
                              aLine.LineNumber() );
            aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
            return false;
        }

        AddPoint( pt );
    }

    return true;
}


int SCH_POLYLINE::GetPenSize() const
{
    int pensize = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    return pensize;
}


void SCH_POLYLINE::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                         int aDrawMode, int aColor )
{
    int color;
    int width = GetPenSize();

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    if( m_Layer == LAYER_BUS )
    {
        width *= 3;
    }

    GRMoveTo( m_PolyPoints[0].x, m_PolyPoints[0].y );

    if( m_Layer == LAYER_NOTES )
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRDashedLineTo( &aPanel->m_ClipBox, aDC, m_PolyPoints[i].x + aOffset.x,
                            m_PolyPoints[i].y + aOffset.y, width, color );
    }
    else
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRLineTo( &aPanel->m_ClipBox, aDC, m_PolyPoints[i].x + aOffset.x,
                      m_PolyPoints[i].y + aOffset.y, width, color );
    }
}


void SCH_POLYLINE::Mirror_X( int aXaxis_position )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        m_PolyPoints[ii].y -= aXaxis_position;
        NEGATE(  m_PolyPoints[ii].y );
        m_PolyPoints[ii].y = aXaxis_position;
    }
}


void SCH_POLYLINE::Mirror_Y( int aYaxis_position )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        m_PolyPoints[ii].x -= aYaxis_position;
        NEGATE(  m_PolyPoints[ii].x );
        m_PolyPoints[ii].x = aYaxis_position;
    }
}


void SCH_POLYLINE::Rotate( wxPoint rotationPoint )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        RotatePoint( &m_PolyPoints[ii], rotationPoint, 900 );
    }
}


bool SCH_POLYLINE::doHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & ( DRAW_ITEM_T | WIRE_T | BUS_T ) ) )
        return false;

    for( size_t i = 0;  i < m_PolyPoints.size() - 1;  i++ )
    {
        if( TestSegmentHit( aPoint, m_PolyPoints[i], m_PolyPoints[i + 1], aAccuracy ) )
            return true;
    }

    return false;
}


bool SCH_POLYLINE::doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}
