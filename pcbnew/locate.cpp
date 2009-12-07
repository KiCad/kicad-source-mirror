/*******************/
/* Locate element. */
/*******************/


#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "class_board_design_settings.h"

#include "protos.h"



/**
 * Function RefPos
 * returns the reference position, coming from either the mouse position or the
 * the cursor position, based on whether the typeloc has the CURSEUR_OFF_GRILLE
 * flag ORed in or not.
 * @param typeloc int with possible CURSEUR_OFF_GRILLE bit on.
 * @return wxPoint - The reference point, either the mouse position or
 *   the cursor position.
 */
wxPoint inline RefPos( int typeloc )
{
    return ActiveScreen->RefPos( (typeloc & CURSEUR_OFF_GRILLE) != 0 );
}


/* Locates a via point pX, pY
 * If layer < 0 will be located via whatever layer
 * If layer = 0 .. 15 Via will be located according to its type:
 * - Traverse: all layers
 * - = Blind between layers useful
 * - Blind idem
 * Entry: coord point of reference, layer
 * Output: NULL if not via
 * (* TRACK) address via
 */
TRACK* Locate_Via( BOARD* Pcb, const wxPoint& pos, int layer )
{
    TRACK* track;

    for( track = Pcb->m_Track;  track; track = track->Next() )
    {
        if( track->Type() != TYPE_VIA )
            continue;
        if( track->m_Start != pos )
            continue;
        if( track->GetState( BUSY | DELETED ) )
            continue;
        if( layer < 0 )
            break;
        if( track->IsOnLayer( layer ) )
            break;
    }

    return track;
}


TRACK* Locate_Via_Area( TRACK* aStart, const wxPoint& pos, int layer )
{
    TRACK* track;

    for( track = aStart;   track;  track = track->Next() )
    {
        if( track->Type() != TYPE_VIA )
            continue;
        if( !track->HitTest(pos) )
            continue;
        if( track->GetState( BUSY | DELETED ) )
            continue;
        if( layer < 0 )
            break;
        if( track->IsOnLayer( layer ) )
            break;
    }

    return track;
}


/* Location of the pellet CONNECTED developed a test track
 * input: ptr_piste: pointer to the segment of track
 * Extr = flag = START -> beginning of the test segment
 * END -> end of the segment to be tested
 * Returns:
 * A pointer to the description of the pad if found.
 * NULL pointer if pad NOT FOUND
 */
D_PAD* Locate_Pad_Connecte( BOARD* Pcb, TRACK* ptr_piste, int extr )
{
    D_PAD*  ptr_pad = NULL;
    wxPoint ref_pos;

    int     masque_layer = g_TabOneLayerMask[ptr_piste->GetLayer()];

    if( extr == START )
    {
        ref_pos = ptr_piste->m_Start;
    }
    else
    {
        ref_pos = ptr_piste->m_End;
    }

    for( MODULE* module = Pcb->m_Modules;  module;  module = module->Next() )
    {
        ptr_pad = Locate_Pads( module, ref_pos, masque_layer );
        if( ptr_pad != NULL )
            break;
    }

    return ptr_pad;
}


/*
 * Locate pad pointed to by the coordinates ref_pos.x,, ref_pos.y or
 * the mouse, search done on all tracks.
 * Input:
 * - mouse coord or ref_pos
 * Returns:
 * Pointer to the pad if found.
 * NULL pointer if pad NOT FOUND
 * Num_empr = number of fingerprint pad
 *
 * Priority is the active layer
 */
D_PAD* Locate_Any_Pad( BOARD* Pcb, int typeloc, bool OnlyCurrentLayer )
{
    wxPoint ref_pos = RefPos( typeloc );
    return Locate_Any_Pad( Pcb, ref_pos, OnlyCurrentLayer );
}


D_PAD* Locate_Any_Pad( BOARD* Pcb, const wxPoint& ref_pos,
                       bool OnlyCurrentLayer )
{
    int layer_mask =
        g_TabOneLayerMask[ ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer];

    for( MODULE* module=Pcb->m_Modules;  module;   module = module->Next() )
    {
        D_PAD*  pt_pad;

        /* First: Search a pad on the active layer: */
        if( ( pt_pad = Locate_Pads( module, ref_pos, layer_mask ) ) != NULL )
            return pt_pad;

        /* If not found, search on other layers: */
        if( !OnlyCurrentLayer )
        {
            if( ( pt_pad = Locate_Pads( module, ref_pos, ALL_LAYERS ) ) != NULL )
                return pt_pad;
        }
    }

    return NULL;
}


/* Locate pad pointed to by the coordinate ref_pos.x,, ref_pos.y or
 * the mouse on the current footprint.
 * Input:
 * - General parameters of the footprint update by characters()
 * - = Masque_layer layer(s) (bit_masque) which must be the pad
 * Returns:
 * A pointer to the pad if found
 * NULL pointer if pad NOT FOUND
 */
D_PAD* Locate_Pads( MODULE* module, int masque_layer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );
    return Locate_Pads( module, ref_pos, masque_layer );
}


D_PAD* Locate_Pads( MODULE* module, const wxPoint& ref_pos, int masque_layer )
{
    for( D_PAD* pt_pad = module->m_Pads;   pt_pad;   pt_pad = pt_pad->Next() )
    {
        /* ... and on the correct layer. */
        if( ( pt_pad->m_Masque_Layer & masque_layer ) == 0 )
            continue;

        if( pt_pad->HitTest( ref_pos ) )
            return pt_pad;
    }

    return NULL;
}


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
 * @param Pcb The BOARD to search within.
 * @param typeloc Flag bits, tuning the search, see pcbnew.h
 * @return MODULE* - the best module or NULL if none.
 */
MODULE* Locate_Prefered_Module( BOARD* Pcb, int typeloc )
{
    MODULE* pt_module;
    int     lx, ly;
    MODULE* module      = NULL;
    MODULE* Altmodule   = NULL;
    int     min_dim     = 0x7FFFFFFF;
    int     alt_min_dim = 0x7FFFFFFF;
    int     layer;
    wxPoint ref_pos;

    ref_pos = RefPos( typeloc );

    pt_module = Pcb->m_Modules;
    for(  ;  pt_module;  pt_module = (MODULE*) pt_module->Next() )
    {
        // is the ref point within the module's bounds?
        if( !pt_module->HitTest( ref_pos ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked,
        // skip it.
        if( (typeloc & IGNORE_LOCKED) && pt_module->IsLocked() )
            continue;

        /* Calculate priority: the priority is given to the layer of the
         * module and the copper layer if the module layer is indelible,
         * adhesive copper, a layer if cmp module layer is indelible,
         * adhesive component.
         */
        layer = pt_module->GetLayer();

        if( layer==ADHESIVE_N_CU || layer==SILKSCREEN_N_CU )
            layer = LAYER_N_BACK;

        else if( layer==ADHESIVE_N_CMP || layer==SILKSCREEN_N_CMP )
            layer = LAYER_N_FRONT;

        /* Test of minimum size to choosing the best candidate. */

        int offx = pt_module->m_BoundaryBox.m_Size.x / 2 +
            pt_module->m_BoundaryBox.m_Pos.x +
            pt_module->m_Pos.x;
        int offy = pt_module->m_BoundaryBox.m_Size.y / 2
            + pt_module->m_BoundaryBox.m_Pos.y
            + pt_module->m_Pos.y;
        //off x & offy point to the middle of the box.
        int dist = abs( ref_pos.x - offx ) + abs( ref_pos.y - offy );
        lx = pt_module->m_BoundaryBox.GetWidth();
        ly = pt_module->m_BoundaryBox.GetHeight();
        //int dist = MIN(lx, ly);  // to pick the smallest module (kinda
        // screwy with same-sized modules -- this is bad!)

        if( ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer == layer )
        {
            if( dist <= min_dim )
            {
                /* better footprint shown on the active layer */
                module  = pt_module;
                min_dim = dist;
            }
        }
        else if( !( typeloc & MATCH_LAYER )
            && ( !( typeloc & VISIBLE_ONLY )
                 || IsModuleLayerVisible( layer ) ) )
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


/** Search for the track (or via) segment which is connected to the track
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
TRACK* Locate_Piste_Connectee( TRACK* PtRefSegm, TRACK* pt_base,
                               TRACK* pt_lim, int extr )
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
            if( PtSegmN->GetState( BUSY | DELETED ) )
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
            if( PtSegmB->GetState( BUSY | DELETED ) )
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
        if( PtSegmN->GetState( DELETED | BUSY ) )
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
TRACK* Locate_Pistes( TRACK* start_adresse, int MasqueLayer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );

    return Locate_Pistes( start_adresse, ref_pos, MasqueLayer );
}


TRACK* Locate_Pistes( TRACK* start_adresse, const wxPoint& ref_pos,
                      int MasqueLayer )
{
    for( TRACK* track = start_adresse;   track;  track =  track->Next() )
    {
        int layer = track->GetLayer();

        if( track->GetState( BUSY | DELETED ) )
        {
            // D( printf( "track %p is BUSY | DELETED. BUSY=%d DELETED=%d\n",
            //            track, track->GetState( BUSY ),
            //            track->GetState( DELETED ) );)
            continue;
        }

        if( g_DesignSettings.IsLayerVisible( layer ) == false )
            continue;

        if( track->Type() == TYPE_VIA ) /* VIA encountered. */
        {
            if( track->HitTest( ref_pos ) )
                return track;
        }
        else
        {
            if( (g_TabOneLayerMask[layer] & MasqueLayer) == 0 )
                continue;   /* Segments on different layers. */

            if( track->HitTest( ref_pos ) )
                return track;
        }
    }

    return NULL;
}

/*
 * 1 - Locate zone area by the mouse.
 * 2 - Locate zone area by point
 * def_pos.x, ref_pos.y.r
 *
 * If layer == -1, tst layer is not
 *
 * The search begins to address start_adresse
 */
TRACK* Locate_Zone( TRACK* start_adresse, int layer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );

    return Locate_Zone( start_adresse, ref_pos, layer );
}


TRACK* Locate_Zone( TRACK* start_adresse, const wxPoint& ref_pos, int layer )
{
    for( TRACK* Zone = start_adresse;  Zone;   Zone =  Zone->Next() )
    {
        if( (layer != -1) && (Zone->GetLayer() != layer) )
            continue;

        if( Zone->HitTest( ref_pos ) )
            return Zone;
    }

    return NULL;
}


/* Find the pad center px, py,
 * The layer INDICATED BY masque_layer (bitwise)
 * (Runway end)
 * The list of pads must already exist.
 *
 * Returns:
 *   NULL if no pad located.
 *   Pointer to the structure corresponding descr_pad if pad found
 * (Good position and good layer).
 */
D_PAD* Fast_Locate_Pad_Connecte( BOARD* Pcb, const wxPoint& ref_pos,
                                 int masque_layer )
{
    for( unsigned i=0; i<Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = Pcb->m_NetInfo->GetPad(i);

        if( pad->m_Pos != ref_pos )
            continue;

        /* Pad found, it must be on the correct layer */
        if( pad->m_Masque_Layer & masque_layer )
            return pad;
    }

    return NULL;
}


/* Locate segment with one end that coincides with the point x, y
 * Data on layers by masklayer
 * Research is done to address start_adr has end_adr
 * If end_adr = NULL, end search list
 * The segments of track marks with the flag are not DELETED or taken
 * into account
 */
TRACK* Fast_Locate_Piste( TRACK* start_adr, TRACK* end_adr,
                          const wxPoint& ref_pos, int MaskLayer )
{
    TRACK* PtSegm;

    if( start_adr == NULL )
        return NULL;

    for( PtSegm = start_adr; PtSegm != NULL; PtSegm =  PtSegm->Next() )
    {
        if( PtSegm->GetState( DELETED | BUSY ) == 0 )
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


/* Locates via through the point x, y, on layer data by masklayer.
 * Search is done to address start_adr has end_adr.
 * If end_adr = NULL, end search list
 * Vias whose parameter has the State or DELETED bit BUSY = 1 are ignored
 */
TRACK* Fast_Locate_Via( TRACK* start_adr, TRACK* end_adr,
                        const wxPoint& pos, int MaskLayer )
{
    TRACK* PtSegm;

    for( PtSegm = start_adr; PtSegm != NULL; PtSegm = PtSegm->Next() )
    {
        if( PtSegm->Type() == TYPE_VIA )
        {
            if( pos == PtSegm->m_Start )
            {
                if( PtSegm->GetState( BUSY | DELETED ) == 0 )
                {
                    if( MaskLayer & PtSegm->ReturnMaskLayer() )
                        return PtSegm;
                }
            }
        }
        if( PtSegm == end_adr )
            break;
    }

    return NULL;
}
