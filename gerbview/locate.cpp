/************************************************/
/* Locate items at the current cursor position. */
/************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "gerbview.h"
#include "trigo.h"
#include "protos.h"


int ux0, uy0, dx, dy, spot_cX, spot_cY;


static TRACK*       Locate_Zone( TRACK* start_adresse, int layer, int typeloc );
static TRACK*       Locate_Zone( TRACK* start_adresse, wxPoint ref, int layer );
static TRACK*       Locate_Pistes( TRACK* start_adresse,
                                   int    Layer,
                                   int    typeloc );
static TRACK*       Locate_Pistes( TRACK*  start_adresse,
                                   wxPoint ref,
                                   int     Layer );
static DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int typeloc );
static TEXTE_PCB*   Locate_Texte_Pcb( TEXTE_PCB* pt_txt_pcb, int typeloc );
static int          distance( int seuil );


/* Macro for calculating the coordinates of the cursor position.
 */
#define SET_REF_POS( ref ) if( typeloc == CURSEUR_ON_GRILLE ) \
    { ref = ActiveScreen->m_Curseur; } \
    else { ref = ActiveScreen->m_MousePosition; }


/* Display the character of the localized STRUCTURE and return a pointer
 * to it.
 */
BOARD_ITEM* WinEDA_GerberFrame::Locate( int typeloc )
{
    TEXTE_PCB*   pt_texte_pcb;
    TRACK*       Track, * TrackLocate;
    DRAWSEGMENT* DrawSegm;
    int          layer;

    /* Locate tracks and vias, with priority to vias */
    layer = GetScreen()->m_Active_Layer;
    Track = Locate_Pistes( GetBoard()->m_Track, -1, typeloc );
    if( Track != NULL )
    {
        TrackLocate = Track;

        while( ( TrackLocate = Locate_Pistes( TrackLocate,
                                              layer, typeloc ) ) != NULL )
        {
            Track = TrackLocate;
            if( TrackLocate->Type() == TYPE_VIA )
                break;
            TrackLocate = TrackLocate->Next();
        }

        Track->DisplayInfo( this );
        return Track;
    }


    pt_texte_pcb = Locate_Texte_Pcb(
         (TEXTE_PCB*) GetBoard()->m_Drawings.GetFirst(), typeloc );

    if( pt_texte_pcb )
    {
        pt_texte_pcb->DisplayInfo( this );
        return pt_texte_pcb;
    }

    if( ( DrawSegm = Locate_Segment_Pcb( GetBoard(), typeloc ) ) != NULL )
    {
        return DrawSegm;
    }

    if( ( TrackLocate = Locate_Zone( GetBoard()->m_Zone,
                                     GetScreen()->m_Active_Layer,
                                     typeloc ) ) != NULL )
    {
        TrackLocate->DisplayInfo( this );
        return TrackLocate;
    }

    MsgPanel->EraseMsgBox();
    return NULL;
}


/* Locate of segments of pcb edge or draw as active layer.
 * Returns:
 *   Pointer to START segment if found
 *   NULL if nothing found
 */
DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int typeloc )
{
    BOARD_ITEM*  PtStruct;
    DRAWSEGMENT* pts;
    wxPoint      ref;
    PCB_SCREEN*  screen = (PCB_SCREEN*) ActiveScreen;

    SET_REF_POS( ref );

    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_DRAWSEGMENT )
            continue;
        pts = (DRAWSEGMENT*) PtStruct;
        ux0 = pts->m_Start.x;
        uy0 = pts->m_Start.y;

        dx = pts->m_End.x - ux0;
        dy = pts->m_End.y - uy0;
        spot_cX = ref.x - ux0;
        spot_cY = ref.y - uy0;

        if( pts->GetLayer() != screen->m_Active_Layer )
            continue;

        if( ( pts->m_Shape == S_CIRCLE ) || ( pts->m_Shape == S_ARC ) )
        {
            int rayon, dist, StAngle, EndAngle, MouseAngle;
            rayon = (int) hypot( (double) (dx), (double) (dy) );
            dist  = (int) hypot( (double) (spot_cX), (double) (spot_cY) );
            if( abs( rayon - dist ) <= ( pts->m_Width / 2 ) )
            {
                if( pts->m_Shape == S_CIRCLE )
                    return pts;

                MouseAngle = (int) ArcTangente( spot_cY, spot_cX );
                StAngle    = (int) ArcTangente( dy, dx );
                EndAngle   = StAngle + pts->m_Angle;

                if( EndAngle > 3600 )
                {
                    StAngle -= 3600; EndAngle -= 3600;
                }
                if( ( MouseAngle >= StAngle ) && ( MouseAngle <= EndAngle ) )
                    return pts;
            }
        }
        else
        {
            if( distance( pts->m_Width / 2 ) )
                return pts;
        }
    }

    return NULL;
}


/*
 * 1 - Locate segment of track at current cursor position.
 * 2 - Locate  segment of track point by point.
 * Ref_pX, ref_pY.r
 *
 * The search begins to address start_adresse
 */
TRACK* Locate_Pistes( TRACK* start_adresse, int Layer, int typeloc )
{
    wxPoint ref;

    SET_REF_POS( ref );

    return Locate_Pistes( start_adresse, ref, Layer );
}


TRACK* Locate_Pistes( TRACK* start_adresse, wxPoint ref, int Layer )
{
    TRACK* Track;
    int    l_piste;             /* half-width of the track */

    for( Track = start_adresse; Track != NULL; Track = Track->Next() )
    {
        if( Track->GetState( BUSY | DELETED ) )
            continue;
        /* Calculate coordinates of the test segment. */
        l_piste = Track->m_Width >> 1;
        ux0     = Track->m_Start.x;
        uy0     = Track->m_Start.y;
        dx = Track->m_End.x;
        dy = Track->m_End.y;

        dx -= ux0;
        dy -= uy0;
        spot_cX = ref.x - ux0;
        spot_cY = ref.y - uy0;

        if( Track->Type() == TYPE_VIA )
        {
            if( ( abs( spot_cX ) <= l_piste ) && ( abs( spot_cY ) <=l_piste ) )
            {
                return Track;
            }
            continue;
        }

        if( Layer >= 0 )
            if( Track->GetLayer() != Layer )
                continue;
        if( distance( l_piste ) )
            return Track;
    }

    return NULL;
}


/*
 * Locate zone area at the cursor position.
 *
 * The search begins to address start_adresse
 */
TRACK* Locate_Zone( TRACK* start_adresse, int layer, int typeloc )
{
    wxPoint ref;

    SET_REF_POS( ref );

    return Locate_Zone( start_adresse, ref, layer );
}


/*
 * Locate zone area at point.
 *
 * The search begins to address start_adresse
 */
TRACK* Locate_Zone( TRACK* start_adresse, wxPoint ref, int layer )
{
    TRACK* Zone;
    int    l_segm;

    for( Zone = start_adresse; Zone != NULL; Zone = Zone->Next() )
    {
        l_segm = Zone->m_Width >> 1;
        ux0    = Zone->m_Start.x;
        uy0    = Zone->m_Start.y;
        dx     = Zone->m_End.x;
        dy     = Zone->m_End.y;

        dx -= ux0;
        dy -= uy0;
        spot_cX = ref.x - ux0;
        spot_cY = ref.y - uy0;

        if( ( layer != -1 ) && ( Zone->GetLayer() != layer ) )
            continue;
        if( distance( l_segm ) )
            return Zone;
    }

    return NULL;
}


/* Location of text on the PCB:
 * INPUT: char pointer to the beginning of the search area
 * Return: pointer to the text description located.
 */
TEXTE_PCB* Locate_Texte_Pcb( TEXTE_PCB* pt_txt_pcb, int typeloc )
{
    int             angle;
    EDA_BaseStruct* PtStruct;
    wxPoint         ref;

    SET_REF_POS( ref );
    PtStruct = (EDA_BaseStruct*) pt_txt_pcb;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_TEXTE )
            continue;
        pt_txt_pcb = (TEXTE_PCB*) PtStruct;

        angle = pt_txt_pcb->m_Orient;
        ux0   = pt_txt_pcb->m_Pos.x;
        uy0   = pt_txt_pcb->m_Pos.y;
        dx    = ( pt_txt_pcb->m_Size.x * pt_txt_pcb->GetLength() ) / 2;
        dy    = pt_txt_pcb->m_Size.y / 2;

        dx *= 13;
        dx /= 9;    /* Character for factor 13/9. */

        /* Cursor in the rectangle around the center.  */
        spot_cX = ref.x - ux0;
        spot_cY = ref.y - uy0;
        RotatePoint( &spot_cX, &spot_cY, -angle );
        if( ( abs( spot_cX ) <= abs( dx ) ) && ( abs( spot_cY ) <= abs( dy ) ) )
            return pt_txt_pcb;
    }

    return NULL;
}


/*
 * Calculate the distance from the cursor to a line segment:
 * (Track, edge, contour module ..
 * Returns:
 * 0 if distance > threshold
 * 1 if distance <= threshold
 * Variables used (must be initialized before use, and
 * are brought to the mark center on the origin of the segment)
 * dx, dy = coord of extremity segment.
 * spot_cX, spot_cY = coord of mouse cursor
 * Search 4 cases:
 *   Horizontal segment
 *   Vertical segment
 *   Segment 45
 *   Any segment
 */
int distance( int seuil )
{
    int cXrot, cYrot, segX, segY;
    int pointX, pointY;

    segX = dx;
    segY = dy;
    pointX = spot_cX;
    pointY = spot_cY;

    /* Reroute coordinate for the segment in 1st quadrant (coord> = 0). */
    if( segX < 0 )   /* Set > 0 if symmetrical about the axis Y. */
    {
        segX   = -segX;
        pointX = -pointX;
    }
    if( segY < 0 )   /* Set > 0 if symmetrical about the axis X. */
    {
        segY   = -segY;
        pointY = -pointY;
    }


    if( segY == 0 ) /* Horizontal track. */
    {
        if( abs( pointY ) <= seuil )
        {
            if( ( pointX >= 0 ) && ( pointX <= segX ) )
                return 1;

            if( ( pointX < 0 ) && ( pointX >= -seuil ) )
            {
                if( ( ( pointX * pointX ) + ( pointY * pointY ) ) <=
                    ( seuil * seuil ) )
                    return 1;
            }
            if( ( pointX > segX ) && ( pointX <= ( segX + seuil ) ) )
            {
                if( ( ( ( pointX - segX ) * ( pointX - segX ) ) +
                      ( pointY * pointY ) ) <= ( seuil * seuil ) )
                    return 1;
            }
        }
    }
    else if( segX == 0 ) /* Vertical track. */
    {
        if( abs( pointX ) <= seuil )
        {
            if( ( pointY >= 0 ) && ( pointY <= segY ) )
                return 1;
            if( ( pointY < 0 ) && ( pointY >= -seuil ) )
            {
                if( ( (pointY * pointY ) + ( pointX * pointX ) ) <=
                   ( seuil * seuil ) )
                    return 1;
            }
            if( ( pointY > segY ) && ( pointY <= ( segY + seuil ) ) )
            {
                if( ( ( ( pointY - segY ) * ( pointY - segY ) ) +
                      ( pointX * pointX ) ) <= ( seuil * seuil ) )
                    return 1;
            }
        }
    }
    else if( segX == segY )    /* 45 degree track. */
    {
        /* You spin axes of 45 degrees. mouse was then
         * coord: x1 = x * y * cos45 + sin45
         * y1 = y * cos45 - sin45 x *
         * And the segment of track is horizontal.
         * coord recalculation of the mouse (sin45 = cos45 = .707 = 7 / 10
         * Note: sin or cos45 = .707, and when recalculating coord
         * dX45 and dy45, lect coeff .707 is neglected, dx and dy are both
         * actually .707
         * too big. (security hole too small)
         * spot_cX *, Y * must be by .707 * .707 = 0.5
         */

        cXrot = (pointX + pointY) >> 1;
        cYrot = (pointY - pointX) >> 1;

        /* Recalculate coordinates of extremity segment, which will be vertical
         * following the orientation of axes on the screen: DX45 = pointx
         * (or pointy) and 1.414 is actually greater, and dy45 = 0
         *
         * Threshold should be .707 to reflect the difference in coeff dx, dy
         */
        seuil *= 7; seuil /= 10;
        if( abs( cYrot ) <= seuil )
        {
            if( ( cXrot >= 0 ) && ( cXrot <= segX ) )
                return 1;

            if( ( cXrot < 0 ) && ( cXrot >= -seuil ) )
            {
                if( ( ( cXrot * cXrot ) + ( cYrot * cYrot ) )
                    <= ( seuil * seuil ) )
                    return 1;
            }
            if( ( cXrot > segX ) && ( cXrot <= ( segX + seuil ) ) )
            {
                if( ( ( ( cXrot - segX ) * ( cXrot - segX ) ) +
                      ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return 1;
            }
        }
    }
    else    /* Any orientation. */
    {
        /* There is a change of axis (rotation), so that the segment
         * track is horizontal in the new reference, */
        int angle;

        angle = (int) ( atan2( (double) segY, (double) segX ) * 1800 / M_PI);
        cXrot = pointX;
        cYrot = pointY;
        RotatePoint( &cXrot, &cYrot, angle );   /* Rotate test point. */
        RotatePoint( &segX, &segY, angle );     /* Rotate segment. */

        /* The track is horizontal, following the changes to coordinate
         * axis and, therefore segX = length of segment
         */

        if( abs( cYrot ) <= seuil )
        {
            if( ( cXrot >= 0 ) && ( cXrot <= segX ) )
                return 1;

            if( ( cXrot < 0 ) && ( cXrot >= -seuil ) )
            {
                if( ( ( cXrot * cXrot ) + ( cYrot * cYrot ) )
                    <= ( seuil * seuil ) )
                    return 1;
            }
            if( ( cXrot > segX ) && ( cXrot <= ( segX + seuil ) ) )
            {
                if( ( ( ( cXrot - segX ) * ( cXrot - segX ) ) +
                      ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return 1;
            }
        }
    }

    return 0;
}
