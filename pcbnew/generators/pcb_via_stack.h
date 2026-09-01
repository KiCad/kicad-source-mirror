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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_VIA_STACK_H
#define PCB_VIA_STACK_H

#include <functional>
#include <optional>
#include <set>

#include <geometry/shape_line_chain.h>
#include <pcb_generator.h>

struct VIA_STACK_PRESET;
class PCB_VIA;


enum class VIA_STACK_STYLE : int
{
    STACKED = 0,
    STAGGERED = 1
};


/**
 * A microvia stack: several single hop microvias, plus connecting traces for staggered
 * stacks, that form one vertical connection and are edited as a unit. Each hop stays an
 * ordinary PCB_VIA, bound as a group member.
 */
class PCB_VIA_STACK : public PCB_GENERATOR
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_VIA_STACK( BOARD_ITEM* aParent = nullptr, PCB_LAYER_ID aLayer = F_Cu );

    wxString GetGeneratorType() const override { return GENERATOR_TYPE; }

    // A stack's members span the whole layer range, so its own layer is the start layer and
    // must not be taken from whichever member the loader sees first.
    bool LayerFollowsMembers() const override { return false; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return _( "Microvia Stack" );
    }

    wxString GetFriendlyName() const override { return _( "Microvia Stack" ); }
    wxString GetPluralName() const override { return _( "Microvia Stacks" ); }
    wxString GetCommitMessage() const override { return _( "Edit Microvia Stack" ); }

    EDA_ITEM* Clone() const override { return new PCB_VIA_STACK( *this ); }

    // The PCB_GENERATOR base returns an empty box, which breaks click selection and the
    // view index. Use the member union like a plain group.
    const BOX2I GetBoundingBox() const override { return PCB_GROUP::GetBoundingBox(); }
    const BOX2I ViewBBox() const override { return GetBoundingBox(); }

    // The group base registers only on LAYER_ANCHOR, which VIEW::Query skips, so box
    // select and select-all would never see the stack. Register on the copper layer too.
    std::vector<int> ViewGetLayers() const override { return { LAYER_ANCHOR, GetLayer() }; }

    // Generator members are not selectable on their own, the selection tool expects the
    // generator itself to hit test. Delegate to the members.
    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        for( BOARD_ITEM* item : GetBoardItems() )
        {
            if( item->HitTest( aPosition, aAccuracy ) )
                return true;
        }

        return false;
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const override
    {
        bool anyItems = false;

        for( BOARD_ITEM* item : GetBoardItems() )
        {
            anyItems = true;

            if( aContained )
            {
                if( !item->HitTest( aRect, true, aAccuracy ) )
                    return false;
            }
            else if( item->HitTest( aRect, false, aAccuracy ) )
            {
                return true;
            }
        }

        return anyItems && aContained;
    }

    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override
    {
        bool anyItems = false;

        for( BOARD_ITEM* item : GetBoardItems() )
        {
            anyItems = true;

            if( aContained )
            {
                if( !item->HitTest( aPoly, true ) )
                    return false;
            }
            else if( item->HitTest( aPoly, false ) )
            {
                return true;
            }
        }

        return anyItems && aContained;
    }

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;
    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;
    void EditFinish( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;
    void EditCancel( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;
    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;

    // The hop polyline stores absolute positions, so it must follow every transform or a
    // later regenerate snaps the vias back to the old location.
    void Move( const VECTOR2I& aMoveVector ) override;
    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;
    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    const STRING_ANY_MAP GetProperties() const override;
    void                 SetProperties( const STRING_ANY_MAP& aProps ) override;

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override;

    void             ApplyPreset( const VIA_STACK_PRESET& aPreset );
    VIA_STACK_PRESET ToPreset() const;

    /**
     * Build a stack from existing board items (microvias plus optional connecting traces).
     * Returns a configured stack with settings inferred from the vias, or nullptr if the
     * vias do not tile a contiguous span. The caller adds it to the board and attaches
     * the items as members. When aMembers is given it receives the items that belong in
     * the stack: the vias plus only those traces that connect hop positions on a shared
     * landing layer. Other selected traces must stay loose or the next regenerate would
     * delete them.
     */
    static PCB_VIA_STACK* CreateFromItems( const std::vector<BOARD_ITEM*>& aItems, BOARD* aBoard,
                                           std::vector<BOARD_ITEM*>* aMembers = nullptr );

    /**
     * True when both span ends are copper layers within the board's copper layer count.
     * A LAYER_RANGE over an absent layer never terminates, so every span must be checked
     * before it is walked.
     */
    static bool IsSpanValid( BOARD* aBoard, PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd );

    /**
     * Replace loose microvias spanning more than one hop with stacks of single hop
     * microvias. Used after routing, which places one via per layer switch. Returns the
     * number of vias expanded. When aMatcher is given, only vias it returns a preset for
     * are expanded, and the created stack takes that preset's attributes.
     */
    static int ExpandMultiHopMicrovias( BOARD* aBoard, BOARD_COMMIT* aCommit,
                                        const std::function<const VIA_STACK_PRESET*( PCB_VIA* )>& aMatcher = nullptr );

    /**
     * Ids of the vias ExpandMultiHopMicrovias would consider right now. Snapshot this before
     * routing to tell the vias a route creates from the ones already on the board.
     */
    static std::set<KIID> CollectExpandableMicrovias( BOARD* aBoard );

    /**
     * Net of the copper under a point on the given layer, pads first, then tracks, then
     * filled zones. Returns 0 when nothing is hit. Used so a placed stack inherits the
     * net of what it lands on.
     */
    static int FindNetAtPosition( BOARD* aBoard, const VECTOR2I& aPosition, PCB_LAYER_ID aLayer );

    /**
     * As above, but across every copper layer in @a aLayers. Placement snaps to copper anywhere
     * in the stack's span, so the net has to be looked for there too.
     */
    static int FindNetAtPosition( BOARD* aBoard, const VECTOR2I& aPosition, const LSET& aLayers );

    /**
     * Rebuild the member vias (and, for a staggered stack, the connecting traces) from the
     * current stack settings. Existing members are removed first. If aCommit is null the
     * changes are applied straight to the board (used by tests and non-interactive callers).
     */
    void Regenerate( BOARD* aBoard, BOARD_COMMIT* aCommit );

    /**
     * Build the member vias/traces for the current settings and return them without adding
     * them anywhere. The caller owns the items (used for regeneration and for placement
     * previews).
     */
    std::vector<BOARD_ITEM*> BuildMembers( BOARD* aBoard, int aNetCode ) const;

    // The net shared by every hop. Read from a member when present, else the stored value.
    // Setting it applies the net to every existing member as well.
    int  GetNetCode() const;
    void SetNetCode( int aNet );

    // Stack level settings.
    PCB_LAYER_ID GetStartLayer() const { return m_startLayer; }

    // The stack registers in the view on its own layer, so it tracks the start layer.
    void SetStartLayer( PCB_LAYER_ID aLayer )
    {
        m_startLayer = aLayer;
        SetLayer( aLayer );
    }

    PCB_LAYER_ID GetEndLayer() const { return m_endLayer; }
    void         SetEndLayer( PCB_LAYER_ID aLayer ) { m_endLayer = aLayer; }

    VIA_STACK_STYLE GetStyle() const { return m_style; }
    void            SetStyle( VIA_STACK_STYLE aStyle ) { m_style = aStyle; }

    int  GetPitch() const { return m_pitch; }
    void SetPitch( int aPitch ) { m_pitch = aPitch; }

    bool IsFilled() const { return m_filled; }
    void SetFilled( bool aFilled ) { m_filled = aFilled; }

    bool CanDuplicate() const override { return true; }

    bool IsCapped() const { return m_capped; }
    void SetCapped( bool aCapped ) { m_capped = aCapped; }

    int  GetViaSize() const { return m_viaSize; }
    void SetViaSize( int aSize ) { m_viaSize = aSize; }

    int  GetViaDrill() const { return m_viaDrill; }
    void SetViaDrill( int aDrill ) { m_viaDrill = aDrill; }

    bool GetUseNetclass() const { return m_useNetclass; }
    void SetUseNetclass( bool aUse ) { m_useNetclass = aUse; }

    const std::optional<SHAPE_LINE_CHAIN>& GetHops() const { return m_hops; }
    void                                   SetHops( const SHAPE_LINE_CHAIN& aHops ) { m_hops = aHops; }
    void                                   ClearHops() { m_hops.reset(); }

    const wxString& GetPresetName() const { return m_presetName; }
    void            SetPresetName( const wxString& aName ) { m_presetName = aName; }

protected:
    void swapData( BOARD_ITEM* aImage ) override
    {
        PCB_VIA_STACK* image = dynamic_cast<PCB_VIA_STACK*>( aImage );

        wxCHECK( image, /* void */ );

        std::swap( *this, *image );

        // Regenerating an edit replaces every member, so the swapped-in member set is
        // always a different one. Without this the restored vias still name the image.
        swapChildOwnership( image );
    }

    // Add a freshly built member to the board (or commit) and to this generator.
    void addMember( BOARD* aBoard, BOARD_COMMIT* aCommit, BOARD_ITEM* aItem );

    // Reshape the current members to match aRebuilt. False when the hops themselves changed,
    // leaving the caller to replace the members outright.
    bool reuseMembers( BOARD_COMMIT* aCommit, const std::vector<BOARD_ITEM*>& aRebuilt );

    PCB_LAYER_ID    m_startLayer;
    PCB_LAYER_ID    m_endLayer;
    VIA_STACK_STYLE m_style;
    int             m_pitch;  // center to center distance, staggered only
    bool            m_filled; // copper filled hops, forced on when stacked
    bool            m_capped;
    int             m_viaSize; // 0 means take from netclass / board defaults
    int             m_viaDrill;
    bool            m_useNetclass;
    int             m_netCode;    // net used when building hops, before members exist
    wxString        m_presetName; // the preset this stack was placed from, if any

    // Hop center positions for a staggered stack. Empty for a stacked (coaxial) stack.
    std::optional<SHAPE_LINE_CHAIN> m_hops;
};


DECLARE_ENUM_TO_WXANY( VIA_STACK_STYLE )

#endif // PCB_VIA_STACK_H
