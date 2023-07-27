/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHOR.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCH_PIN_CONNECTION_H
#define _SCH_PIN_CONNECTION_H

#include <lib_pin.h>
#include <sch_item.h>
#include <sch_sheet_path.h>

#include <mutex>
#include <map>

class SCH_SYMBOL;
class MSG_PANEL_ITEM;


class SCH_PIN : public SCH_ITEM
{
public:
    SCH_PIN( LIB_PIN* aLibPin, SCH_SYMBOL* aParentSymbol );

    SCH_PIN( SCH_SYMBOL* aParentSymbol, const wxString& aNumber, const wxString& aAlt );

    SCH_PIN( const SCH_PIN& aPin );

    SCH_PIN& operator=( const SCH_PIN& aPin );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_PIN_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_PIN" );
    }

    SCH_SYMBOL* GetParentSymbol() const;

    LIB_PIN* GetLibPin() const { return m_libPin; }

    void ClearDefaultNetName( const SCH_SHEET_PATH* aPath );
    wxString GetDefaultNetName( const SCH_SHEET_PATH& aPath, bool aForceNoConnect = false );

    wxString GetAlt() const { return m_alt; }
    void SetAlt( const wxString& aAlt ) { m_alt = aAlt; }

    const BOX2I ViewBBox() const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override {}

    void Move( const VECTOR2I& aMoveVector ) override {}

    void MirrorHorizontally( int aCenter ) override {}
    void MirrorVertically( int aCenter ) override {}
    void Rotate( const VECTOR2I& aCenter ) override {}

    VECTOR2I       GetPosition() const override { return GetTransformedPosition(); }
    const VECTOR2I GetLocalPosition() const { return m_position; }
    void           SetPosition( const VECTOR2I& aPosition ) override { m_position = aPosition; }

    /* Cannot use a default parameter here as it will not be compatible with the virtual. */
    const BOX2I GetBoundingBox() const override { return GetBoundingBox( false, true, false ); }

    /**
     * @param aIncludeInvisibles - if false, do not include labels for invisible pins
     *      in the calculation.
     */
    const BOX2I GetBoundingBox( bool aIncludeInvisiblePins, bool aIncludeNameAndNumber,
                                bool aIncludeElectricalType ) const;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    EDA_ITEM* Clone() const override;

    bool IsConnectable() const override { return true; }

    bool IsDangling() const override
    {
        if( GetType() == ELECTRICAL_PINTYPE::PT_NC || GetType() == ELECTRICAL_PINTYPE::PT_NIC )
            return false;

        return m_isDangling;
    }

    void SetIsDangling( bool isDangling ) { m_isDangling = isDangling; }

    /**
     * @param aPin Comparison Pin
     * @return True if aPin is stacked with this pin
     */
    bool IsStacked( const SCH_PIN* aPin ) const;

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    /// @return the pin's position in global coordinates.
    VECTOR2I GetTransformedPosition() const;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override;

    /*
     * While many of these are currently simply covers for the equivalent LIB_PIN methods,
     * the new Eeschema file format will soon allow us to override them at the schematic level.
     */
    bool IsVisible() const { return m_libPin->IsVisible(); }

    wxString GetName() const;
    wxString GetShownName() const;

    wxString GetNumber() const { return m_number; }
    wxString GetShownNumber() const;

    void SetNumber( const wxString& aNumber ) { m_number = aNumber; }

    ELECTRICAL_PINTYPE GetType() const;

    wxString GetCanonicalElectricalTypeName() const
    {
        return LIB_PIN::GetCanonicalElectricalTypeName( GetType() );
    }

    GRAPHIC_PINSHAPE GetShape() const;

    PIN_ORIENTATION GetOrientation() const;

    int GetLength() const;

    bool IsGlobalPower() const { return m_libPin->IsGlobalPower(); }

    bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const override;

    const wxString& GetOperatingPoint() const { return m_operatingPoint; }
    void SetOperatingPoint( const wxString& aText ) { m_operatingPoint = aText; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

private:
    LIB_PIN*       m_libPin;

    wxString       m_number;
    wxString       m_alt;
    VECTOR2I       m_position;
    bool           m_isDangling;

    /// The name that this pin connection will drive onto a net.
    std::recursive_mutex                                      m_netmap_mutex;
    std::map<const SCH_SHEET_PATH, std::pair<wxString, bool>> m_net_name_map;

    wxString       m_operatingPoint;
};

#endif
