/*****************************/
/* DIMENSION class definition */
/*****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"
#include "wxstruct.h"
#include "class_board_design_settings.h"
#include "class_drawpanel.h"
#include "colors_selection.h"
#include "kicad_string.h"
#include "protos.h"

DIMENSION::DIMENSION( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, TYPE_DIMENSION )
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


/* Setup the dimension text */
void DIMENSION::SetText( const wxString& NewText )
{
    m_Text->m_Text = NewText;
}


/* Return the dimension text
*/
wxString DIMENSION::GetText( void )
{
    return m_Text->m_Text;
}

/**
 * Function SetLayer
 * sets the layer this item is on.
 * @param aLayer The layer number.
 */
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

    Barre_ox    = source->Barre_ox;
    Barre_oy    = source->Barre_oy;
    Barre_fx    = source->Barre_fx;
    Barre_fy    = source->Barre_fy;
    TraitG_ox   = source->TraitG_ox;
    TraitG_oy   = source->TraitG_oy;
    TraitG_fx   = source->TraitG_fx;
    TraitG_fy   = source->TraitG_fy;
    TraitD_ox   = source->TraitD_ox;
    TraitD_oy   = source->TraitD_oy;
    TraitD_fx   = source->TraitD_fx;
    TraitD_fy   = source->TraitD_fy;
    FlecheD1_ox = source->FlecheD1_ox;
    FlecheD1_oy = source->FlecheD1_oy;
    FlecheD1_fx = source->FlecheD1_fx;
    FlecheD1_fy = source->FlecheD1_fy;
    FlecheD2_ox = source->FlecheD2_ox;
    FlecheD2_oy = source->FlecheD2_oy;
    FlecheD2_fx = source->FlecheD2_fx;
    FlecheD2_fy = source->FlecheD2_fy;
    FlecheG1_ox = source->FlecheG1_ox;
    FlecheG1_oy = source->FlecheG1_oy;
    FlecheG1_fx = source->FlecheG1_fx;
    FlecheG1_fy = source->FlecheG1_fy;
    FlecheG2_ox = source->FlecheG2_ox;
    FlecheG2_oy = source->FlecheG2_oy;
    FlecheG2_fx = source->FlecheG2_fx;
    FlecheG2_fy = source->FlecheG2_fy;
}


bool DIMENSION::ReadDimensionDescr( FILE* File, int* LineNum )
{
    char Line[2048], Text[2048];

    while(  GetLine( File, Line, LineNum ) != NULL )
    {
        if( strnicmp( Line, "$EndDIMENSION", 4 ) == 0 )
            return TRUE;

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
            m_Text->m_Text = CONV_FROM_UTF8( Text );
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
                        &Barre_ox, &Barre_oy,
                        &Barre_fx, &Barre_fy,
                        &m_Width );
                break;

            case 'd':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &TraitD_ox, &TraitD_oy,
                        &TraitD_fx, &TraitD_fy,
                        &Dummy );
                break;

            case 'g':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &TraitG_ox, &TraitG_oy,
                        &TraitG_fx, &TraitG_fy,
                        &Dummy );
                break;

            case '1':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &FlecheD1_ox, &FlecheD1_oy,
                        &FlecheD1_fx, &FlecheD1_fy,
                        &Dummy );
                break;

            case '2':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &FlecheD2_ox, &FlecheD2_oy,
                        &FlecheD2_fx, &FlecheD2_fy,
                        &Dummy );
                break;

            case '3':
                sscanf( Line + 2, " %d %d %d %d %d %d\n",
                        &Dummy,
                        &FlecheG1_ox, &FlecheG1_oy,
                        &FlecheG1_fx, &FlecheG1_fy,
                        &Dummy );
                break;

            case '4':
                sscanf( Line + 2, " %d %d %d %d %d %d",
                        &Dummy,
                        &FlecheG2_ox, &FlecheG2_oy,
                        &FlecheG2_fx, &FlecheG2_fy,
                        &Dummy );
                break;
            }

            continue;
        }
    }

    return FALSE;
}


/**
 * Function Move
 * @param offset : moving vector
 */
void DIMENSION::Move(const wxPoint& offset)
{
    m_Pos += offset;
    m_Text->m_Pos += offset;
    Barre_ox    += offset.x;
    Barre_oy    += offset.y;
    Barre_fx    += offset.x;
    Barre_fy    += offset.y;
    TraitG_ox   += offset.x;
    TraitG_oy   += offset.y;
    TraitG_fx   += offset.x;
    TraitG_fy   += offset.y;
    TraitD_ox   += offset.x;
    TraitD_oy   += offset.y;
    TraitD_fx   += offset.x;
    TraitD_fy   += offset.y;
    FlecheG1_ox += offset.x;
    FlecheG1_oy += offset.y;
    FlecheG1_fx += offset.x;
    FlecheG1_fy += offset.y;
    FlecheG2_ox += offset.x;
    FlecheG2_oy += offset.y;
    FlecheG2_fx += offset.x;
    FlecheG2_fy += offset.y;
    FlecheD1_ox += offset.x;
    FlecheD1_oy += offset.y;
    FlecheD1_fx += offset.x;
    FlecheD1_fy += offset.y;
    FlecheD2_ox += offset.x;
    FlecheD2_oy += offset.y;
    FlecheD2_fx += offset.x;
    FlecheD2_fy += offset.y;
}


/**
 * Function Rotate
 * @param center : Rotation point
 * @param angle : Rotation angle in 0.1 degrees
 */
void DIMENSION::Rotate(const wxPoint& centre, int angle)
{
    RotatePoint( &m_Pos, centre, angle );

    RotatePoint( &m_Text->m_Pos, centre, angle );
    m_Text->m_Orient += angle;
    if( m_Text->m_Orient >= 3600 )
        m_Text->m_Orient -= 3600;
    if( ( m_Text->m_Orient > 900 ) && ( m_Text->m_Orient <2700 ) )
        m_Text->m_Orient -= 1800;

    RotatePoint( &Barre_ox, &Barre_oy, centre.x, centre.y, angle );
    RotatePoint( &Barre_fx, &Barre_fy, centre.x, centre.y, angle );
    RotatePoint( &TraitG_ox, &TraitG_oy, centre.x, centre.y, angle );
    RotatePoint( &TraitG_fx, &TraitG_fy, centre.x, centre.y, angle );
    RotatePoint( &TraitD_ox, &TraitD_oy, centre.x, centre.y, angle );
    RotatePoint( &TraitD_fx, &TraitD_fy, centre.x, centre.y, angle );
    RotatePoint( &FlecheG1_ox, &FlecheG1_oy, centre.x, centre.y, angle );
    RotatePoint( &FlecheG1_fx, &FlecheG1_fy, centre.x, centre.y, angle );
    RotatePoint( &FlecheG2_ox, &FlecheG2_oy, centre.x, centre.y, angle );
    RotatePoint( &FlecheG2_fx, &FlecheG2_fy, centre.x, centre.y, angle );
    RotatePoint( &FlecheD1_ox, &FlecheD1_oy, centre.x, centre.y, angle );
    RotatePoint( &FlecheD1_fx, &FlecheD1_fy, centre.x, centre.y, angle );
    RotatePoint( &FlecheD2_ox, &FlecheD2_oy, centre.x, centre.y, angle );
    RotatePoint( &FlecheD2_fx, &FlecheD2_fy, centre.x, centre.y, angle );
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param const wxPoint& aCentre - the rotation point.
 */
void DIMENSION::Flip(const wxPoint& aCentre )
{
    Mirror( aCentre );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


/**
 * Function Mirror
 * Mirror the Dimension , relative to a given horizontal axis
 * the text is not mirrored. only its position (and angle) is mirrored
 * the layer is not changed
 * @param axis_pos : vertical axis position
 */
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

    INVERT( Barre_oy );
    INVERT( Barre_fy );
    INVERT( TraitG_oy );
    INVERT( TraitG_fy );
    INVERT( TraitD_oy );
    INVERT( TraitD_fy );
    INVERT( FlecheG1_oy );
    INVERT( FlecheG1_fy );
    INVERT( FlecheG2_oy );
    INVERT( FlecheG2_fy );
    INVERT( FlecheD1_oy );
    INVERT( FlecheD1_fy );
    INVERT( FlecheD2_oy );
    INVERT( FlecheD2_fy );
}


bool DIMENSION::Save( FILE* aFile ) const
{
    if( GetState( DELETED ) )
        return true;

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
        fprintf( aFile, "Te \"%s\"\n", CONV_TO_UTF8( m_Text->m_Text ) );
    else
        fprintf( aFile, "Te \"?\"\n" );

    fprintf( aFile, "Po %d %d %d %d %d %d %d\n",
             m_Text->m_Pos.x, m_Text->m_Pos.y,
             m_Text->m_Size.x, m_Text->m_Size.y,
             m_Text->m_Thickness, m_Text->m_Orient,
             m_Text->m_Mirror ? 0 : 1 );

    fprintf( aFile, "Sb %d %d %d %d %d %d\n", S_SEGMENT,
             Barre_ox, Barre_oy,
             Barre_fx, Barre_fy, m_Width );

    fprintf( aFile, "Sd %d %d %d %d %d %d\n", S_SEGMENT,
             TraitD_ox, TraitD_oy,
             TraitD_fx, TraitD_fy, m_Width );

    fprintf( aFile, "Sg %d %d %d %d %d %d\n", S_SEGMENT,
             TraitG_ox, TraitG_oy,
             TraitG_fx, TraitG_fy, m_Width );

    fprintf( aFile, "S1 %d %d %d %d %d %d\n", S_SEGMENT,
             FlecheD1_ox, FlecheD1_oy,
             FlecheD1_fx, FlecheD1_fy, m_Width );

    fprintf( aFile, "S2 %d %d %d %d %d %d\n", S_SEGMENT,
             FlecheD2_ox, FlecheD2_oy,
             FlecheD2_fx, FlecheD2_fy, m_Width );


    fprintf( aFile, "S3 %d %d %d %d %d %d\n", S_SEGMENT,
             FlecheG1_ox, FlecheG1_oy,
             FlecheG1_fx, FlecheG1_fy, m_Width );

    fprintf( aFile, "S4 %d %d %d %d %d %d\n", S_SEGMENT,
             FlecheG2_ox, FlecheG2_oy,
             FlecheG2_fx, FlecheG2_fy, m_Width );

    if( fputs( keyWordLineEnd, aFile ) == EOF )
        goto out;

    rc = true;

out:
    return rc;
}


/**
 * Function AdjustDimensionDetails
 * Calculate coordinates of segments used to draw the dimension.
 * @param aDoNotChangeText (bool) if false, the dimension text is initialized
 */
void DIMENSION::AdjustDimensionDetails( bool aDoNotChangeText )
{
    #define ARROW_SIZE 500    //size of arrows
    int      ii;
    int      mesure, deltax, deltay;            /* valeur de la mesure sur les axes X et Y */
    int      fleche_up_X = 0, fleche_up_Y = 0;  /* coord des fleches : barre / */
    int      fleche_dw_X = 0, fleche_dw_Y = 0;  /* coord des fleches : barre \ */
    int      hx, hy;                            /* coord des traits de rappel de cote */
    float    angle, angle_f;
    wxString msg;

    /* Init layer : */
    m_Text->SetLayer( GetLayer() );

    /* calculate the size of the cdimension
     * (text + line above the text) */
    ii = m_Text->m_Size.y +
         m_Text->m_Thickness + (m_Width * 3);

    deltax = TraitD_ox - TraitG_ox;
    deltay = TraitD_oy - TraitG_oy;

    /* Calculate dimension value */
    mesure = wxRound(hypot( (double) deltax, (double) deltay ) );

    if( deltax || deltay )
        angle = atan2( (double) deltay, (double) deltax );
    else
        angle = 0.0;

    /* Calcul des parametre dimensions X et Y des fleches et traits de cotes */
    hx = hy = ii;

    /* On tient compte de l'inclinaison de la cote */
    if( mesure )
    {
        hx = (abs) ( (int) ( ( (double) deltay * hx ) / mesure ) );
        hy = (abs) ( (int) ( ( (double) deltax * hy ) / mesure ) );

        if( TraitG_ox > Barre_ox )
            hx = -hx;
        if( TraitG_ox == Barre_ox )
            hx = 0;
        if( TraitG_oy > Barre_oy )
            hy = -hy;
        if( TraitG_oy == Barre_oy )
            hy = 0;

        angle_f     = angle + (M_PI * 27.5 / 180);
        fleche_up_X = (int) ( ARROW_SIZE * cos( angle_f ) );
        fleche_up_Y = (int) ( ARROW_SIZE * sin( angle_f ) );
        angle_f     = angle - (M_PI * 27.5 / 180);
        fleche_dw_X = (int) ( ARROW_SIZE * cos( angle_f ) );
        fleche_dw_Y = (int) ( ARROW_SIZE * sin( angle_f ) );
    }


    FlecheG1_ox = Barre_ox;
    FlecheG1_oy = Barre_oy;
    FlecheG1_fx = Barre_ox + fleche_up_X;
    FlecheG1_fy = Barre_oy + fleche_up_Y;

    FlecheG2_ox = Barre_ox;
    FlecheG2_oy = Barre_oy;
    FlecheG2_fx = Barre_ox + fleche_dw_X;
    FlecheG2_fy = Barre_oy + fleche_dw_Y;

    /*la fleche de droite est symetrique a celle de gauche:
     *  / = -\  et  \ = -/
     */
    FlecheD1_ox = Barre_fx;
    FlecheD1_oy = Barre_fy;
    FlecheD1_fx = Barre_fx - fleche_dw_X;
    FlecheD1_fy = Barre_fy - fleche_dw_Y;

    FlecheD2_ox = Barre_fx;
    FlecheD2_oy = Barre_fy;
    FlecheD2_fx = Barre_fx - fleche_up_X;
    FlecheD2_fy = Barre_fy - fleche_up_Y;


    TraitG_fx = Barre_ox + hx;
    TraitG_fy = Barre_oy + hy;

    TraitD_fx = Barre_fx + hx;
    TraitD_fy = Barre_fy + hy;

    /* Calculate the better text position and orientation: */
    m_Pos.x   = m_Text->m_Pos.x
                          = (Barre_fx + TraitG_fx) / 2;
    m_Pos.y   = m_Text->m_Pos.y
                          = (Barre_fy + TraitG_fy) / 2;

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


/* Print 1 dimension: segments and text
 */
void DIMENSION::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                     int mode_color, const wxPoint& offset )
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

#ifdef USE_WX_ZOOM
    if( DC->LogicalToDeviceXRel( width ) < 2 )
#else
    if( panel->GetScreen()->Scale( width ) < 2 )
#endif
        typeaff = FILAIRE;

    switch( typeaff )
    {
    case FILAIRE:
        width = 0;

    case FILLED:
        GRLine( &panel->m_ClipBox, DC,
                Barre_ox - ox, Barre_oy - oy,
                Barre_fx - ox, Barre_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                TraitG_ox - ox, TraitG_oy - oy,
                TraitG_fx - ox, TraitG_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                TraitD_ox - ox, TraitD_oy - oy,
                TraitD_fx - ox, TraitD_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                FlecheD1_ox - ox, FlecheD1_oy - oy,
                FlecheD1_fx - ox, FlecheD1_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                FlecheD2_ox - ox, FlecheD2_oy - oy,
                FlecheD2_fx - ox, FlecheD2_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                FlecheG1_ox - ox, FlecheG1_oy - oy,
                FlecheG1_fx - ox, FlecheG1_fy - oy, width, gcolor );
        GRLine( &panel->m_ClipBox, DC,
                FlecheG2_ox - ox, FlecheG2_oy - oy,
                FlecheG2_fx - ox, FlecheG2_fy - oy, width, gcolor );
        break;

    case SKETCH:
        GRCSegm( &panel->m_ClipBox, DC,
                 Barre_ox - ox, Barre_oy - oy,
                 Barre_fx - ox, Barre_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 TraitG_ox - ox, TraitG_oy - oy,
                 TraitG_fx - ox, TraitG_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 TraitD_ox - ox, TraitD_oy - oy,
                 TraitD_fx - ox, TraitD_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 FlecheD1_ox - ox, FlecheD1_oy - oy,
                 FlecheD1_fx - ox, FlecheD1_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 FlecheD2_ox - ox, FlecheD2_oy - oy,
                 FlecheD2_fx - ox, FlecheD2_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 FlecheG1_ox - ox, FlecheG1_oy - oy,
                 FlecheG1_fx - ox, FlecheG1_fy - oy,
                 width, gcolor );
        GRCSegm( &panel->m_ClipBox, DC,
                 FlecheG2_ox - ox, FlecheG2_oy - oy,
                 FlecheG2_fx - ox, FlecheG2_fy - oy,
                 width, gcolor );
        break;
    }
}


// see class_cotation.h
void DIMENSION::DisplayInfo( WinEDA_DrawFrame* frame )
{
    // for now, display only the text within the DIMENSION using class TEXTE_PCB.
    m_Text->DisplayInfo( frame );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool DIMENSION::HitTest( const wxPoint& ref_pos )
{
    int             ux0, uy0;
    int             dx, dy, spot_cX, spot_cY;

    if( m_Text && m_Text->TextHitTest( ref_pos ) )
            return true;

    /* Locate SEGMENTS? */
    ux0 = Barre_ox;
    uy0 = Barre_oy;

    /* Recalculate coordinates with ux0, uy0 = origin. */
    dx = Barre_fx - ux0;
    dy = Barre_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = TraitG_ox;
    uy0 = TraitG_oy;

    dx = TraitG_fx - ux0;
    dy = TraitG_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = TraitD_ox;
    uy0 = TraitD_oy;

    dx = TraitD_fx - ux0;
    dy = TraitD_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = FlecheD1_ox;
    uy0 = FlecheD1_oy;

    dx = FlecheD1_fx - ux0;
    dy = FlecheD1_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = FlecheD2_ox;
    uy0 = FlecheD2_oy;

    dx = FlecheD2_fx - ux0;
    dy = FlecheD2_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = FlecheG1_ox;
    uy0 = FlecheG1_oy;

    dx = FlecheG1_fx - ux0;
    dy = FlecheG1_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    ux0 = FlecheG2_ox;
    uy0 = FlecheG2_oy;

    dx = FlecheG2_fx - ux0;
    dy = FlecheG2_fy - uy0;

    spot_cX = ref_pos.x - ux0;
    spot_cY = ref_pos.y - uy0;

    if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
        return true;

    return false;
}


/**
 * Function HitTest (overlaid)
 * tests if the given EDA_Rect intersect this object.
 * @param EDA_Rect : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool DIMENSION::HitTest( EDA_Rect& refArea )
{
    if( refArea.Inside( m_Pos ) )
        return true;
    return false;
}
