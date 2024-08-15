/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef MODEL_ZONES_OVERVIEW_TABLE_H
#define MODEL_ZONES_OVERVIEW_TABLE_H

#include <memory>
#include <utility>
#include <vector>
#include <wx/dataview.h>
#include <wx/event.h>
#include <wx/string.h>
#include <board.h>
#include <zone.h>

class PCB_BASE_FRAME;
class PCB_BASE_FRAME;
class MANAGED_ZONE;

wxDECLARE_EVENT( EVT_ZONES_OVERVIEW_COUNT_CHANGE, wxCommandEvent );


enum class ZONE_INDEX_MOVEMENT
{
    MOVE_UP,
    MOVE_DOWN
};


class MODEL_ZONES_OVERVIEW_TABLE : public wxDataViewVirtualListModel
{
public:
    enum
    {
        NAME,
        NET,
        LAYERS,

        COL_COUNT
    };

    enum WIDTH_SETTING
    {
        NAME_WIDTH = 128,
        LAYER_BAR_WIDTH = 16
    };


    static std::map<int, wxString> GetColumnNames()
    {
        //NOTE - Build the column name dynamicly in case the display language changed
        const std::map<int, wxString> ColNames = std::map<int, wxString>{
            std::make_pair( NAME, _( "Name" ) ),
            std::make_pair( NET, _( "Net" ) ),
            std::make_pair( LAYERS, _( "Layers" ) )
        };
        return ColNames;
    }

    MODEL_ZONES_OVERVIEW_TABLE( std::vector<std::shared_ptr<MANAGED_ZONE>> aZones, BOARD* a_pcb,
                                PCB_BASE_FRAME* aPCB_FRAME, wxWindow* a_dialog );

    ~MODEL_ZONES_OVERVIEW_TABLE() override;

    void EnableFitterByName( bool aEnable );

    void EnableFitterByNet( bool aEnable );

    void GetValueByRow( wxVariant& aVariant, unsigned aRow, unsigned aCol ) const override;

    bool SetValueByRow( const wxVariant& aVariant, unsigned aRow, unsigned aCol ) override;

    // returns the number of rows
    unsigned int GetCount() const override;

    ZONE* GetZone( wxDataViewItem const& item ) const;

    wxDataViewItem GetItemByZone( ZONE* ) const;

    /**
     * @brief Move selected zone up/down
     *
     * @return std::optional<unsigned> the new index for selected one if success
     */
    std::optional<unsigned> MoveZoneIndex( unsigned aIndex, ZONE_INDEX_MOVEMENT aMovement );

    /**
     * @brief Swap two zone while drag && drop
     *
     * @return std::optional<unsigned>  the new index for the dragged one if success
     */
    std::optional<unsigned> SwapZonePriority( unsigned aDragIndex, unsigned aDropIndex );

    /**
     * @brief Filter the zones by the filter text
     *
     * @param aFilterText Sub text matching zone name, net name or layer name
     * @param aSelection Current selection
     * @return unsigned Selection after the filter is applied
     */
    wxDataViewItem ApplyFilter( wxString const& aFilterText, wxDataViewItem aSelection );

    /**
     * @brief Clear up the filter
     *
     * @param aSelection Current selection
     * @return unsigned
     */
    wxDataViewItem ClearFilter( wxDataViewItem aSelection );

    unsigned int GetAllZonesCount() const { return m_allZones.size(); }

private:
    void SortZoneContainers();

    void OnRowCountChange();

private:
    std::vector<std::shared_ptr<MANAGED_ZONE>> m_allZones;
    std::vector<std::shared_ptr<MANAGED_ZONE>> m_filteredZones;
    BOARD*                                     m_pcb;
    PCB_BASE_FRAME*                            m_PCB_FRAME;
    wxWindow*                                  m_dialog;
    bool                                       m_sortByName;
    bool                                       m_sortByNet;
};

#endif