/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#ifndef SCH_JUNCTION_H
#define SCH_JUNCTION_H


#include <sch_item.h>
#include <gal/color4d.h>
#include <geometry/shape_circle.h>

class NETLIST_OBJECT_LIST;

class SCH_JUNCTION : public SCH_ITEM
{
public:
    SCH_JUNCTION( const VECTOR2I& aPosition = VECTOR2I( 0, 0 ), int aDiameter = 0,
                  SCH_LAYER_ID aLayer = LAYER_JUNCTION );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_JUNCTION() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_JUNCTION_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_JUNCTION" );
    }

    void swapData( SCH_ITEM* aItem ) override;

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_JUNCTION* aJunction = dynamic_cast<const SCH_JUNCTION*>( aItem );

        if( aJunction )
        {
            m_lastResolvedDiameter = aJunction->m_lastResolvedDiameter;
            m_lastResolvedColor = aJunction->m_lastResolvedColor;
        }
    }

    std::vector<int> ViewGetLayers() const override;

    const BOX2I GetBoundingBox() const override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_pos += aMoveVector;
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList ) override;

    bool IsConnectable() const override { return true; }

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->IsConnectable() && ( aItem->Type() == SCH_LINE_T
                                        || aItem->Type() == SCH_SYMBOL_T
                                        || aItem->Type() == SCH_LABEL_T
                                        || aItem->Type() == SCH_GLOBAL_LABEL_T
                                        || aItem->Type() == SCH_HIER_LABEL_T
                                        || aItem->Type() == SCH_DIRECTIVE_LABEL_T );
    }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return wxString( _( "Junction" ) );
    }

    BITMAPS GetMenuImage() const override;

    VECTOR2I GetPosition() const override { return m_pos; }
    void     SetPosition( const VECTOR2I& aPosition ) override { m_pos = aPosition; }

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override { return false; }

    int GetEffectiveDiameter() const;

    int GetDiameter() const { return m_diameter; }
    void SetDiameter( int aDiameter );

    COLOR4D GetJunctionColor() const;

    COLOR4D GetColor() const { return m_color; }
    void SetColor( const COLOR4D& aColor );

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override;

    virtual bool operator <( const SCH_ITEM& aItem ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_ITEM& aOther ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override;

    SHAPE_CIRCLE getEffectiveShape() const;

private:
    VECTOR2I         m_pos;
    int              m_diameter;   ///< Zero is user default.
    COLOR4D          m_color;      ///< #COLOR4D::UNSPECIFIED is user default.

    // If real-time connectivity gets disabled (due to being too slow on a particular design),
    // we can no longer rely on getting the NetClass to find netclass-specific linestyles,
    // linewidths and colors.
    mutable int      m_lastResolvedDiameter;
    mutable COLOR4D  m_lastResolvedColor;
};


#endif    // SCH_JUNCTION_H
