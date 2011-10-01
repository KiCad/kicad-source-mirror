/**
 * @file class_dimension.cpp
 */

#include "fctsys.h"
#include "macros.h"
#include "gr_basic.h"
#include "pcbcommon.h"
#include "trigo.h"
#include "wxstruct.h"
#include "class_drawpanel.h"
#include "colors_selection.h"
#include "kicad_string.h"
#include "protos.h"
#include "richio.h"

#include "class_board.h"
#include "class_pcb_text.h"
#include "class_dimension.h"


DIMENSION::DIMENSION( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_DIMENSION_T )
{
    m_Layer = DRAW_LAYER;
    m_Width = 50;
    m_Value = 0;
    m_Shape = 0;
    m_Unit  = INCHES;
    m_Text  = new TEXTE_PCB( this );
}


DIMENSION::~DIMENSION()
{
    delete m_Text;
}


void DIMENSION::SetText( const wxString& NewText )
{
    m_Text->m_Text = NewText;
}


wxString DIMENSION::GetText( void ) const
{
    return m_Text->m_Text;
}


void  DIMENSION::SetLayer( int aLayer )
{
    m_Layer = aLayer;
    m_Text->SetLayer( aLayer);
}


void DIMENSION::Copy( DIMENSION* source )
{
    m_Value     = source->m_Value;
    SetLayer( source->GetLayer() );
    m_Width     = source->m_Width;
    m_Pos       = source->m_Pos;
    m_Shape     = source->m_Shape;
    m_Unit      = source->m_Unit;
    m_TimeStamp = GetTimeStamp();
    m_Text->Copy( source->m_Text );

    m_crossBarOx    = source->m_crossBarOx;
    m_crossBarOy    = source->m_crossBarOy;
    m_crossBarFx    = source->m_crossBarFx;
    m_crossBarFy    = source->m_crossBarFy;
    m_featureLineGOx   = source->m_featureLineGOx;
    m_featureLineGOy   = source->m_featureLineGOy;
    m_featureLineGFx   = source->m_featureLineGFx;
    m_featureLineGFy   = source->m_featureLineGFy;
    m_featureLineDOx   = source->m_featureLineDOx;
    m_featureLineDOy   = source->m_featureLineDOy;
    m_featureLineDFx   = source->m_featureLineDFx;
    m_featureLineDFy   = source->m_featureLineDFy;
    m_arrowD1Ox = source->m_arrowD1Ox;
    m_arrowD1Oy = source->m_arrowD1Oy;
    m_arrowD1Fx = source->m_arrowD1Fx;
    m_arrowD1Fy = source->m_arrowD1Fy;
    m_arrowD2Ox = source->m_arrowD2Ox;
    m_arrowD2Oy = source->m_arrowD2Oy;
    m_arrowD2Fx = source->m_arrowD2Fx;
    m_arrowD2Fy = source->m_arrowD2Fy;
    m_arrowG1Ox = source->m_arrowG1Ox;
    m_arrowG1Oy = source->m_arrowG1Oy;
    m_arrowG1Fx = source->m_arrowG1Fx;
    m_arrowG1Fy = source->m_arrowG1Fy;
    m_arrowG2Ox = source->m_arrowG2Ox;
    m_arrowG2Oy = source->m_arrowG2Oy;
    m_arrowG2Fx = source->m_arrowG2Fx;
    m_arrowG2Fy = source->m_arrowG2Fy;
}


bool DIMENSION::ReadDimensionDescr( LINE_READER* aReader )
{
    char* Line;
    char  Text[2048];

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "$EndDIMENSION", 4 ) == 0 )
            return true;

        if( Line[0] == 'V' )
        {
            sscanf( Line + 2, " %d", &m_Value );
            continue;
        }

        if( Line[0] == 'G' )
        {
            int layer;

            sscanf( Line + 2, " %d %d %lX", &m_Shape, &layer, &m_TimeStamp );

            if( layer < FIRST_NO_COPPER_LAYER )
                layer = FIRST_NO_COPPER_LAYER;

            if( layer > LAST_NO_COPPER_LAYER )
                layer = LAST_NO_COPPER_LAYER;

            SetLayer( layer );
            m_Text->SetLayer( layer );
            continue;
        }

        if( Line[0] == 'T' )
        {
            ReadDelimitedText( Text, Line + 2, sizeof(Text) );
            m_Text->m_Text = FROM_UTF8( Text );
            continue;
        }

        if( Line[0] == 'P' )
        {
            int normal_display = 1;
            sscanf( Line + 2, " %d %d %d %d %d %d %d",
                    &m_Text->m_Pos.x, &m_Text->m_Pos.y,
                    &m_Text->m_Size.x, &m_Text->m_Size.y,
                    &m_Text->m_Thickness, &m_Text->m_Orient,
                    &normal_display );

            m_Text->m_Mirror = normal_display ? false : true;
            m_Pos = m_Text->m_Pos;
            continue;
        }

        if( Line[0] == 'S' )
        {
            switch( Line[1] )
            {
                int Dummy;

            case 'b':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_crossBarOx, &m_crossBarOy,
                        &m_crossBarFx, &m_crossBarFy,
                        &m_Width );
                break;

            case 'd':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_featureLineDOx, &m_featureLineDOy,
                        &m_featureLineDFx, &m_featureLineDFy,
                        &Dummy );
                break;

            case 'g':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_featureLineGOx, &m_featureLineGOy,
                        &m_featureLineGFx, &m_featureLineGFy,
                        &Dummy );
                break;

            case '1':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowD1Ox, &m_arrowD1Oy,
                        &m_arrowD1Fx, &m_arrowD1Fy,
                        &Dummy );
                break;

            case '2':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowD2Ox, &m_arrowD2Oy,
                        &m_arrowD2Fx, &m_arrowD2Fy,
                        &Dummy );
                break;

            case '3':
                sscanf( Line + 2, " %d %d %d %d %d %d\n",
                        &Dummy,
                        &m_arrowG1Ox, &m_arrowG1Oy,
                        &m_arrowG1Fx, &m_arrowG1Fy,
                        &Dummy );
                break;

            case '4':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &m_arrowG2Ox, &m_arrowG2Oy,
                        &m_arrowG2Fx, &m_arrowG2Fy,
                        &Dummy );
                break;
            }

            continue;
        }
    }

    return false;
}


void DIMENSION::Move(const wxPoint& offset)
{
    m_Pos += offset;
    m_Text->m_Pos += offset;
    m_crossBarOx    += offset.x;
    m_crossBarOy    += offset.y;
    m_crossBarFx    += offset.x;
    m_crossBarFy    += offset.y;
    m_featureLineGOx   += offset.x;
    m_featureLineGOy   += offset.y;
    m_featureLineGFx   += offset.x;
    m_featureLineGFy   += offset.y;
    m_featureLineDOx   += offset.x;
    m_featureLineDOy   += offset.y;
    m_featureLineDFx   += offset.x;
    m_featureLineDFy   += offset.y;
    m_arrowG1Ox += offset.x;
    m_arrowG1Oy += offset.y;
    m_arrowG1Fx += offset.x;
    m_arrowG1Fy += offset.y;
    m_arrowG2Ox += offset.x;
    m_arrowG2Oy += offset.y;
    m_arrowG2Fx += offset.x;
    m_arrowG2Fy += offset.y;
    m_arrowD1Ox += offset.x;
    m_arrowD1Oy += offset.y;
    m_arrowD1Fx += offset.x;
    m_arrowD1Fy += offset.y;
    m_arrowD2Ox += offset.x;
    m_arrowD2Oy += offset.y;
    m_arrowD2Fx += offset.x;
    m_arrowD2Fy += offset.y;
}


void DIMENSION::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );

    RotatePoint( &m_Text->m_Pos, aRotCentre, aAngle );
    m_Text->m_Orient += aAngle;

    if( m_Text->m_Orient >= 3600 )
        m_Text->m_Orient -= 3600;

    if( ( m_Text->m_Orient > 900 ) && ( m_Text->m_Orient <2700 ) )
        m_Text->m_Orient -= 1800;

    RotatePoint( &m_crossBarOx, &m_crossBarOy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_crossBarFx, &m_crossBarFy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_featureLineGOx, &m_featureLineGOy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_featureLineGFx, &m_featureLineGFy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_featureLineDOx, &m_featureLineDOy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_featureLineDFx, &m_featureLineDFy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowG1Ox, &m_arrowG1Oy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowG1Fx, &m_arrowG1Fy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowG2Ox, &m_arrowG2Oy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowG2Fx, &m_arrowG2Fy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowD1Ox, &m_arrowD1Oy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowD1Fx, &m_arrowD1Fy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowD2Ox, &m_arrowD2Oy, aRotCentre.x, aRotCentre.y, aAngle );
    RotatePoint( &m_arrowD2Fx, &m_arrowD2Fy, aRotCentre.x, aRotCentre.y, aAngle );
}


void DIMENSION::Flip(const wxPoint& aCentre )
{
    Mirror( aCentre );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


void DIMENSION::Mirror(const wxPoint& axis_pos)
{
#define INVERT( pos )       (pos) = axis_pos.y - ( (pos) - axis_pos.y )
#define INVERT_ANGLE( phi ) (phi) = -(phi)
    INVERT( m_Pos.y );
    INVERT( m_Text->m_Pos.y );
    INVERT_ANGLE( m_Text->m_Orient );

    if( m_Text->m_Orient >= 3600 )
        m_Text->m_Orient -= 3600;

    if( ( m_Text->m_Orient > 900 ) && ( m_Text->m_Orient < 2700 ) )
        m_Text->m_Orient -= 1800;

    INVERT( m_crossBarOy );
    INVERT( m_crossBarFy );
    INVERT( m_featureLineGOy );
    INVERT( m_featureLineGFy );
    INVERT( m_featureLineDOy );
    INVERT( m_featureLineDFy );
    INVERT( m_arrowG1Oy );
    INVERT( m_arrowG1Fy );
    INVERT( m_arrowG2Oy );
    INVERT( m_arrowG2Fy );
    INVERT( m_arrowD1Oy );
    INVERT( m_arrowD1Fy );
    INVERT( m_arrowD2Oy );
    INVERT( m_arrowD2Fy );
}


bool DIMENSION::Save( FILE* aFile ) const
{
    bool rc = false;

    // note: COTATION was the previous name of DIMENSION
    // this old keyword is used here for compatibility
    const char keyWordLine[] = "$COTATION\n";
    const char keyWordLineEnd[] = "$endCOTATION\n";

    if( fputs( keyWordLine, aFile ) == EOF )
        goto out;

    fprintf( aFile, "Ge %d %d %lX\n", m_Shape, m_Layer, m_TimeStamp );

    fprintf( aFile, "Va %d\n", m_Value );

    if( !m_Text->m_Text.IsEmpty() )
        fprintf( aFile, "Te %s\n", EscapedUTF8( m_Text->m_Text ).c_str() );
    else
        fprintf( aFile, "Te \"?\"\n" );

    fprintf( aFile, "Po %d %d %d %d %d %d %d\n",
             m_Text->m_Pos.x, m_Text->m_Pos.y,
             m_Text->m_Size.x, m_Text->m_Size.y,
             m_Text->m_Thickness, m_Text->m_Orient,
             m_Text->m_Mirror ? 0 : 1 );

    fprintf( aFile, "Sb %d %d %d %d %d %d\n", S_SEGMENT,
             m_crossBarOx, m_crossBarOy,
             m_crossBarFx, m_crossBarFy, m_Width );

    fprintf( aFile, "Sd %d %d %d %d %d %d\n", S_SEGMENT,
             m_featureLineDOx, m_featureLineDOy,
             m_featureLineDFx, m_featureLineDFy, m_Width );

    fprintf( aFile, "Sg %d %d %d %d %d %d\n", S_SEGMENT,
             m_featureLineGOx, m_featureLineGOy,
             m_featureLineGFx, m_featureLineGFy, m_Width );

    fprintf( aFile, "S1 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowD1Ox, m_arrowD1Oy,
             m_arrowD1Fx, m_arrowD1Fy, m_Width );

    fprintf( aFile, "S2 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowD2Ox, m_arrowD2Oy,
             m_arrowD2Fx, m_arrowD2Fy, m_Width );


    fprintf( aFile, "S3 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowG1Ox, m_arrowG1Oy,
             m_arrowG1Fx, m_arrowG1Fy, m_Width );

    fprintf( aFile, "S4 %d %d %d %d %d %d\n", S_SEGMENT,
             m_arrowG2Ox, m_arrowG2Oy,
             m_arrowG2Fx, m_arrowG2Fy, m_Width );

    if( fputs( keyWordLineEnd, aFile ) == EOF )
        goto out;

    rc = true;

out:
    return rc;
}


void DIMENSION::AdjustDimensionDetails( bool aDoNotChangeText )
{
    #define ARROW_SIZE 500    //size of arrows
    int      ii;
    int      mesure, deltax, deltay;            /* value of the measure on X and Y axes */
    int      arrow_up_X = 0, arrow_up_Y = 0;    /* coordinates of arrow line / */
    int      arrow_dw_X = 0, arrow_dw_Y = 0;    /* coordinates of arrow line \ */
    int      hx, hy;                            /* dimension line interval */
    float    angle, angle_f;
    wxString msg;

    /* Init layer : */
    m_Text->SetLayer( GetLayer() );

    /* calculate the size of the dimension (text + line above the text) */
    ii = m_Text->m_Size.y +
         m_Text->m_Thickness + (m_Width * 3);

    deltax = m_featureLineDOx - m_featureLineGOx;
    deltay = m_featureLineDOy - m_featureLineGOy;

    /* Calculate dimension value */
    mesure = wxRound(hypot( (double) deltax, (double) deltay ) );

    if( deltax || deltay )
        angle = atan2( (double) deltay, (double) deltax );
    else
        angle = 0.0;

    /* Calculation of parameters X and Y dimensions of the arrows and lines. */
    hx = hy = ii;

    /* Taking into account the slope of the side lines. */
    if( mesure )
    {
        hx = (abs) ( (int) ( ( (double) deltay * hx ) / mesure ) );
        hy = (abs) ( (int) ( ( (double) deltax * hy ) / mesure ) );

        if( m_featureLineGOx > m_crossBarOx )
            hx = -hx;

        if( m_featureLineGOx == m_crossBarOx )
            hx = 0;

        if( m_featureLineGOy > m_crossBarOy )
            hy = -hy;

        if( m_featureLineGOy == m_crossBarOy )
            hy = 0;

        angle_f     = angle + (M_PI * 27.5 / 180);
        arrow_up_X = (int) ( ARROW_SIZE * cos( angle_f ) );
        arrow_up_Y = (int) ( ARROW_SIZE * sin( angle_f ) );
        angle_f     = angle - (M_PI * 27.5 / 180);
        arrow_dw_X = (int) ( ARROW_SIZE * cos( angle_f ) );
        arrow_dw_Y = (int) ( ARROW_SIZE * sin( angle_f ) );
    }


    m_arrowG1Ox = m_crossBarOx;
    m_arrowG1Oy = m_crossBarOy;
    m_arrowG1Fx = m_crossBarOx + arrow_up_X;
    m_arrowG1Fy = m_crossBarOy + arrow_up_Y;

    m_arrowG2Ox = m_crossBarOx;
    m_arrowG2Oy = m_crossBarOy;
    m_arrowG2Fx = m_crossBarOx + arrow_dw_X;
    m_arrowG2Fy = m_crossBarOy + arrow_dw_Y;

    /* The right arrow is symmetrical to the left.
     *  / = -\  and  \ = -/
     */
    m_arrowD1Ox = m_crossBarFx;
    m_arrowD1Oy = m_crossBarFy;
    m_arrowD1Fx = m_crossBarFx - arrow_dw_X;
    m_arrowD1Fy = m_crossBarFy - arrow_dw_Y;

    m_arrowD2Ox = m_crossBarFx;
    m_arrowD2Oy = m_crossBarFy;
    m_arrowD2Fx = m_crossBarFx - arrow_up_X;
    m_arrowD2Fy = m_crossBarFy - arrow_up_Y;


    m_featureLineGFx = m_crossBarOx + hx;
    m_featureLineGFy = m_crossBarOy + hy;

    m_featureLineDFx = m_crossBarFx + hx;
    m_featureLineDFy = m_crossBarFy + hy;

    /* Calculate the better text position and orientation: */
    m_Pos.x = m_Text->m_Pos.x = (m_crossBarFx + m_featureLineGFx) / 2;
    m_Pos.y = m_Text->m_Pos.y = (m_crossBarFy + m_featureLineGFy) / 2;

    m_Text->m_Orient = -(int) (angle * 1800 / M_PI);

    if( m_Text->m_Orient < 0 )
        m_Text->m_Orient += 3600;

    if( m_Text->m_Orient >= 3600 )
        m_Text->m_Orient -= 3600;

    if( (m_Text->m_Orient > 900) && (m_Text->m_Orient <2700) )
        m_Text->m_Orient -= 1800;

    if( !aDoNotChangeText )
    {
        m_Value = mesure;
        valeur_param( m_Value, msg );
        SetText( msg );
    }
}


void DIMENSION::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int mode_color, const wxPoint& offset )
{
    int ox, oy, typeaff, width, gcolor;

    ox = -offset.x;
    oy = -offset.y;

    m_Text->Draw( panel, DC, mode_color, offset );

    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    gcolor = brd->GetLayerColor(m_Layer);

    GRSetDrawMode( DC, mode_color );
    typeaff = DisplayOpt.DisplayDrawItems;
    width   = m_Width;

    if( DC->LogicalToDeviceXRel( width ) < 2 )
        typeaff = FILAIRE;

    switch( typeaff )
    {
    case FILAIRE:
        width = 0;

    case FILLED:
        GRLine( &panel->m_ClipBox, DC,
                m_crossBarOx - ox, m_crossBarOy - oy,
                m_crossBarFx - ox, m_crossBarFy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_featureLineGOx - ox, m_featureLineGOy - oy,
                m_featureLineGFx - ox, m_featureLineGFy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_featureLineDOx - ox, m_featureLineDOy - oy,
                m_featureLineDFx - ox, m_featureLineDFy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_arrowD1Ox - ox, m_arrowD1Oy - oy,
                m_arrowD1Fx - ox, m_arrowD1Fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_arrowD2Ox - ox, m_arrowD2Oy - oy,
                m_arrowD2Fx - ox, m_arrowD2Fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_arrowG1Ox - ox, m_arrowG1Oy - oy,
                m_arrowG1Fx - ox, m_arrowG1Fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                m_arrowG2Ox - ox, m_arrowG2Oy - oy,
                m_arrowG2Fx - ox, m_arrowG2Fy - oy, width, gcolor );
        break;

    case SKETCH:
        GRCSegm( &panel->m_ClipBox, DC,
                 m_crossBarOx - ox, m_crossBarOy - oy,
                 m_crossBarFx - ox, m_crossBarFy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_featureLineGOx - ox, m_featureLineGOy - oy,
                 m_featureLineGFx - ox, m_featureLineGFy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_featureLineDOx - ox, m_featureLineDOy - oy,
                 m_featureLineDFx - ox, m_featureLineDFy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_arrowD1Ox - ox, m_arrowD1Oy - oy,
                 m_arrowD1Fx - ox, m_arrowD1Fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_arrowD2Ox - ox, m_arrowD2Oy - oy,
                 m_arrowD2Fx - ox, m_arrowD2Fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_arrowG1Ox - ox, m_arrowG1Oy - oy,
                 m_arrowG1Fx - ox, m_arrowG1Fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 m_arrowG2Ox - ox, m_arrowG2Oy - oy,
                 m_arrowG2Fx - ox, m_arrowG2Fy - oy,
                 width, gcolor );
        break;
    }
}


// see class_cotation.h
void DIMENSION::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    // for now, display only the text within the DIMENSION using class TEXTE_PCB.
    m_Text->DisplayInfo( frame );
}


bool DIMENSION::HitTest( const wxPoint& aPoint )
{
    int ux0, uy0;
    int dx, dy, spot_cX, spot_cY;

    if( m_Text && m_Text->TextHitTest( aPoint ) )
        return true;

    /* Locate SEGMENTS? */
    ux0 = m_crossBarOx;
    uy0 = m_crossBarOy;

    /* Recalculate coordinates with ux0, uy0 = origin. */
    dx = m_crossBarFx - ux0;
    dy = m_crossBarFy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_featureLineGOx;
    uy0 = m_featureLineGOy;

    dx = m_featureLineGFx - ux0;
    dy = m_featureLineGFy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_featureLineDOx;
    uy0 = m_featureLineDOy;

    dx = m_featureLineDFx - ux0;
    dy = m_featureLineDFy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_arrowD1Ox;
    uy0 = m_arrowD1Oy;

    dx = m_arrowD1Fx - ux0;
    dy = m_arrowD1Fy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_arrowD2Ox;
    uy0 = m_arrowD2Oy;

    dx = m_arrowD2Fx - ux0;
    dy = m_arrowD2Fy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_arrowG1Ox;
    uy0 = m_arrowG1Oy;

    dx = m_arrowG1Fx - ux0;
    dy = m_arrowG1Fy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = m_arrowG2Ox;
    uy0 = m_arrowG2Oy;

    dx = m_arrowG2Fx - ux0;
    dy = m_arrowG2Fy - uy0;

    spot_cX = aPoint.x - ux0;
    spot_cY = aPoint.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    return false;
}


bool DIMENSION::HitTest( EDA_RECT& refArea )
{
    if( refArea.Contains( m_Pos ) )
        return true;

    return false;
}


EDA_RECT DIMENSION::GetBoundingBox() const
{
    EDA_RECT bBox;
    int xmin, xmax, ymin, ymax;

    bBox = m_Text->GetTextBox( -1 );
    xmin = bBox.GetX();
    xmax = bBox.GetRight();
    ymin = bBox.GetY();
    ymax = bBox.GetBottom();

    xmin = MIN( xmin, m_crossBarOx );
    xmin = MIN( xmin, m_crossBarFx );
    ymin = MIN( ymin, m_crossBarOy );
    ymin = MIN( ymin, m_crossBarFy );
    xmax = MAX( xmax, m_crossBarOx );
    xmax = MAX( xmax, m_crossBarFx );
    ymax = MAX( ymax, m_crossBarOy );
    ymax = MAX( ymax, m_crossBarFy );

    xmin = MIN( xmin, m_featureLineGOx );
    xmin = MIN( xmin, m_featureLineGFx );
    ymin = MIN( ymin, m_featureLineGOy );
    ymin = MIN( ymin, m_featureLineGFy );
    xmax = MAX( xmax, m_featureLineGOx );
    xmax = MAX( xmax, m_featureLineGFx );
    ymax = MAX( ymax, m_featureLineGOy );
    ymax = MAX( ymax, m_featureLineGFy );

    bBox.SetX( xmin );
    bBox.SetY( ymin );
    bBox.SetWidth( xmax - xmin + 1 );
    bBox.SetHeight( ymax - ymin + 1 );

    bBox.Normalize();

    return bBox;
}


wxString DIMENSION::GetSelectMenuText() const
{
    wxString text;

    text << _( "Dimension" ) << wxT( " \"" ) << GetText() << wxT( "\"" );

    return text;
}
