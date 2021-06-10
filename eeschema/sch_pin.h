/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHOR.txt for contributors.
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
#include <widgets/msgpanel.h>

#include <mutex>
#include <map>

class SCH_SYMBOL;

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

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList ) override;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override {}

    void Move( const wxPoint& aMoveVector ) override {}

    void MirrorHorizontally( int aCenter ) override {}
    void MirrorVertically( int aCenter ) override {}
    void Rotate( const wxPoint& aCenter ) override {}

    wxPoint GetPosition() const override { return GetTransformedPosition(); }
    const wxPoint GetLocalPosition() const { return m_position; }
    void SetPosition( const wxPoint& aPosition ) override { m_position = aPosition; }

    const EDA_RECT GetBoundingBox() const override;
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool isDangling ) { m_isDangling = isDangling; }

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    /// @return the pin's position in global coordinates.
    wxPoint GetTransformedPosition() const;

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override;

    bool Replace( const wxFindReplaceData& aSearchData, void* aAuxData ) override;

    /*
     * While many of these are currently simply covers for the equivalent LIB_PIN methods,
     * the new Eeschema file format will soon allow us to override them at the schematic level.
     */
    bool IsVisible() const { return m_libPin->IsVisible(); }

    wxString GetName() const;

    wxString GetNumber() const { return m_number; }
    void SetNumber( const wxString& aNumber ) { m_number = aNumber; }

    ELECTRICAL_PINTYPE GetType() const;

    wxString GetCanonicalElectricalTypeName() const
    {
        return LIB_PIN::GetCanonicalElectricalTypeName( GetType() );
    }

    GRAPHIC_PINSHAPE GetShape() const;

    int GetOrientation() const;

    int GetLength() const;

    bool IsPowerConnection() const { return m_libPin->IsPowerConnection(); }

    bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const override;


#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

private:
    LIB_PIN*       m_libPin;

    wxString       m_number;
    wxString       m_alt;
    wxPoint        m_position;
    bool           m_isDangling;

    /// The name that this pin connection will drive onto a net.
    std::recursive_mutex                                      m_netmap_mutex;
    std::map<const SCH_SHEET_PATH, std::pair<wxString, bool>> m_net_name_map;
};

#endif
