/**
 * @file class_module_transform_functions.cpp
 * @brief Functions of class MODULE to handle some geometric changes such as move, rotate ...
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <trigo.h>
#include <pcbcommon.h>
#include <pcbnew.h>
#include <macros.h>

#include <protos.h>
#include <class_board.h>
#include <class_pad.h>
#include <class_edge_mod.h>
#include <class_module.h>



/* Returns the layer number after flipping an item
 * some layers: external copper, Mask, Paste, and solder
 * are swapped between front and back sides
 */
int BOARD::ReturnFlippedLayerNumber( int oldlayer )
{
    int newlayer;

    switch( oldlayer )
    {
    case LAYER_N_BACK:
        newlayer = LAYER_N_FRONT;
        break;

    case LAYER_N_FRONT:
        newlayer = LAYER_N_BACK;
        break;

    case SILKSCREEN_N_BACK:
        newlayer = SILKSCREEN_N_FRONT;
        break;

    case SILKSCREEN_N_FRONT:
        newlayer = SILKSCREEN_N_BACK;
        break;

    case ADHESIVE_N_BACK:
        newlayer = ADHESIVE_N_FRONT;
        break;

    case ADHESIVE_N_FRONT:
        newlayer = ADHESIVE_N_BACK;
        break;

    case SOLDERMASK_N_BACK:
        newlayer = SOLDERMASK_N_FRONT;
        break;

    case SOLDERMASK_N_FRONT:
        newlayer = SOLDERMASK_N_BACK;
        break;

    case SOLDERPASTE_N_BACK:
        newlayer = SOLDERPASTE_N_FRONT;
        break;

    case SOLDERPASTE_N_FRONT:
        newlayer = SOLDERPASTE_N_BACK;
        break;

    default:
        newlayer = oldlayer;
    }

    return newlayer;
}


/* Calculate the mask layer when flipping a footprint
 * BACK and FRONT copper layers , mask, paste, solder layers are swapped
 */
int ChangeSideMaskLayer( int aMask )
{
    int newMask;

    newMask = aMask & ~(LAYER_BACK | LAYER_FRONT |
                           SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT |
                           ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT |
                           SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT |
                           SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT |
                           ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT);

    if( aMask & LAYER_BACK )
        newMask |= LAYER_FRONT;

    if( aMask & LAYER_FRONT )
        newMask |= LAYER_BACK;

    if( aMask & SILKSCREEN_LAYER_BACK )
        newMask |= SILKSCREEN_LAYER_FRONT;

    if( aMask & SILKSCREEN_LAYER_FRONT )
        newMask |= SILKSCREEN_LAYER_BACK;

    if( aMask & ADHESIVE_LAYER_BACK )
        newMask |= ADHESIVE_LAYER_FRONT;

    if( aMask & ADHESIVE_LAYER_FRONT )
        newMask |= ADHESIVE_LAYER_BACK;

    if( aMask & SOLDERMASK_LAYER_BACK )
        newMask |= SOLDERMASK_LAYER_FRONT;

    if( aMask & SOLDERMASK_LAYER_FRONT )
        newMask |= SOLDERMASK_LAYER_BACK;

    if( aMask & SOLDERPASTE_LAYER_BACK )
        newMask |= SOLDERPASTE_LAYER_FRONT;

    if( aMask & SOLDERPASTE_LAYER_FRONT )
        newMask |= SOLDERPASTE_LAYER_BACK;

    if( aMask & ADHESIVE_LAYER_BACK )
        newMask |= ADHESIVE_LAYER_FRONT;

    if( aMask & ADHESIVE_LAYER_FRONT )
        newMask |= ADHESIVE_LAYER_BACK;

    return newMask;
}
