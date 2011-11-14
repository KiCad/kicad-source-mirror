/**
 * @file class_module_transform_functions.cpp
 * @brief Functions of class MODULE to handle some geometric changes such as move, rotate ...
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "trigo.h"
#include "pcbcommon.h"
#include "pcbnew.h"
#include "macros.h"

#include "protos.h"
#include "class_pad.h"
#include "class_edge_mod.h"
#include "class_module.h"


/* Calculate the layer number for changing cu / cmp layers for Cu / CMP
 * (Copper, Mask, Paste, solder)
 */
int ChangeSideNumLayer( int oldlayer )
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


/**
 * Function Move (virtual)
 * move this object.
 * @param aMoveVector - the move vector for this object.
 */
void MODULE::Move(const wxPoint& aMoveVector)
{
    wxPoint newpos = m_Pos + aMoveVector;
    SetPosition( newpos );
}


/**
 * Function Rotate
 * Rotate this object.
 * @param aRotCentre - the rotation point.
 * @param aAngle - the rotation angle in 0.1 degree.
 */
void MODULE::Rotate(const wxPoint& aRotCentre, int aAngle)
{
    wxPoint newpos = m_Pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( m_Orient + aAngle );
}


/**
 * Function Flip
 * Flip this object, i.e. change the board side for this object
 * @param aCentre - the rotation point.
 */
void MODULE::Flip(const wxPoint& aCentre )
{
    D_PAD*        pt_pad;
    TEXTE_MODULE* pt_texte;
    EDGE_MODULE*  pt_edgmod;
    EDA_ITEM*     PtStruct;

    // Move module to its final position:
    wxPoint finalPos = m_Pos;
    finalPos.y  = aCentre.y - ( finalPos.y - aCentre.y );     /// Mirror the Y position
    SetPosition(finalPos);

    /* Flip layer */
    SetLayer( ChangeSideNumLayer( GetLayer() ) );

    /* Reverse mirror orientation. */
    NEGATE( m_Orient );
    NORMALIZE_ANGLE_POS( m_Orient );

    /* Mirror inversion layers pads. */
    pt_pad = m_Pads;

    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        pt_pad->m_Pos.y      -= m_Pos.y;
        pt_pad->m_Pos.y       = -pt_pad->m_Pos.y;
        pt_pad->m_Pos.y      += m_Pos.y;
        NEGATE( pt_pad->m_Pos0.y );
        NEGATE( pt_pad->m_Offset.y );
        NEGATE( pt_pad->m_DeltaSize.y );
        NEGATE_AND_NORMALIZE_ANGLE_POS( pt_pad->m_Orient );

        /* flip pads layers*/
        pt_pad->m_layerMask = ChangeSideMaskLayer( pt_pad->m_layerMask );
    }

    /* Mirror reference. */
    pt_texte = m_Reference;
    pt_texte->m_Pos.y -= m_Pos.y;
    pt_texte->m_Pos.y  = -pt_texte->m_Pos.y;
    pt_texte->m_Pos.y += m_Pos.y;
    NEGATE(pt_texte->m_Pos0.y);
    pt_texte->m_Mirror = false;
    NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );
    pt_texte->SetLayer( GetLayer() );
    pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

    if( GetLayer() == LAYER_N_BACK )
        pt_texte->SetLayer( SILKSCREEN_N_BACK );

    if( GetLayer() == LAYER_N_FRONT )
        pt_texte->SetLayer( SILKSCREEN_N_FRONT );

    if( (GetLayer() == SILKSCREEN_N_BACK)
       || (GetLayer() == ADHESIVE_N_BACK) || (GetLayer() == LAYER_N_BACK) )
        pt_texte->m_Mirror = true;

    /* Mirror value. */
    pt_texte = m_Value;
    pt_texte->m_Pos.y -= m_Pos.y;
    NEGATE( pt_texte->m_Pos.y );
    pt_texte->m_Pos.y += m_Pos.y;
    NEGATE( pt_texte->m_Pos0.y );
    pt_texte->m_Mirror = false;
    NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );
    pt_texte->SetLayer( GetLayer() );
    pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

    if( GetLayer() == LAYER_N_BACK )
        pt_texte->SetLayer( SILKSCREEN_N_BACK );

    if( GetLayer() == LAYER_N_FRONT )
        pt_texte->SetLayer( SILKSCREEN_N_FRONT );

    if( (GetLayer() == SILKSCREEN_N_BACK)
       || (GetLayer() == ADHESIVE_N_BACK) || (GetLayer() == LAYER_N_BACK) )
        pt_texte->m_Mirror = true;

    /* Reverse mirror footprints. */
    PtStruct = m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_MODULE_EDGE_T:
            pt_edgmod = (EDGE_MODULE*) PtStruct;
            pt_edgmod->m_Start.y -= m_Pos.y;
            pt_edgmod->m_Start.y  = -pt_edgmod->m_Start.y;
            pt_edgmod->m_Start.y += m_Pos.y;
            pt_edgmod->m_End.y   -= m_Pos.y;
            pt_edgmod->m_End.y    = -pt_edgmod->m_End.y;
            pt_edgmod->m_End.y   += m_Pos.y;
            NEGATE( pt_edgmod->m_Start0.y );
            NEGATE( pt_edgmod->m_End0.y );
            if( pt_edgmod->m_Shape == S_ARC )
            {
                NEGATE(pt_edgmod->m_Angle);
            }

            pt_edgmod->SetLayer( ChangeSideNumLayer( pt_edgmod->GetLayer() ) );
            break;

        case PCB_MODULE_TEXT_T:
            /* Reverse mirror position and mirror. */
            pt_texte = (TEXTE_MODULE*) PtStruct;
            pt_texte->m_Pos.y -= m_Pos.y;
            pt_texte->m_Pos.y  = -pt_texte->m_Pos.y;
            pt_texte->m_Pos.y += m_Pos.y;
            NEGATE( pt_texte->m_Pos0.y );
            pt_texte->m_Mirror = false;
            NEGATE_AND_NORMALIZE_ANGLE_POS( pt_texte->m_Orient );

            pt_texte->SetLayer( GetLayer() );
            pt_texte->SetLayer( ChangeSideNumLayer( pt_texte->GetLayer() ) );

            if( GetLayer() == LAYER_N_BACK )
                pt_texte->SetLayer( SILKSCREEN_N_BACK );

            if( GetLayer() == LAYER_N_FRONT )
                pt_texte->SetLayer( SILKSCREEN_N_FRONT );

            if(  GetLayer() == SILKSCREEN_N_BACK
                 || GetLayer() == ADHESIVE_N_BACK
                 || GetLayer() == LAYER_N_BACK )
            {
                pt_texte->m_Mirror = true;
            }

            break;

        default:
            wxMessageBox( wxT( "MODULE::Flip() error: Unknown Draw Type" ) );
            break;
        }
    }

    CalculateBoundingBox();
}

void MODULE::SetPosition( const wxPoint& newpos )
{
    wxPoint delta = newpos - m_Pos;

    m_Pos += delta;
    m_Reference->m_Pos += delta;
    m_Value->m_Pos += delta;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->m_Pos += delta;
    }

    EDA_ITEM* PtStruct = m_Drawings;

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) PtStruct;
            pt_edgmod->SetDrawCoord();
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) PtStruct;
            pt_texte->m_Pos += delta;
            break;
        }

        default:
            wxMessageBox( wxT( "Draw type undefined." ) );
            break;
        }
    }

    CalculateBoundingBox();
}


void MODULE::SetOrientation( int newangle )
{
    int px, py;

    newangle -= m_Orient;       // = Change in rotation

    m_Orient += newangle;
    NORMALIZE_ANGLE_POS( m_Orient );

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        px = TO_LEGACY_LU( pad->m_Pos0.x );
        py = TO_LEGACY_LU( pad->m_Pos0.y );

        pad->m_Orient += newangle; /* change m_Orientation */
        NORMALIZE_ANGLE_POS( pad->m_Orient );

        RotatePoint( &px, &py, m_Orient );
        pad->m_Pos.x = m_Pos.x + px;
        pad->m_Pos.y = m_Pos.y + py;
    }

    /* Update of the reference and value. */
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    /* Displace contours and text of the footprint. */
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) item;
            pt_edgmod->SetDrawCoord();
        }

        if( item->Type() == PCB_MODULE_TEXT_T )
        {
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) item;
            pt_texte->SetDrawCoord();
        }
    }

    CalculateBoundingBox();
}
