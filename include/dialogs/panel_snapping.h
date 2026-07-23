/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef PANEL_SNAPPING_H
#define PANEL_SNAPPING_H

#include <functional>
#include <vector>

#include <frame_type.h>
#include <panel_snapping_base.h>
#include <settings/snap_settings.h>

/**
 * The snapping preferences page.
 *
 * Every editor gets the same page, showing the groups its own settings support: grid snapping
 * everywhere, object snapping in the board editors, and snap inference wherever the geometry
 * tools run.  Use CreateSnappingPanel() rather than constructing this directly.
 */
class PANEL_SNAPPING : public PANEL_SNAPPING_BASE
{
public:
    /// One editor's snapping settings by value, so the page can also be filled from a settings
    /// object that isn't the live one.
    struct VALUES
    {
        int                     gridSnap = 0;
        MAGNETIC_SETTINGS       magnetics;
        SNAP_INFERENCE_SETTINGS inference;
    };

    /**
     * @param aFrameType is the editor the page belongs to, which decides the modes offered.
     * @param aGridSnap points at the live grid snapping mode.
     * @param aInference points at the live inference settings, or null if the editor has none.
     * @param aMagnetics points at the live object snapping settings, or null if the editor has none.
     * @param aDefaults yields the values the reset button restores.
     */
    PANEL_SNAPPING( wxWindow* aParent, FRAME_T aFrameType, int* aGridSnap,
                    SNAP_INFERENCE_SETTINGS* aInference, MAGNETIC_SETTINGS* aMagnetics,
                    std::function<VALUES()> aDefaults );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void load( const VALUES& aValues );

    bool                     m_routing;

    int*                     m_gridSnap;
    SNAP_INFERENCE_SETTINGS* m_inference;
    MAGNETIC_SETTINGS*       m_magnetics;
    std::function<VALUES()>  m_defaults;

    std::vector<MAGNETIC_OPTIONS> m_padOptions;
    std::vector<MAGNETIC_OPTIONS> m_trackOptions;
    std::vector<MAGNETIC_OPTIONS> m_graphicsOptions;
};


/**
 * The object snapping modes an editor offers.
 *
 * Only editors that route tracks have a mode that applies to the router alone; elsewhere the
 * choice is between never and always.
 */
std::vector<MAGNETIC_OPTIONS> MagneticSnapOptions( bool aRouting );

/**
 * The position of \a aValue in \a aOptions, falling back to the "never" entry for a value the
 * editor does not offer.  The grid helpers ignore modes an editor can't reach, so reading one
 * back as "never" matches what the user actually gets.
 */
int MagneticSnapIndex( const std::vector<MAGNETIC_OPTIONS>& aOptions, MAGNETIC_OPTIONS aValue );

/**
 * The mode at \a aIndex, or "never" if the index is out of range.
 */
MAGNETIC_OPTIONS MagneticSnapValue( const std::vector<MAGNETIC_OPTIONS>& aOptions, int aIndex );


/**
 * Build the snapping page for an editor.
 *
 * The settings members are named explicitly so that an editor which grows or loses a snapping
 * setting fails to compile rather than silently dropping a group from its preferences.
 */
template <typename SETTINGS_T>
PANEL_SNAPPING* CreateSnappingPanel( wxWindow* aParent, SETTINGS_T* aCfg, FRAME_T aFrameType,
                                     SNAP_INFERENCE_SETTINGS SETTINGS_T::* aInference = nullptr,
                                     MAGNETIC_SETTINGS SETTINGS_T::* aMagnetics = nullptr )
{
    auto valuesOf =
            [aInference, aMagnetics]( const SETTINGS_T& aSettings )
            {
                PANEL_SNAPPING::VALUES values;
                values.gridSnap = aSettings.m_Window.grid.snap;

                if( aInference )
                    values.inference = aSettings.*aInference;

                if( aMagnetics )
                    values.magnetics = aSettings.*aMagnetics;

                return values;
            };

    return new PANEL_SNAPPING( aParent, aFrameType, &aCfg->m_Window.grid.snap,
                               aInference ? &( aCfg->*aInference ) : nullptr,
                               aMagnetics ? &( aCfg->*aMagnetics ) : nullptr,
                               [valuesOf]()
                               {
                                   SETTINGS_T defaults;
                                   defaults.Load();     // Loading without a file gives the defaults
                                   return valuesOf( defaults );
                               } );
}

#endif
