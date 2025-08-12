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

#ifndef TOOLS_ZONE_CREATE_HELPER__H_
#define TOOLS_ZONE_CREATE_HELPER__H_

#include <preview_items/polygon_geom_manager.h>
#include <preview_items/polygon_item.h>

#include <tools/drawing_tool.h>

namespace KIGFX
{
class VIEW;
}

/**
 * An adjunct helper to the DRAWING_TOOL interactive tool, which handles incoming geometry
 * changes from a #POLYGON_GEOM_MANAGER and translates that into a ZONE based on given
 * parameters.
 */
class ZONE_CREATE_HELPER : public POLYGON_GEOM_MANAGER::CLIENT
{
public:
    /**
     * Parameters used to fully describe a zone creation process
     */
    struct PARAMS
    {
        ///< Should create a keepout zone?
        bool m_keepout;

        ///< Layer to begin drawing
        PCB_LAYER_ID m_layer;

        ///< The zone mode to operate in
        ZONE_MODE m_mode;

        ///< Zone settings source (for similar and cutout zones)
        ZONE* m_sourceZone;

        ///< Zone leader mode
        LEADER_MODE m_leaderMode;
    };

    /**
     * @param aTool the #DRAWING_TOOL to provide the zone tool to.
     * @param aParams the parameters to use to guide the zone creation.
     */
    ZONE_CREATE_HELPER( DRAWING_TOOL& aTool, PARAMS& aParams );

    virtual ~ZONE_CREATE_HELPER();

    ZONE* GetZone() const { return m_zone.get(); }

    /*
     * Interface for receiving #POLYGON_GEOM_MANAGER update
     */
    void OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr ) override;

    bool OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr ) override;

    void OnComplete( const POLYGON_GEOM_MANAGER& aMgr ) override;

    /**
     * Prompt the user for new zone settings, and create a new zone with those settings.
     *
     * @param aKeepout should the zone be a keepout
     * @return the new zone, can be null if the user aborted
     */
    std::unique_ptr<ZONE> createNewZone( bool aKeepout );

    /**
     * Create a new zone with the settings from an existing zone.
     *
     * @param aSrcZone the zone to copy settings from
     * @return the new zone
     */
    std::unique_ptr<ZONE> createZoneFromExisting( const ZONE& aSrcZone );

    /**
     * Cut one zone out of another one (i.e. subtraction) and update the zone..
     *
     * @param aZone the zone to removed area from
     * @param aCutout the area to remove
     */
    void performZoneCutout( ZONE& aZone, const ZONE& aCutout );

    /**
     * Commit the current zone-in-progress to the BOARD.
     *
     * This might be adding a new zone, or modifying an existing zone with a cutout, depending
     * on parameters.
     *
     * @param aZone is the drawn zone outline to commit.
     */
    void commitZone( std::unique_ptr<ZONE> aZone );

private:

    /**
     * Choose a new priority for @aZoneInfo.  This will be the lowest unused zone priority number
     * @param aZoneInfo ZONE_SETTINGS to apply the new priority number to
     */
    void setUniquePriority( ZONE_SETTINGS& aZoneInfo );

    DRAWING_TOOL& m_tool;

    ///< Parameters of the zone to be drawn
    PARAMS& m_params;

    ///< The preview item to display
    KIGFX::PREVIEW::POLYGON_ITEM m_previewItem;

    ///< view that show the preview item
    KIGFX::VIEW& m_parentView;

    ///< The zone-in-progress
    std::unique_ptr<ZONE> m_zone;
};

#endif // TOOLS_ZONE_CREATE_HELPER__H_
