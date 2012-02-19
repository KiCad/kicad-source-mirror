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
#include <class_pad.h>
#include <class_edge_mod.h>
#include <class_module.h>


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


void MODULE::Move( const wxPoint& aMoveVector )
{
    wxPoint newpos = m_Pos + aMoveVector;
    SetPosition( newpos );
}


void MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint newpos = m_Pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( GetOrientation() + aAngle );
}


void MODULE::Flip( const wxPoint& aCentre )
{
    TEXTE_MODULE* pt_texte;

    // Move module to its final position:
    wxPoint finalPos = m_Pos;

    finalPos.y  = aCentre.y - ( finalPos.y - aCentre.y );     /// Mirror the Y position

    SetPosition( finalPos );

    // Flip layer
    SetLayer( ChangeSideNumLayer( GetLayer() ) );

    // Reverse mirror orientation.
    NEGATE( m_Orient );
    NORMALIZE_ANGLE_POS( m_Orient );

    // Mirror pads to other side of board about the x axis, i.e. vertically.
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
        pad->Flip( m_Pos.y );

    // Mirror reference.
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

    // Mirror value.
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

    // Reverse mirror module graphics and texts.
    for( EDA_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
            {
                EDGE_MODULE*  em = (EDGE_MODULE*) item;

                wxPoint s = em->GetStart();
                s.y -= m_Pos.y;
                s.y  = -s.y;
                s.y += m_Pos.y;
                em->SetStart( s );

                wxPoint e = em->GetEnd();
                e.y -= m_Pos.y;
                e.y  = -e.y;
                e.y += m_Pos.y;
                em->SetEnd( e );

                NEGATE( em->m_Start0.y );
                NEGATE( em->m_End0.y );

                if( em->GetShape() == S_ARC )
                {
                    em->SetAngle( -em->GetAngle() );
                }

                em->SetLayer( ChangeSideNumLayer( em->GetLayer() ) );
            }
            break;

        case PCB_MODULE_TEXT_T:
            // Reverse mirror position and mirror.
            pt_texte = (TEXTE_MODULE*) item;
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
        pad->SetPosition( pad->GetPosition() + delta );
    }

    for( EDA_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) item;
            pt_edgmod->SetDrawCoord();
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) item;
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


void MODULE::SetOrientation( double newangle )
{
    double  angleChange = newangle - m_Orient;  // change in rotation
    wxPoint pt;

    NORMALIZE_ANGLE_POS( newangle );

    m_Orient = newangle;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pt = pad->GetPos0();

        pad->SetOrientation( pad->GetOrientation() + angleChange );

        RotatePoint( &pt, m_Orient );

        pad->SetPosition( GetPosition() + pt );
    }

    // Update of the reference and value.
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    // Displace contours and text of the footprint.
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->Type() == PCB_MODULE_EDGE_T )
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) item;
            pt_edgmod->SetDrawCoord();
        }

        else if( item->Type() == PCB_MODULE_TEXT_T )
        {
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) item;
            pt_texte->SetDrawCoord();
        }
    }

    CalculateBoundingBox();
}
