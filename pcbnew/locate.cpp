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
