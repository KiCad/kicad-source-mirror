/**
 * @file class_gbr_layout.cpp
 * @brief  PL_EDITOR_LAYOUT class functions.
 */

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <class_pl_editor_layout.h>

PL_EDITOR_LAYOUT::PL_EDITOR_LAYOUT()
{
    PAGE_INFO pageInfo( wxT( "A4" ) );
    SetPageSettings( pageInfo );
}


PL_EDITOR_LAYOUT::~PL_EDITOR_LAYOUT()
{
}

EDA_RECT PL_EDITOR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;

    SetBoundingBox( bbox );
    return bbox;
}
