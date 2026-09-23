/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <preview_items/simple_overlay_item.h>
#include <preview_items/two_point_geom_manager.h>


/**
 * Preview of the axis the selection is about to be mirrored about.
 */
class MIRROR_AXIS_ITEM : public KIGFX::PREVIEW::SIMPLE_OVERLAY_ITEM
{
public:
    explicit MIRROR_AXIS_ITEM( const KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER& aGeomMgr ) :
            m_geomMgr( aGeomMgr )
    {
    }

    const BOX2I ViewBBox() const override
    {
        BOX2I bbox( m_geomMgr.GetOrigin(), m_geomMgr.GetEnd() - m_geomMgr.GetOrigin() );
        bbox.Normalize();
        return bbox;
    }

    wxString GetClass() const override { return wxT( "MIRROR_AXIS_ITEM" ); }

#if defined( DEBUG )
    void Show( int, std::ostream& ) const override {}
#endif

private:
    void drawPreviewShape( KIGFX::VIEW* aView ) const override
    {
        KIGFX::GAL* gal = aView->GetGAL();

        gal->SetLineWidth( aView->ToWorld( 1.0 ) );
        gal->DrawLine( m_geomMgr.GetOrigin(), m_geomMgr.GetEnd() );
    }

    const KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER& m_geomMgr;
};