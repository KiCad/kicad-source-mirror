/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008-2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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

#pragma once

#include <functional>
#include <vector>
#include <utility>

#include <layer_ids.h>
#include <lset.h>
#include <widgets/wx_grid.h>
#include <zone_settings.h>

class PCB_BASE_FRAME;


class LAYER_PROPERTIES_GRID_TABLE : public WX_GRID_TABLE_BASE
{
public:
    LAYER_PROPERTIES_GRID_TABLE( PCB_BASE_FRAME* aFrame, std::function<LSET()> getLayers );
    ~LAYER_PROPERTIES_GRID_TABLE() override;

    int GetNumberRows() override { return (int) m_items.size(); }
    int GetNumberCols() override { return 3; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return _( "Layer" );
        case 1: return _( "Offset X" );
        case 2: return _( "Offset Y" );
        default: return wxEmptyString;
        }
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case 0: return aTypeName == wxGRID_VALUE_NUMBER;
        case 1: return aTypeName == wxGRID_VALUE_STRING;
        case 2: return aTypeName == wxGRID_VALUE_STRING;
        default: wxFAIL; return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int aRow, int aCol ) override;
    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    long GetValueAsLong( int aRow, int aCol ) override;
    void SetValueAsLong( int aRow, int aCol, long aValue ) override;

    void AddItem( PCB_LAYER_ID aLayer, const ZONE_LAYER_PROPERTIES& aProps );
    bool AppendRows( size_t aNumRows = 1 ) override;
    bool DeleteRows( size_t aPos, size_t aNumRows ) override;

    const std::vector<std::pair<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>>& GetItems() { return m_items; }

protected:
    void onUnitsChanged( wxCommandEvent& aEvent );

private:
    PCB_BASE_FRAME*                                             m_frame;
    std::function<LSET()>                                       m_getLayersFunc;
    std::vector<std::pair<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>> m_items;
};
