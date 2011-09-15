/*******************/
/* Locate element. */
/*******************/


#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "class_board_design_settings.h"

#include "protos.h"


/**
 * Function Locate_Prefered_Module
 * locates a footprint by its bounding rectangle.  If several footprints
 * are possible, then the priority is: the closest on the active layer, then
 * closest.
 * The current mouse or cursor coordinates are grabbed from the active window
 * to perform hit-testing.
 * distance is calculated via manhattan distance from the center of the
 * bounding rectangle to the cursor position.
 *
 * @param aPcb The BOARD to search within.
 * @param aPosition Flag bits, tuning the search, see pcbnew.h
 * @param aActiveLayer Layer to test.
 * @param aVisibleOnly Search only the visible layers if true.
 * @param aIgnoreLocked Ignore locked modules when true.
 * @return MODULE* The best module or NULL if none.
 */
MODULE* Locate_Prefered_Module( BOARD* aPcb, const wxPoint& aPosition, int aActiveLayer,
                                bool aVisibleOnly, bool aIgnoreLocked )
{
    MODULE* pt_module;
    MODULE* module      = NULL;
    MODULE* Altmodule   = NULL;
    int     min_dim     = 0x7FFFFFFF;
    int     alt_min_dim = 0x7FFFFFFF;
    int     layer;

    pt_module = aPcb->m_Modules;

    for(  ;  pt_module;  pt_module = (MODULE*) pt_module->Next() )
    {
        // is the ref point within the module's bounds?
        if( !pt_module->HitTest( aPosition ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked, skip it.
        if( aIgnoreLocked && pt_module->IsLocked() )
            continue;

        /* Calculate priority: the priority is given to the layer of the
         * module and the copper layer if the module layer is indelible,
         * adhesive copper, a layer if cmp module layer is indelible,
         * adhesive component.
         */
        layer = pt_module->GetLayer();

        if( layer==ADHESIVE_N_BACK || layer==SILKSCREEN_N_BACK )
            layer = LAYER_N_BACK;
        else if( layer==ADHESIVE_N_FRONT || layer==SILKSCREEN_N_FRONT )
            layer = LAYER_N_FRONT;

        /* Test of minimum size to choosing the best candidate. */

        EDA_RECT bb = pt_module->GetFootPrintRect();
        int offx = bb.GetX() + bb.GetWidth() / 2;
        int offy = bb.GetY() + bb.GetHeight() / 2;

        //off x & offy point to the middle of the box.
        int dist = abs( aPosition.x - offx ) + abs( aPosition.y - offy );

        //int dist = MIN(lx, ly);  // to pick the smallest module (kinda
        // screwy with same-sized modules -- this is bad!)

        if( aActiveLayer == layer )
        {
            if( dist <= min_dim )
            {
                /* better footprint shown on the active layer */
                module  = pt_module;
                min_dim = dist;
            }
        }
        else if( aVisibleOnly && aPcb->IsModuleLayerVisible( layer ) )
        {
            if( dist <= alt_min_dim )
            {
                /* better footprint shown on other layers */
                Altmodule   = pt_module;
                alt_min_dim = dist;
            }
        }
    }

    if( module )
    {
        return module;
    }

    if( Altmodule )
    {
        return Altmodule;
    }

    return NULL;
}


/*
 * return true if the dist between p1 and p2 < max_dist
 * Currently in test (currently rasnest algos work only if p1 == p2)
*/
inline bool IsPointsAreNear(wxPoint & p1, wxPoint & p2, int max_dist)
{
#if 0   // Do not change it: does not work
{
int dist;
    dist = abs(p1.x - p2.x) + abs (p1.y - p2.y);
    dist *= 7;
    dist /= 10;
    if ( dist < max_dist ) return true;
}
#else
    if ( p1 == p2 ) return true;
#endif
    return false;
}


/**
 * Search for the track (or via) segment which is connected to the track
 *  segment PtRefSegm
 *  if extr == START, the starting track segment PtRefSegm is used to locate
 *  a connected segment
 *  if extr == END, the ending track segment PtRefSegm is used
 *  The test connection consider only end track segments
 *
 *  Search is made from  pt_base to pt_lim (in the track linked list)
 *  if pt_lim == NULL,  the search  is made from  pt_base to the end of list
 *
 *  In order to have a fast computation time:
 *  a first search is made considering only the +/- 50 next door neighbor
 *  of PtRefSegm.
 *  if no track is found : the entire list is tested
 *
 *  @param PtRefSegm = reference segment
 *  @param pt_base = lower limit for search
 *  @param pt_lim = upper limit for search (can be NULL)
 *  @param extr = START or END = end of ref track segment to use in tests
 */
TRACK* GetConnectedTrace( TRACK* PtRefSegm, TRACK* pt_base, TRACK* pt_lim, int extr )
{
    const int NEIGHTBOUR_COUNT_MAX = 50;

    TRACK*  PtSegmB, * PtSegmN;
    int     Reflayer;
    wxPoint pos_ref;
    int     ii;
    int     max_dist;

    if( extr == START )
        pos_ref = PtRefSegm->m_Start;
    else
        pos_ref = PtRefSegm->m_End;

    Reflayer = PtRefSegm->ReturnMaskLayer();

    PtSegmB = PtSegmN = PtRefSegm;

    for( ii = 0; ii < NEIGHTBOUR_COUNT_MAX; ii++ )
    {
        if( (PtSegmN == NULL) && (PtSegmB == NULL) )
            break;

        if( PtSegmN )
        {
            if( PtSegmN->GetState( BUSY | IS_DELETED ) )
                goto suite;

            if( PtSegmN == PtRefSegm )
                goto suite;

            /* max_dist is the max distance between 2 track ends which
             * ensure a copper continuity */
            max_dist = (PtSegmN->m_Width + PtRefSegm->m_Width)/2;

            if( IsPointsAreNear(pos_ref, PtSegmN->m_Start, max_dist) )
            {
                if( Reflayer & PtSegmN->ReturnMaskLayer() )
                    return PtSegmN;
            }

            if( IsPointsAreNear(pos_ref, PtSegmN->m_End, max_dist) )
            {
                if( Reflayer & PtSegmN->ReturnMaskLayer() )
                    return PtSegmN;
            }
suite:
            if( PtSegmN == pt_lim )
                PtSegmN = NULL;
            else
                PtSegmN =  PtSegmN->Next();
        }

        if( PtSegmB )
        {
            if( PtSegmB->GetState( BUSY | IS_DELETED ) )
                goto suite1;

            if( PtSegmB == PtRefSegm )
                goto suite1;

            max_dist = (PtSegmB->m_Width + PtRefSegm->m_Width)/2;

            if( IsPointsAreNear(pos_ref, PtSegmB->m_Start, max_dist) )
            {
                if( Reflayer & PtSegmB->ReturnMaskLayer() )
                    return PtSegmB;
            }

            if( IsPointsAreNear(pos_ref, PtSegmB->m_End, max_dist) )
            {
                if( Reflayer & PtSegmB->ReturnMaskLayer() )
                    return PtSegmB;
            }
suite1:
            if( PtSegmB == pt_base )
                PtSegmB = NULL;
            else if( PtSegmB->Type() != TYPE_PCB )
                PtSegmB =  PtSegmB->Back();
            else
                PtSegmB = NULL;
        }
    }

    /* General search. */
    for( PtSegmN = pt_base; PtSegmN != NULL; PtSegmN =  PtSegmN->Next() )
    {
        if( PtSegmN->GetState( IS_DELETED | BUSY ) )
        {
            if( PtSegmN == pt_lim )
                break;

            continue;
        }

        if( PtSegmN == PtRefSegm )
        {
            if( PtSegmN == pt_lim )
                break;

            continue;
        }

        max_dist = ( PtSegmN->m_Width + PtRefSegm->m_Width ) / 2;

        if( IsPointsAreNear( pos_ref, PtSegmN->m_Start, max_dist ) )
        {
            if( Reflayer & PtSegmN->ReturnMaskLayer() )
                return PtSegmN;
        }

        if( IsPointsAreNear( pos_ref, PtSegmN->m_End, max_dist ) )
        {
            if( Reflayer & PtSegmN->ReturnMaskLayer() )
                return PtSegmN;
        }

        if( PtSegmN == pt_lim )
            break;
    }

    return NULL;
}


/*
 * 1 - Locate segment of track leading from the mouse.
 * 2 - Locate segment of track point by point
 * ref_pos.x, ref_pos.y.r
 *
 * The search begins to address start_adresse
 */
TRACK* GetTrace( BOARD* aPcb, TRACK* start_adresse, const wxPoint& ref_pos, int LayerMask )
{
    for( TRACK* track = start_adresse;   track;  track =  track->Next() )
    {
        int layer = track->GetLayer();

        if( track->GetState( BUSY | IS_DELETED ) )
        {
            // D( printf( "track %p is BUSY | IS_DELETED. BUSY=%d IS_DELETED=%d\n",
            //            track, track->GetState( BUSY ),
            //            track->GetState( IS_DELETED ) );)
            continue;
        }

        if( aPcb->GetBoardDesignSettings()->IsLayerVisible( layer ) == false )
            continue;

        if( track->Type() == TYPE_VIA ) /* VIA encountered. */
        {
            if( track->HitTest( ref_pos ) )
                return track;
        }
        else
        {
            if( (g_TabOneLayerMask[layer] & LayerMask) == 0 )
                continue;   /* Segments on different layers. */

            if( track->HitTest( ref_pos ) )
                return track;
        }
    }

    return NULL;
}


/* Locate segment with one end that coincides with the point x, y
 * Data on layers by masklayer
 * Research is done to address start_adr has end_adr
 * If end_adr = NULL, end search list
 * The segments of track marks with the flag are not IS_DELETED or taken
 * into account
 */
TRACK* GetTrace( TRACK* start_adr, TRACK* end_adr, const wxPoint& ref_pos, int MaskLayer )
{
    TRACK* PtSegm;

    if( start_adr == NULL )
        return NULL;

    for( PtSegm = start_adr; PtSegm != NULL; PtSegm =  PtSegm->Next() )
    {
        if( PtSegm->GetState( IS_DELETED | BUSY ) == 0 )
        {
            if( ref_pos == PtSegm->m_Start )
            {
                if( MaskLayer & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }

            if( ref_pos == PtSegm->m_End )
            {
                if( MaskLayer & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }
        }

        if( PtSegm == end_adr )
            break;
    }

    return NULL;
}
