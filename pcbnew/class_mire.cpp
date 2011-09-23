/**
 * @file class_mire.cpp
 * MIRE class definition (targets for photo plots)
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "pcbcommon.h"
#include "colors_selection.h"
#include "trigo.h"
#include "protos.h"
#include "richio.h"

#include "class_board.h"
#include "class_mire.h"


PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_Shape = 0;
    m_Size  = 5000;
}


PCB_TARGET::~PCB_TARGET()
{
}


void PCB_TARGET::Copy( PCB_TARGET* source )
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
bool PCB_TARGET::ReadMirePcbDescr( LINE_READER* aReader )
{
    char* Line;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$End", 4 ) == 0 )
            return true;

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

    return false;
}


bool PCB_TARGET::Save( FILE* aFile ) const
{
    bool rc = false;

    if( fprintf( aFile, "$PCB_TARGET\n" ) != sizeof("$PCB_TARGET\n")-1 )
        goto out;

    fprintf( aFile, "Po %X %d %d %d %d %d %8.8lX\n",
             m_Shape, m_Layer,
             m_Pos.x, m_Pos.y,
             m_Size, m_Width, m_TimeStamp );

    if( fprintf( aFile, "$EndPCB_TARGET\n" ) != sizeof("$EndPCB_TARGET\n")-1 )
        goto out;

    rc = true;

out:
    return rc;
}




/* Draw PCB_TARGET object: 2 segments + 1 circle
 * The circle radius is half the radius of the target
 * 2 lines have length the diameter of the target
 */
void PCB_TARGET::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int mode_color, const wxPoint& offset )
{
    int radius, ox, oy, gcolor, width;
    int dx1, dx2, dy1, dy2;
    int typeaff;

    ox = m_Pos.x + offset.x;
    oy = m_Pos.y + offset.y;

    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    gcolor = brd->GetLayerColor( m_Layer );

    GRSetDrawMode( DC, mode_color );
    typeaff = DisplayOpt.DisplayDrawItems;
    width   = m_Width;

    if( DC->LogicalToDeviceXRel( width ) < 2 )
        typeaff = FILAIRE;

    radius = m_Size / 4;

    switch( typeaff )
    {
    case FILAIRE:
        width = 0;

    case FILLED:
        GRCircle( &panel->m_ClipBox, DC, ox, oy, radius, width, gcolor );
        break;

    case SKETCH:
        GRCircle( &panel->m_ClipBox, DC, ox, oy, radius + (width / 2), gcolor );
        GRCircle( &panel->m_ClipBox, DC, ox, oy, radius - (width / 2), gcolor );
        break;
    }


    radius = m_Size / 2;
    dx1   = radius;
    dy1   = 0;
    dx2   = 0;
    dy2   = radius;

    if( m_Shape ) /* Form X */
    {
        dx1 = dy1 = ( radius * 7 ) / 5;
        dx2 = dx1;
        dy2 = -dy1;
    }

    switch( typeaff )
    {
    case FILAIRE:
    case FILLED:
        GRLine( &panel->m_ClipBox, DC, ox - dx1, oy - dy1, ox + dx1, oy + dy1, width, gcolor );
        GRLine( &panel->m_ClipBox, DC, ox - dx2, oy - dy2, ox + dx2, oy + dy2, width, gcolor );
        break;

    case SKETCH:
        GRCSegm( &panel->m_ClipBox, DC, ox - dx1, oy - dy1, ox + dx1, oy + dy1, width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC, ox - dx2, oy - dy2, ox + dx2, oy + dy2, width, gcolor );
        break;
    }
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool PCB_TARGET::HitTest( const wxPoint& refPos )
{
    int dX    = refPos.x - m_Pos.x;
    int dY    = refPos.y - m_Pos.y;
    int radius = m_Size / 2;
    return abs( dX ) <= radius && abs( dY ) <= radius;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_RECT intersect this object.
 * @param refArea : the given EDA_RECT
 * @return bool - true if a hit, else false
 */
bool PCB_TARGET::HitTest( EDA_RECT& refArea )
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
void PCB_TARGET::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param aCentre - the rotation point.
 */
void PCB_TARGET::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


EDA_RECT PCB_TARGET::GetBoundingBox() const
{
    EDA_RECT bBox;
    bBox.SetX( m_Pos.x - m_Size/2 );
    bBox.SetY( m_Pos.y - m_Size/2 );
    bBox.SetWidth( m_Size );
    bBox.SetHeight( m_Size );

    return bBox;
}


wxString PCB_TARGET::GetSelectMenuText() const
{
    wxString text;
    wxString msg;

    valeur_param( m_Size, msg );

    text << _( "Target" ) << _( " on " ) << GetLayerName() << wxT( " " ) << _( "size" )
         << wxT( " " ) << msg;

    return text;
}
