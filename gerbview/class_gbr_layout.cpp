/**
 * @file class_gbr_layout.cpp
 * @brief  GBR_LAYOUT class functions.
 */

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <class_gbr_layout.h>

GBR_LAYOUT::GBR_LAYOUT()
{
    PAGE_INFO pageInfo( wxT( "GERBER" ) );
    SetPageSettings( pageInfo );
    m_printLayersMask = -1;
}


GBR_LAYOUT::~GBR_LAYOUT()
{
}

/* Function IsLayerVisible
 * tests whether a given layer is visible
 * param aLayerIndex = The index of the layer to be tested
 * return bool - true if the layer is visible.
 */
bool GBR_LAYOUT::IsLayerVisible( int aLayerIndex ) const
{
    return m_printLayersMask & (1 << aLayerIndex );
}


EDA_RECT GBR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;

    for( GERBER_DRAW_ITEM* gerb_item = m_Drawings; gerb_item; gerb_item = gerb_item->Next() )
        bbox.Merge( gerb_item->GetBoundingBox() );

    SetBoundingBox( bbox );
    return bbox;
}
