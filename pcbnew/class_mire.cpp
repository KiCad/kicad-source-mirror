/****************************************************/
/* MIRE class definition (targets for photos) */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "kicad_string.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"
#include "trigo.h"
#include "protos.h"
#include "richio.h"


MIREPCB::MIREPCB( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, TYPE_MIRE )
{
    m_Shape = 0;
    m_Size  = 5000;
}


MIREPCB::~MIREPCB()
{
}


void MIREPCB::Copy( MIREPCB* source )
{
    m_Layer     = source->m_Layer;
    m_Width     = source->m_Width;
    m_Pos       = source->m_Pos;
    m_Shape     = source->m_Shape;
    m_Size      = source->m_Size;
    m_TimeStamp = GetTimeStamp();
}


/* Read the description from the PCB file.
 */
bool MIREPCB::ReadMirePcbDescr( LINE_READER* aReader )
{
    char* Line;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return TRUE;
        if( Line[0] == 'P' )
        {
            sscanf( Line + 2, " %X %d %d %d %d %d %lX",
                    &m_Shape, &m_Layer,
                    &m_Pos.x, &m_Pos.y,
                    &m_Size, &m_Width, &m_TimeStamp );
            if( m_Layer < FIRST_NO_COPPER_LAYER )
                m_Layer = FIRST_NO_COPPER_LAYER;
            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;
        }
    }

    return FALSE;
}


bool MIREPCB::Save( FILE* aFile ) const
{
    if( GetState( DELETED ) )
        return true;

    bool rc = false;

    if( fprintf( aFile, "$MIREPCB\n" ) != sizeof("$MIREPCB\n")-1 )
        goto out;

    fprintf( aFile, "Po %X %d %d %d %d %d %8.8lX\n",
             m_Shape, m_Layer,
             m_Pos.x, m_Pos.y,
             m_Size, m_Width, m_TimeStamp );

    if( fprintf( aFile, "$EndMIREPCB\n" ) != sizeof("$EndMIREPCB\n")-1 )
        goto out;

    rc = true;

out:
    return rc;
}




/* Draw MIREPCB object: 2 segments + 1 circle
 * The circle radius is half the radius of the target
 * 2 lines have length the diameter of the target
 */
void MIREPCB::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int mode_color, const wxPoint& offset )
{
    int rayon, ox, oy, gcolor, width;
    int dx1, dx2, dy1, dy2;
    int typeaff;

    ox = m_Pos.x + offset.x;
    oy = m_Pos.y + offset.y;

    BOARD * brd =  GetBoard( );
    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    gcolor = brd->GetLayerColor(m_Layer);

    GRSetDrawMode( DC, mode_color );
    typeaff = DisplayOpt.DisplayDrawItems;
    width   = m_Width;

#ifdef USE_WX_ZOOM
    if( DC->LogicalToDeviceXRel( width ) < 2 )
#else
    if( panel->GetScreen()->Scale( width ) < 2 )
#endif
        typeaff = FILAIRE;

    rayon = m_Size / 4;

    switch( typeaff )
    {
    case FILAIRE:
        width = 0;

    case FILLED:
        GRCircle( &panel->m_ClipBox, DC, ox, oy, rayon, width, gcolor );
        break;

    case SKETCH:
        GRCircle( &panel->m_ClipBox, DC, ox, oy, rayon + (width / 2), gcolor );
        GRCircle( &panel->m_ClipBox, DC, ox, oy, rayon - (width / 2), gcolor );
        break;
    }


    rayon = m_Size / 2;
    dx1   = rayon;
    dy1   = 0;
    dx2   = 0;
    dy2   = rayon;

    if( m_Shape ) /* Form X */
    {
        dx1 = dy1 = ( rayon * 7 ) / 5;
        dx2 = dx1;
        dy2 = -dy1;
    }

    switch( typeaff )
    {
    case FILAIRE:
    case FILLED:
        GRLine( &panel->m_ClipBox, DC, ox - dx1, oy - dy1,
                ox + dx1, oy + dy1, width, gcolor );
        GRLine( &panel->m_ClipBox, DC, ox - dx2, oy - dy2,
                ox + dx2, oy + dy2, width, gcolor );
        break;

    case SKETCH:
        GRCSegm( &panel->m_ClipBox, DC, ox - dx1, oy - dy1,
                 ox + dx1, oy + dy1,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC, ox - dx2, oy - dy2,
                 ox + dx2, oy + dy2,
                 width, gcolor );
        break;
    }
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool MIREPCB::HitTest( const wxPoint& refPos )
{
    int dX    = refPos.x - m_Pos.x;
    int dY    = refPos.y - m_Pos.y;
    int rayon = m_Size / 2;
    return abs( dX ) <= rayon && abs( dY ) <= rayon;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool    MIREPCB::HitTest( EDA_Rect& refArea )
{
    if( refArea.Contains( m_Pos ) )
        return true;
    return false;
}


/**
 * Function Rotate
 * Rotate this object.
 * @param aRotCentre - the rotation point.
 * @param aAngle - the rotation angle in 0.1 degree.
 */
void MIREPCB::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param aCentre - the rotation point.
 */
void MIREPCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}
