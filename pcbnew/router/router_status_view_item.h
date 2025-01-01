/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ROUTER_STATUS_VIEW_ITEM_H
#define ROUTER_STATUS_VIEW_ITEM_H

#include <cstdio>

#include <view/view.h>
#include <view/view_item.h>
#include <view/view_group.h>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>

#include <gal/color4d.h>

#include <eda_item.h>


class ROUTER_STATUS_VIEW_ITEM : public EDA_ITEM
{
public:
    ROUTER_STATUS_VIEW_ITEM() :
            EDA_ITEM( NOT_USED )    // Never added to anything - just a preview
    { }

    wxString GetClass() const override { return wxT( "ROUTER_STATUS" ); }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

    VECTOR2I GetPosition() const override { return m_pos; }
    void     SetPosition( const VECTOR2I& aPos ) override { m_pos = aPos; };

    void SetMessage( const wxString& aStatus )
    {
        m_status = aStatus;
    }

    void SetHint( const wxString& aHint )
    {
        m_hint = aHint;
    }

    const BOX2I ViewBBox() const override;
    std::vector<int> ViewGetLayers() const override;
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

protected:
    VECTOR2I m_pos;
    wxString m_status;
    wxString m_hint;
};



#endif  // ROUTER_STATUS_VIEW_ITEM_H
