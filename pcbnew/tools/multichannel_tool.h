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

#ifndef TOOLS_MULTICHANNEL_TOOL_H
#define TOOLS_MULTICHANNEL_TOOL_H

#include <map>
#include <memory>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <tools/pcb_tool_base.h>
#include <tools/pcb_picker_tool.h>
#include <connectivity/topo_match.h>

#include <reporter.h>
#include <zone_settings.h>

class wxWindow;
class wxCloseEvent;
class EDA_ITEM;
class DIALOG_MULTICHANNEL_REPEAT_LAYOUT;
class FOOTPRINT;
class PCB_GROUP;
class NETINFO_ITEM;

struct RULE_AREA;

struct RULE_AREA_COMPAT_DATA
{
    bool                      m_isOk = false;
    bool                      m_doCopy = false;
    wxString                  m_errorMsg;
    TMATCH::COMPONENT_MATCHES m_matchingComponents;
    std::vector<wxString>     m_mismatchReasons;
    /// Filled in by copyRuleAreaContents with items that were affected by the copy operation.
    std::unordered_set<BOARD_ITEM*> m_affectedItems;
    /// Filled in by copyRuleAreaContents with affected items that can be grouped together.
    std::unordered_set<BOARD_ITEM*> m_groupableItems;
    TMATCH::COMPONENT_MATCHES m_discovered;      ///< Pairing found by discovery
    std::set<FOOTPRINT*>      m_unmatched;       ///< Selected footprints the channel lacks
    bool                      m_searchLimit = false;
};

struct RULE_AREA
{
    PLACEMENT_SOURCE_T        m_sourceType = PLACEMENT_SOURCE_T::SHEETNAME;
    /// The reference of a plain selection, copied as given with its footprint area deciding what else comes along
    bool                      m_fromSelection = false;
    ZONE*                     m_zone = nullptr;
    std::set<FOOTPRINT*>      m_components;
    std::unordered_set<EDA_ITEM*> m_designBlockItems;
    PCB_GROUP*                    m_group = nullptr;
    bool                      m_generateEnabled = false;
    wxString                  m_sheetPath;
    wxString                  m_sheetName;
    wxString                  m_ruleName;
    wxString                  m_componentClass;
    wxString                  m_groupName;
    VECTOR2I                  m_center;

    /// True when the contents are scanned from the area rather than taken from an exact item list
    bool ScansContents() const { return m_sourceType != PLACEMENT_SOURCE_T::DESIGN_BLOCK || m_fromSelection; }
};


struct RULE_AREAS_DATA
{
    bool                  m_replaceExisting = false;
    bool                  m_linkAreas = false;    ///< Link the ticked targets to the reference
    RULE_AREA*            m_refRA = nullptr;
    REPEAT_LAYOUT_OPTIONS m_options;

    std::vector<RULE_AREA>                                m_areas;
    std::unordered_map<RULE_AREA*, RULE_AREA_COMPAT_DATA> m_compatMap;

    /// A plain selection is the reference, and its channels are created only when applied
    bool FromSelection() const { return !m_selectedItems.empty(); }

    std::set<BOARD_ITEM*>                    m_selectedItems;
    std::set<FOOTPRINT*>                     m_selectedFootprints;
    std::vector<std::unique_ptr<BOARD_ITEM>> m_scratchItems;    ///< Areas and groups not yet on the board
};


namespace KIGFX
{
class VIEW_OVERLAY;
}

class MULTICHANNEL_TOOL : public PCB_TOOL_BASE, public PCB_PICKER_TOOL::RECEIVER
{
public:
    MULTICHANNEL_TOOL();
    ~MULTICHANNEL_TOOL();

    int RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone );
    int RepeatLayout( const TOOL_EVENT& aEvent, RULE_AREA& aRefArea, RULE_AREA& aTargetArea,
                      REPEAT_LAYOUT_OPTIONS& aOptions, BOARD_COMMIT* aExternalCommit = nullptr,
                      wxString* aErrorOut = nullptr );
    int AutogenerateRuleAreas( const TOOL_EVENT& aEvent );

    void UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint ) override {};
    void UpdatePickedItem( const EDA_ITEM* aItem ) override;

    void ShowMismatchDetails( wxWindow* aParent, const wxString& aSummary,
                              const std::vector<wxString>& aReasons ) const;

    RULE_AREAS_DATA* GetData() { return &m_areas; }

    void GeneratePotentialRuleAreas();
    void FindExistingRuleAreas();
    int  CheckRACompatibility( ZONE* aRefZone );

    /**
     * Copy aSource onto the other areas of its link set, as one undo step.
     * Returns true if anything was copied.
     */
    bool UpdateLinkedLayouts( ZONE* aSource );

    /// Remove aZones from their link sets, and any area left alone in one, as one undo step
    void UnlinkAreas( const std::set<ZONE*>& aZones );

    /// Use plain selected items as the reference and discover the channels that repeat them
    void StartRepeatFromSelection( const std::vector<EDA_ITEM*>& aItems );
    void DiscoverRepeatCandidates( bool aLoose );

    /// The rule area zone that makes aGroup a repeated channel, or null for any other group
    static ZONE* ChannelZone( const PCB_GROUP* aGroup );

    /**
     * Grow aSeed with the parts next to it, keeping only sets that still have an exact twin elsewhere on the board.
     *
     * One step adds a layer of neighbours. The largest form repeats that until nothing more fits, after first
     * trying the whole sheet instance.
     */
    std::set<FOOTPRINT*> ExpandToMatchingTopology( const std::set<FOOTPRINT*>& aSeed, bool aLargest );

    /// Re-derive the channels after a tick or the anchor changed, then redraw the preview
    void RefreshRepeat();
    void UpdateRepeatPreview();

    /// Mark the reference parts, and the parts of aArea if given, on the board while the dialog is open
    void HighlightRepeatAreas( const RULE_AREA* aArea );

    /// Apply what the Repeat Layout dialog shows
    int ApplyRepeat();

    static wxString JoinReferences( const std::set<FOOTPRINT*>& aFootprints );

    /**
     * Remap auto-generated nets (Net-(...), unconnected-...) of a design block that was appended
     * for layout matching onto fresh private nets, so they cannot fuse by name with same-named
     * nets that belong to different parts on the board (issue 24767). Returns the created nets so
     * the caller can remove them after the temporary block is reverted.
     */
    static std::vector<NETINFO_ITEM*> IsolateDesignBlockAutoNets( BOARD*                               aBoard,
                                                                  const std::set<FOOTPRINT*>&          aFootprints,
                                                                  const std::unordered_set<EDA_ITEM*>& aItems );

private:
    bool Init() override;
    void Reset( RESET_REASON aReason ) override;
    void setTransitions() override;
    int  repeatLayout( const TOOL_EVENT& aEvent );
    int  updateLinkedLayouts( const TOOL_EVENT& aEvent );
    int  unlinkLayout( const TOOL_EVENT& aEvent );
    int  expandToMatchingTopology( const TOOL_EVENT& aEvent );
    int  expandToLargestTopology( const TOOL_EVENT& aEvent );
    int  expandSelection( bool aLargest );

    bool routerBusy() const;

    /// Link aTargets and aRefZone into one set with their own copies of the options
    void linkAreas( BOARD_COMMIT& aCommit, ZONE* aRefZone, const std::vector<RULE_AREA*>& aTargets );
    void detachArea( BOARD_COMMIT& aCommit, ZONE* aZone );
    void dropLoneLinks( BOARD_COMMIT& aCommit, const std::set<KIID>& aSets );

    /// The channels elsewhere on the board that repeat the topology of aSelected
    std::vector<RULE_AREA_COMPAT_DATA> findMatches( const std::set<FOOTPRINT*>& aSelected, bool aLoose );

    /// @return false if the user cancelled the topology check.
    bool updateSelectionCore();
    int  applySelectionRepeat();
    void clearRepeatPreview();

    /**
     * The routing of aArea on aNets, the only nets a repeat ever carries. Unless the options restrict it to the
     * area, each run of routing continues to the first segment outside of it.
     */
    std::set<BOARD_CONNECTED_ITEM*> findRepeatRouting( RULE_AREA* aArea, const std::set<int>& aNets,
                                                       const REPEAT_LAYOUT_OPTIONS& aOpts );
    void hideMovedRatsnest();
    void setPreviewHidden( bool aHide );
    void closeRepeatDialog();
    void onRepeatDialogClosed( wxCloseEvent& aEvent );
    int  applyRepeat( BOARD_COMMIT& aCommit );


    bool findComponentsInRuleArea( RULE_AREA* aRuleArea, std::set<FOOTPRINT*>& aComponents );
    bool findOtherItemsInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_ITEM*>& aItems );
    int  findRoutingInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_CONNECTED_ITEM*>& aOutput ) const;
    void copyRuleAreaContents( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                               BOARD_COMMIT* aCommit, REPEAT_LAYOUT_OPTIONS aOpts,
                               RULE_AREA_COMPAT_DATA& aCompatData );

    const SHAPE_LINE_CHAIN buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin );
    const SHAPE_LINE_CHAIN buildRAOutline( const std::set<BOARD_ITEM*>& aItems, int aMargin );

    std::set<FOOTPRINT*> queryComponentsInSheet( wxString aSheetName ) const;
    std::set<FOOTPRINT*> queryComponentsInComponentClass( const wxString& aComponentClassName ) const;
    std::set<FOOTPRINT*> queryComponentsInGroup( const wxString& aGroupName ) const;
    std::set<BOARD_ITEM*> queryBoardItemsInGroup( const wxString& aGroupName ) const;
    PCB_GROUP*            findPlacementGroup( const wxString& aGroupName ) const;

    bool       resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                          RULE_AREA_COMPAT_DATA& aMatches,
                                          const TMATCH::ISOMORPHISM_PARAMS& aParams = {},
                                          const std::unordered_set<int>& aExtraGlobalNets = {} );

    struct TOPOLOGY_JOB
    {
        RULE_AREA*             m_ref;
        RULE_AREA*             m_target;
        RULE_AREA_COMPAT_DATA* m_compat;
    };

    /**
     * Resolve the topology of each job in turn on a worker thread.
     *
     * In the GUI a progress dialog titled @p aTitle shows how far the matching got, and the board
     * and the Repeat dialog are disabled until it closes.  Jobs not finished when the user cancels
     * are marked as not OK.
     *
     * @return false if the user cancelled.
     */
    bool       resolveTopologies( const wxString& aTitle, const std::vector<TOPOLOGY_JOB>& aJobs );
    void       fixupNet( BOARD_CONNECTED_ITEM* aRef, BOARD_CONNECTED_ITEM* aTarget,
                         bool aHasMatches,
                         const std::map<int, int>& aNetMap );
    bool       pruneExistingGroups( COMMIT& aCommit, const std::unordered_set<BOARD_ITEM*>& aItemsToCheck );

    std::unique_ptr<REPORTER> m_reporter;
    RULE_AREAS_DATA           m_areas;
    DIALOG_MULTICHANNEL_REPEAT_LAYOUT* m_dialog = nullptr;
    std::vector<KIID>         m_previewHidden;
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_highlight;
    const RULE_AREA*          m_focusArea = nullptr;
    std::map<FOOTPRINT*, BOX2I> m_previewBoxes;
    std::vector<SEG>          m_previewRatsnest;
};


#endif // TOOLS_MULTICHANNEL_TOOL
