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

#pragma once

#include <pcb_generator.h>
#include <generators_mgr.h>
#include <i18n_utility.h>
#include <geometry/shape_poly_set.h>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

class ACTION_MENU;
class TOOL_INTERACTIVE;
class PCB_VIA;


enum class PCB_VIA_STITCH_LAYOUT
{
    PLAIN,      ///< Regular row/column grid.
    STAGGERED,  ///< Odd rows shifted by half the pitch.
    POISSON     ///< Tiled poisson distribution
};


enum class PCB_VIA_STITCH_MODE
{
    STITCH,     ///< Fill the outline with vias with a pattern
    GUARD       ///< Place vias to guard a net contained within
};


class PCB_VIA_STITCH : public PCB_GENERATOR_POLY
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_VIA_STITCH( BOARD_ITEM* aParent = nullptr );
    PCB_VIA_STITCH( const PCB_VIA_STITCH& aOther );
    PCB_VIA_STITCH( PCB_VIA_STITCH&& aOther ) noexcept;
    PCB_VIA_STITCH& operator=( const PCB_VIA_STITCH& aOther );
    PCB_VIA_STITCH& operator=( PCB_VIA_STITCH&& aOther ) noexcept;
    ~PCB_VIA_STITCH() override;

    wxString GetGeneratorType() const override { return wxS( "via_stitch" ); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return wxString( _( "Via Stitching" ) );
    }

    wxString GetFriendlyName() const override { return wxString( _( "Via Stitching" ) ); }

    wxString GetPluralName() const override { return wxString( _( "Via Stitching" ) ); }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

    std::vector<int> ViewGetLayers() const override
    {
        return std::vector<int>{ LAYER_VIAS, LAYER_VIA_HOLES, LAYER_ANCHOR, LAYER_VIA_STITCHING };
    }
    int GetNetCode() const;

    void        SetNetCode( int aNetCode );

    const BOX2I GetBoundingBox() const override
    {
        return m_outline.BBox();
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        return m_outline.Collide( aPosition, aAccuracy );
    }

    VECTOR2I GetPosition() const override;

    /**
     * This method is overrided to translate the stitch zone outline only on move.
     * Otherwise the default Move() implementation would move the child vias as well.
     * As the outline moves, the next Update() call will add/remove vias as needed on the stitch grid.
     */
    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_outline.Move( aMoveVector );
        m_origin += aMoveVector;
        MarkDirty();
    }

    /**
     * Fill in any unset via-template and pitch values from the board's design settings.
     */
    void InitializeDefaults( BOARD* aBoard );

    void     EditStart( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* ) override;
    bool     Update( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* ) override; // generate vias
    void     EditFinish( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* ) override;
    void     EditCancel( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* ) override;
    void Remove( GENERATOR_TOOL*, BOARD*, BOARD_COMMIT* ) override;
    wxString GetCommitMessage() const override { return _( "Stitching Update" ); }

    // Properties
    int  GetViaSize() const;
    void SetViaSize( int aVal );
    int  GetViaDrill() const;
    void SetViaDrill( int aVal );
    int  GetPitch() const { return m_pitch; }
    void SetPitch( int aVal ) { m_pitch = aVal; MarkDirty(); }
    PCB_VIA_STITCH_LAYOUT GetLayout() const { return m_layout; }

    void SetLayout( PCB_VIA_STITCH_LAYOUT aVal )
    {
        if( aVal != m_layout )
        {
            m_layout = aVal;
            ClearAllExclusions();
        }

        MarkDirty();
    }

    PCB_VIA_STITCH_MODE GetMode() const { return m_mode; }

    void SetMode( PCB_VIA_STITCH_MODE aVal )
    {
        if( aVal != m_mode )
        {
            m_mode = aVal;
            ClearAllExclusions();
        }

        MarkDirty();
    }

    /// Net being guarded in GUARD mode
    int  GetGuardedNetCode() const;
    void SetGuardedNetCode( int aNetCode );

    uint32_t  GetSeed() const { return m_seed; }
    void SetSeed( uint32_t aVal ) { m_seed = aVal; MarkDirty(); }

    PCB_VIA*       ViaTemplate() { return m_viaTemplate.get(); }
    const PCB_VIA* ViaTemplate() const { return m_viaTemplate.get(); }

    std::vector<std::pair<wxString, const BOARD_ITEM*>> GetTemplateItems() const override;
    void SetTemplateItem( const wxString& aName, std::unique_ptr<BOARD_ITEM> aItem ) override;

    /**
     * Remove a board position from the exclusion list for the current layout/mode.
     */
    void ClearExclusion( const VECTOR2I& aPos );

    /**
     * Add a board position to the exclusion set so the next Update() skips it.  The position
     * is converted to a grid cell index so the exclusion follows the grid as the offset or
     * outline changes.
     */
    void ExcludePosition( const VECTOR2I& aPos );

    /** Drop all manual exclusions. */
    void ClearAllExclusions();

    /** @return true if there is at least one exclusion that ClearAllExclusions() would drop. */
    bool HasExclusions() const { return !m_excludedCells.empty() || !m_excludedPositions.empty(); }

    const STRING_ANY_MAP GetProperties() const override;
    void SetProperties( const STRING_ANY_MAP& aProps ) override;

    EDA_ITEM* Clone() const override { return new PCB_VIA_STITCH( *this ); }

    bool ChildrenAreIndividuallySelectable() const override { return true; }

    ACTION_MENU* GetChildContextMenu( TOOL_INTERACTIVE* aTool ) const override;

    void OnZoneFillChanged( const std::vector<ZONE*>& aZones ) override;

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override;

    /// List of zones that cross out stitch zone, they'll need to be refilled for the punches
    std::vector<ZONE*> GetZonesNeedingRefillAfterUpdate() const override;

protected:
    void swapData( BOARD_ITEM* aImage ) override
    {
        wxASSERT( aImage->Type() == PCB_GENERATOR_T );
        std::swap( *this, *static_cast<PCB_VIA_STITCH*>( aImage ) );
    }

private:
    int defaultPitch( BOARD* aBoard ) const;      // grid pitch
    int defaultViaSize( BOARD* aBoard ) const;    // via diameter
    int defaultViaDrill( BOARD* aBoard ) const;   // drill diameter

    /**
     * @return true when the current layout/mode places vias on a regular grid, and so keys
     *         exclusions by (col, row) cell index rather than by absolute board position.
     *         POISSON and GUARD both place at arbitrary positions and return false.
     */
    bool usesGridCells() const
    {
        return m_layout != PCB_VIA_STITCH_LAYOUT::POISSON && m_mode != PCB_VIA_STITCH_MODE::GUARD;
    }

    /// Convert an absolute board position to a (col, row) grid cell index.
    VECTOR2I cellForPosition( const VECTOR2I& aPos ) const;

    /**
     * Convert a (col, row) grid cell index to an absolute board position, including
     * any stagger shift on odd rows.
     */
    VECTOR2I positionForCell( const VECTOR2I& aCell, int aPitch ) const;

    /**
     * Walk the placement grid, run DRC clearance tests, and return the set of (col, row)
     * cells where a via fits cleanly.  Does not consult exclusions — that's a separate
     * user-driven concern applied at consumption time.
     */
    std::set<VECTOR2I> buildPlacementCells( BOARD* aBoard ) const;

    /**
     * Grabs all zones we need to consider for via placement.
     * Namely footprints can have zones (rule areas) for keepouts under them.
     */
    static std::vector<ZONE*> collectAllZones( const BOARD* aBoard );


public:
    /**
     * Tile size for the POISSON layout, in units of pitch.  The baked pattern's normalized
     * min-distance is 1 / POISSON_TILE_PITCHES, scaled by (pitch * POISSON_TILE_PITCHES)
     * at use time → physical min-distance of `pitch`.
     */
    static constexpr int      POISSON_TILE_PITCHES = 8;

    /**
      * Fixed seed for baking the toroidal Poisson pattern.
      * This is an arbitrary value but must never be changed (it'll break the patterns used in designs).
      */
    static constexpr uint32_t POISSON_TILE_SEED = 0x542c21fc;

    /**
      * Helper method to cache the poisson tile we pattern.
      */
    static const std::vector<VECTOR2D>& bakedPoissonTile();

private:
    int                   m_pitch  = 0;                   // center-to-center spacing
    PCB_VIA_STITCH_LAYOUT m_layout = PCB_VIA_STITCH_LAYOUT::PLAIN;
    PCB_VIA_STITCH_MODE   m_mode   = PCB_VIA_STITCH_MODE::STITCH;
    uint32_t              m_seed   = 0;                   // Tile random shift seed

    std::unique_ptr<PCB_VIA> m_viaTemplate;

    wxString    m_lastNetName;
    mutable int m_netCode = 0;

    // GUARD-mode target net name
    wxString    m_lastGuardedNetName;
    mutable int m_guardedNetCode = 0;

    // Grid cells (col, row) the user has explicitly excluded, used by the grid layouts
    std::set<VECTOR2I> m_excludedCells;

    // Absolute board positions the user has explicitly excluded, used by POISSON layout and
    // GUARD mode, where vias are not placed on a grid and so have no cell index to key on.
    std::set<VECTOR2I> m_excludedPositions;

    // The grid configuration that produced the currently attached child vias.
    struct GRID_CONFIG
    {
        PCB_VIA_STITCH_LAYOUT layout = PCB_VIA_STITCH_LAYOUT::PLAIN;
        PCB_VIA_STITCH_MODE   mode   = PCB_VIA_STITCH_MODE::STITCH;
        int                   pitch  = 0;

        bool operator==( const GRID_CONFIG& aOther ) const = default;
    };

    GRID_CONFIG m_childGridConfig;

    // Offset of the placement grid relative to the global (0,0) anchor, in [0, pitch) on each
    // axis. Users can offset the grid by dragging a single stitched via
    VECTOR2I m_originOffset{ 0, 0 };

    // Whether the last Update() actually added, moved, or removed any via.
    bool m_lastUpdateChangedVias = true;

    // Vias in the current generator edit session that are pending removal.
    std::set<PCB_VIA*> m_pendingRemovals;

    // Right-click context menu for our child vias.
    mutable std::unique_ptr<ACTION_MENU> m_childContextMenu;
};