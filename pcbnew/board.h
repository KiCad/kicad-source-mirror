/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#ifndef CLASS_BOARD_H_
#define CLASS_BOARD_H_

#include <board_item_container.h>
#include <board_stackup_manager/board_stackup.h>
#include <component_classes/component_class_manager.h>
#include <embedded_files.h>
#include <common.h> // Needed for stl hash extensions
#include <convert_shape_list_to_polygon.h> // for OUTLINE_ERROR_HANDLER
#include <hash.h>
#include <layer_ids.h>
#include <lset.h>
#include <netinfo.h>
#include <pcb_item_containers.h>
#include <pcb_plot_params.h>
#include <title_block.h>
#include <tools/pcb_selection.h>
#include <shared_mutex>
#include <project.h>
#include <list>

class BOARD_DESIGN_SETTINGS;
class BOARD_CONNECTED_ITEM;
class BOARD_COMMIT;
class DRC_RTREE;
class PCB_BASE_FRAME;
class PCB_EDIT_FRAME;
class PICKED_ITEMS_LIST;
class LENGTH_DELAY_CALCULATION;
class BOARD;
class FOOTPRINT;
class ZONE;
class PCB_TRACK;
class PAD;
class PCB_GROUP;
class PCB_GENERATOR;
class PCB_MARKER;
class MSG_PANEL_ITEM;
class NETLIST;
class REPORTER;
class SHAPE_POLY_SET;
class CONNECTIVITY_DATA;
class COMPONENT;
class PROJECT;
class PROGRESS_REPORTER;
class PCB_BOARD_OUTLINE;

namespace KIGFX
{
class RENDER_SETTINGS;
};


namespace KIFONT
{
    class OUTLINE_FONT;
}

struct ISOLATED_ISLANDS;

// The default value for m_outlinesChainingEpsilon to convert a board outlines to polygons
// It is the max dist between 2 end points to see them connected
#define DEFAULT_CHAINING_EPSILON_MM 0.01

// Forward declare endpoint from class_track.h
enum ENDPOINT_T : int;


struct PTR_PTR_CACHE_KEY
{
    BOARD_ITEM*  A;
    BOARD_ITEM*  B;

    bool operator==(const PTR_PTR_CACHE_KEY& other) const
    {
        return A == other.A && B == other.B;
    }
};

struct PTR_LAYER_CACHE_KEY
{
    BOARD_ITEM*  A;
    PCB_LAYER_ID Layer;

    bool operator==(const PTR_LAYER_CACHE_KEY& other) const
    {
        return A == other.A && Layer == other.Layer;
    }
};

struct PTR_PTR_LAYER_CACHE_KEY
{
    BOARD_ITEM*  A;
    BOARD_ITEM*  B;
    PCB_LAYER_ID Layer;

    bool operator==(const PTR_PTR_LAYER_CACHE_KEY& other) const
    {
        return A == other.A && B == other.B && Layer == other.Layer;
    }
};

struct LAYERS_CHECKED
{
    LAYERS_CHECKED() :
            layers(),
            has_error( false )
    {}

    LAYERS_CHECKED( PCB_LAYER_ID aLayer ) :
            layers( { aLayer } ),
            has_error( false )
    {}

    LSET layers;
    bool has_error;
};


namespace std
{
    template <>
    struct hash<PTR_PTR_CACHE_KEY>
    {
        std::size_t operator()( const PTR_PTR_CACHE_KEY& k ) const
        {
            std::size_t seed = 0xa82de1c0;
            hash_combine( seed, k.A, k.B );
            return seed;
        }
    };

    template <>
    struct hash<PTR_LAYER_CACHE_KEY>
    {
        std::size_t operator()( const PTR_LAYER_CACHE_KEY& k ) const
        {
            std::size_t seed = 0xa82de1c0;
            hash_combine( seed, k.A, k.Layer );
            return seed;
        }
    };

    template <>
    struct hash<PTR_PTR_LAYER_CACHE_KEY>
    {
        std::size_t operator()( const PTR_PTR_LAYER_CACHE_KEY& k ) const
        {
            std::size_t seed = 0xa82de1c0;
            hash_combine( seed, k.A, k.B, k.Layer );
            return seed;
        }
    };
}


/**
 * The allowed types of layers, same as Specctra DSN spec.
 */
enum LAYER_T
{
    LT_UNDEFINED = -1,
    LT_SIGNAL,
    LT_POWER,
    LT_MIXED,
    LT_JUMPER,
    LT_AUX,
    LT_FRONT,
    LT_BACK
};


/**
 * Container to hold information pertinent to a layer of a BOARD.
 */
struct LAYER
{
    LAYER()
    {
        clear();
    }

    void clear()
    {
        m_type    = LT_SIGNAL;
        m_visible = true;
        m_number  = 0;
        m_name.clear();
        m_userName.clear();
        m_opposite = m_number;
    }

    /*
    LAYER( const wxString& aName = wxEmptyString,
            LAYER_T aType = LT_SIGNAL, bool aVisible = true, int aNumber = -1 ) :
        m_name( aName ),
        m_type( aType ),
        m_visible( aVisible ),
        m_number( aNumber )
    {
    }
    */

    wxString    m_name;      ///< The canonical name of the layer. @see #LSET::Name
    wxString    m_userName;  ///< The user defined name of the layer.
    LAYER_T     m_type;      ///< The type of the layer. @see #LAYER_T
    bool        m_visible;
    int         m_number;    ///< The layer ID. @see PCB_LAYER_ID
    int         m_opposite;  ///< Similar layer on opposite side of the board, if any.

    /**
     * Convert a #LAYER_T enum to a string representation of the layer type.
     *
     * @param aType The #LAYER_T to convert
     * @return The string representation of the layer type.
     */
    static const char* ShowType( LAYER_T aType );

    /**
     * Convert a string to a #LAYER_T
     *
     * @param aType The layer name to convert.
     * @return The binary representation of the layer type, or
     *         LAYER_T(-1) if the string is invalid.
     */
    static LAYER_T     ParseType( const char* aType );
};


// Helper class to handle high light nets
class HIGH_LIGHT_INFO
{
protected:
    std::set<int> m_netCodes;    // net(s) selected for highlight (-1 when no net selected )
    bool          m_highLightOn; // highlight active

    void Clear()
    {
        m_netCodes.clear();
        m_highLightOn = false;
    }

    HIGH_LIGHT_INFO()
    {
        Clear();
    }

private:
    friend class BOARD;
};

/**
 * Provide an interface to hook into board modifications and get callbacks
 * on certain modifications that are made to the board.  This allows updating
 * auxiliary views other than the primary board editor view.
 */
class BOARD;

class BOARD_LISTENER
{
public:
    virtual ~BOARD_LISTENER() { }
    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) { }
    virtual void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) { }
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) { }
    virtual void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) { }
    virtual void OnBoardNetSettingsChanged( BOARD& aBoard ) { }
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) { }
    virtual void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) { }
    virtual void OnBoardHighlightNetChanged( BOARD& aBoard ) { }
    virtual void OnBoardRatsnestChanged( BOARD& aBoard ) { }
    virtual void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                         std::vector<BOARD_ITEM*>& aRemovedItems,
                                         std::vector<BOARD_ITEM*>& aChangedItems )
    {
    }
};

/**
 * Set of BOARD_ITEMs ordered by UUID.
 */
typedef std::set<BOARD_ITEM*, CompareByUuid> BOARD_ITEM_SET;

/**
 * Flags to specify how the board is being used.
 */
enum class BOARD_USE
{
    NORMAL,     // A normal board
    FPHOLDER    // A board that holds a single footprint
};


/**
 * Information pertinent to a Pcbnew printed circuit board.
 */
class BOARD : public BOARD_ITEM_CONTAINER, public EMBEDDED_FILES, public PROJECT::_ELEM
{
public:
    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_T == aItem->Type();
    }

    /**
     * Set what the board is going to be used for.
     *
     * @param aUse is the flag
     */
    void SetBoardUse( BOARD_USE aUse ) { m_boardUse = aUse; }

    /**
     * Get what the board use is.
     *
     * @return what the board is being used for
     */
    BOARD_USE GetBoardUse() const { return m_boardUse; }

    void IncrementTimeStamp();

    int GetTimeStamp() const { return m_timeStamp; }

    /**
     * Find out if the board is being used to hold a single footprint for editing/viewing.
     *
     * @return if the board is just holding a footprint
     */
    bool IsFootprintHolder() const
    {
        return m_boardUse == BOARD_USE::FPHOLDER;
    }

    void SetFileName( const wxString& aFileName ) { m_fileName = aFileName; }

    const wxString &GetFileName() const { return m_fileName; }

    const TRACKS& Tracks() const { return m_tracks; }

    const FOOTPRINTS& Footprints() const { return m_footprints; }

    const DRAWINGS& Drawings() const { return m_drawings; }

    const ZONES& Zones() const { return m_zones; }

    const GENERATORS& Generators() const { return m_generators; }

    PCB_BOARD_OUTLINE*       BoardOutline() { return m_boardOutline; }
    const PCB_BOARD_OUTLINE* BoardOutline() const { return m_boardOutline; }
    void                     UpdateBoardOutline();

    const MARKERS& Markers() const { return m_markers; }

    const PCB_POINTS& Points() const { return m_points; }

    // SWIG requires non-const accessors for some reason to make the custom iterators in board.i
    // work.  It would be good to remove this if we can figure out how to fix that.
#ifdef SWIG
    DRAWINGS& Drawings() { return m_drawings; }
    TRACKS& Tracks() { return m_tracks; }
#endif

    const BOARD_ITEM_SET GetItemSet();

    /**
     * The groups must maintain the following invariants. These are checked by
     * GroupsSanityCheck():
     *   - An item may appear in at most one group
     *   - Each group must contain at least one item
     *   - If a group specifies a name, it must be unique
     *   - The graph of groups containing subgroups must be cyclic.
     */
    const GROUPS& Groups() const { return m_groups; }

    const std::vector<BOARD_CONNECTED_ITEM*> AllConnectedItems();

    const std::map<wxString, wxString>& GetProperties() const { return m_properties; }
    void SetProperties( const std::map<wxString, wxString>& aProps ) { m_properties = aProps; }

    // Variant system
    wxString GetCurrentVariant() const { return m_currentVariant; }
    void SetCurrentVariant( const wxString& aVariant );

    const std::vector<wxString>& GetVariantNames() const { return m_variantNames; }
    void SetVariantNames( const std::vector<wxString>& aNames ) { m_variantNames = aNames; }

    bool HasVariant( const wxString& aVariantName ) const;
    void AddVariant( const wxString& aVariantName );
    void DeleteVariant( const wxString& aVariantName );
    void RenameVariant( const wxString& aOldName, const wxString& aNewName );

    wxString GetVariantDescription( const wxString& aVariantName ) const;
    void SetVariantDescription( const wxString& aVariantName, const wxString& aDescription );

    /**
     * Return the variant names for UI display.
     *
     * This returns a list suitable for populating UI controls, with the default variant
     * included and the names sorted using the SortVariantNames helper.
     *
     * @return List of variant names including the default entry.
     */
    wxArrayString GetVariantNamesForUI() const;

    void GetContextualTextVars( wxArrayString* aVars ) const;
    bool ResolveTextVar( wxString* token, int aDepth ) const;

    /// Visibility settings stored in board prior to 6.0, only used for loading legacy files
    LSET    m_LegacyVisibleLayers;
    GAL_SET m_LegacyVisibleItems;

    /// True if the legacy board design settings were loaded from a file
    bool m_LegacyDesignSettingsLoaded;
    bool m_LegacyCopperEdgeClearanceLoaded;

    /// True if netclasses were loaded from the file
    bool m_LegacyNetclassesLoaded;

    BOARD();
    ~BOARD();

    VECTOR2I       GetPosition() const override;
    void           SetPosition( const VECTOR2I& aPos ) override;
    const VECTOR2I GetFocusPosition() const override { return GetBoundingBox().GetCenter(); }

    bool IsEmpty() const;

    void Move( const VECTOR2I& aMoveVector ) override;

    void RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const override;

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad() const { return m_fileFormatVersionAtLoad; }

    void SetGenerator( const wxString& aGenerator ) { m_generator = aGenerator; }
    const wxString& GetGenerator() const { return m_generator; }

    ///< @copydoc BOARD_ITEM_CONTAINER::Add()
    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_MODE::INSERT,
              bool aSkipConnectivity = false ) override;

    ///< @copydoc BOARD_ITEM_CONTAINER::Remove()
    void Remove( BOARD_ITEM* aBoardItem, REMOVE_MODE aMode = REMOVE_MODE::NORMAL ) override;

    /**
     * An efficient way to remove all items of a certain type from the board.
     * Because of how items are stored, this method has some limitations in order to preserve
     * performance: tracks, vias, and arcs are all removed together by PCB_TRACE_T, and all graphics
     * and text object types are removed together by PCB_SHAPE_T.  If you need something more
     * granular than that, use BOARD::Remove.
     * @param aTypes is a list of one or more types to remove, or leave default to remove all
     */
    void RemoveAll( std::initializer_list<KICAD_T> aTypes = { PCB_NETINFO_T, PCB_MARKER_T,
                                                              PCB_GROUP_T, PCB_ZONE_T,
                                                              PCB_GENERATOR_T, PCB_FOOTPRINT_T,
                                                              PCB_TRACE_T, PCB_SHAPE_T } );

    bool HasItemsOnLayer( PCB_LAYER_ID aLayer );

    /**
     * Removes all owned items other than footprints existing on the given board layer, and modifies
     * the stackup for multilayer items to remove the given layer where applicable.
     * Used when removing an existing layer from the board via Board Setup or the API.
     * Caller is responsible for clearing the selection before calling this.
     * @param aLayer should be a layer enabled for this board
     * @return true if any items were removed or modified
     */
    bool RemoveAllItemsOnLayer( PCB_LAYER_ID aLayer );

    /**
     * Remove all teardrop zones with the STRUCT_DELETED flag set.  This avoids O(n^2) traversal
     * over the zone list.
     */
    void BulkRemoveStaleTeardrops( BOARD_COMMIT& aCommit );

    /**
     * Must be used if Add() is used using a BULK_x ADD_MODE to generate a change event for
     * listeners.
     */
    void FinalizeBulkAdd( std::vector<BOARD_ITEM*>& aNewItems );

    /**
     * Must be used if Remove() is used using a BULK_x REMOVE_MODE to generate a change event
     * for listeners.
     */
    void FinalizeBulkRemove( std::vector<BOARD_ITEM*>& aRemovedItems );

    /**
     * After loading a file from disk, the footprints do not yet contain the full
     * data for their embedded files, only a reference.  This iterates over all footprints
     * in the board and updates them with the full embedded data.
    */
    void FixupEmbeddedData();

    void RunOnNestedEmbeddedFiles( const std::function<void( EMBEDDED_FILES* )>& aFunction ) override;

    void CacheTriangulation( PROGRESS_REPORTER* aReporter = nullptr,
                             const std::vector<ZONE*>& aZones = {} );

    /**
     * Get the first footprint on the board or nullptr.
     *
     * This is used primarily by the footprint editor which knows there is only one.
     *
     * @return first footprint or null pointer
     */
    FOOTPRINT* GetFirstFootprint() const
    {
        return m_footprints.empty() ? nullptr : m_footprints.front();
    }

    /**
     * Remove all footprints from the deque and free the memory associated with them.
     */
    void DeleteAllFootprints();

    /**
     * Remove all footprints without deleting.  Footprint lifecycle MUST be managed elsewhere.
     */
    void DetachAllFootprints();

    /**
     * @return null if aID is null. If \a aID cannot be resolved, returns either an object of
     *         Type() == NOT_USED or null, depending on \a aAllowNullptrReturn.
     */
    BOARD_ITEM* ResolveItem( const KIID& aID, bool aAllowNullptrReturn = false ) const;

    void FillItemMap( std::map<KIID, EDA_ITEM*>& aMap );

    /**
     * Convert cross-references back and forth between ${refDes:field} and ${kiid:field}
     */
    wxString ConvertCrossReferencesToKIIDs( const wxString& aSource ) const;
    wxString ConvertKIIDsToCrossReferences( const wxString& aSource ) const;

    /**
     * Return a list of missing connections between components/tracks.
     * @return an object that contains information about missing connections.
     */
    std::shared_ptr<CONNECTIVITY_DATA> GetConnectivity() const { return m_connectivity; }

    /**
     * Build or rebuild the board connectivity database for the board,
     * especially the list of connected items, list of nets and rastnest data
     * Needed after loading a board to have the connectivity database updated.
     */
    bool BuildConnectivity( PROGRESS_REPORTER* aReporter = nullptr );

    /**
     * Delete all MARKERS from the board.
     */
    void DeleteMARKERs();

    void DeleteMARKERs( bool aWarningsAndErrors, bool aExclusions );

    PROJECT* GetProject() const { return m_project; }

    /**
     * Link a board to a given project.
     *
     * Should be called immediately after loading board in order for everything to work.
     *
     * @param aProject is a loaded project to link to.
     * @param aReferenceOnly avoids taking ownership of settings stored in project if true
     */
    void SetProject( PROJECT* aProject, bool aReferenceOnly = false );

    void ClearProject();

    /**
     * Rebuild DRC markers from the serialized data in BOARD_DESIGN_SETTINGS.
     *
     * @param aCreateMarkers if true, create markers from serialized data; if false only
     *                       use serialized data to set existing markers to excluded.
     *                       The former is used on board load; the later after a DRC.
     */
    std::vector<PCB_MARKER*> ResolveDRCExclusions( bool aCreateMarkers );

    /**
     * Scan existing markers and record data from any that are Excluded.
     */
    void RecordDRCExclusions();

    /**
     * Update the visibility flags on the current unconnected ratsnest lines.
     */
    void UpdateRatsnestExclusions();

    /**
     * Reset all high light data to the init state
     */
    void ResetNetHighLight();

    /**
     * @return the set of net codes that should be highlighted
     */
    const std::set<int>& GetHighLightNetCodes() const
    {
        return m_highLight.m_netCodes;
    }

    /**
      * Select the netcode to be highlighted.
      *
      * @param aNetCode is the net to highlight.
      * @param aMulti is true if you want to add a highlighted net without clearing the old one.
      */
    void SetHighLightNet( int aNetCode, bool aMulti = false );

    /**
     * @return true if a net is currently highlighted
     */
    bool IsHighLightNetON() const { return m_highLight.m_highLightOn; }

    /**
     * Enable or disable net highlighting.
     *
     * If a netcode >= 0 has been set with SetHighLightNet and aValue is true, the net will be
     * highlighted.  If aValue is false, net highlighting will be disabled regardless of
     * the highlight netcode being set.
     */
    void HighLightON( bool aValue = true );

    /**
     * Disable net highlight.
     */
    void HighLightOFF()
    {
        HighLightON( false );
    }

    /**
     * @return The number of copper layers in the BOARD.
     */
    int  GetCopperLayerCount() const;
    void SetCopperLayerCount( int aCount );

    int GetUserDefinedLayerCount() const;
    void SetUserDefinedLayerCount( int aCount );

    /**
     * @return The copper layer max PCB_LAYER_ID in the BOARD.
     * similar to GetCopperLayerCount(), but returns the max PCB_LAYER_ID
     */
    PCB_LAYER_ID  GetCopperLayerStackMaxId() const;

    PCB_LAYER_ID FlipLayer( PCB_LAYER_ID aLayer ) const;

    int LayerDepth( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer ) const;

    /**
     * A proxy function that calls the corresponding function in m_BoardSettings.
     *
     * @return the enabled layers in bit-mapped form.
     */
    const LSET& GetEnabledLayers() const;
    LSET GetLayerSet() const override { return GetEnabledLayers(); }

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings.
     *
     * @param aLayerMask the new bit-mask of enabled layers.
     */
    void SetEnabledLayers( const LSET& aLayerMask );
    void SetLayerSet( const LSET& aLayerMask ) override { SetEnabledLayers( aLayerMask ); }

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is enabled
     * @param aLayer = The layer to be tested
     * @return true if the layer is visible.
     */
    bool IsLayerEnabled( PCB_LAYER_ID aLayer ) const;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is visible
     *
     * @param aLayer is the layer to be tested.
     * @return  true if the layer is visible otherwise false.
     */
    bool IsLayerVisible( PCB_LAYER_ID aLayer ) const;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings.
     *
     * @return the visible layers in bit-mapped form.
     */
    const LSET& GetVisibleLayers() const;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible layers.
     *
     * @param aLayerMask is the new bit-mask of visible layers.
     */
    void SetVisibleLayers( const LSET& aLayerMask );

    // these 2 functions are not tidy at this time, since there are PCB_LAYER_IDs that
    // are not stored in the bitmap.

    /**
     * Return a set of all the element categories that are visible.
     *
     * @return the set of visible GAL layers.
     * @see enum GAL_LAYER_ID
     */
    GAL_SET GetVisibleElements() const;

    /**
     * A proxy function that calls the correspondent function in m_BoardSettings.
     *
     * @param aMask is the new bit-mask of visible element bitmap or-ed from enum GAL_LAYER_ID
     * @see enum GAL_LAYER_ID
     */
    void SetVisibleElements( const GAL_SET& aMask );

    /**
     * Change the bit-mask of visible element categories and layers.
     *
     * @see enum GAL_LAYER_ID
     */
    void SetVisibleAlls();

    /**
     * Test whether a given element category is visible.
     *
     * @param aLayer is from the enum by the same name.
     * @return true if the element is visible otherwise false.
     * @see enum GAL_LAYER_ID
     */
    bool IsElementVisible( GAL_LAYER_ID aLayer ) const;

    /**
     * Change the visibility of an element category.
     *
     * @param aLayer is from the enum by the same name.
     * @param aNewState is the new visibility state of the element category.
     * @see enum GAL_LAYER_ID
     */
    void SetElementVisibility( GAL_LAYER_ID aLayer, bool aNewState );

    /**
     * Expect either of the two layers on which a footprint can reside, and returns
     * whether that layer is visible.
     *
     * @param aLayer is one of the two allowed layers for footprints: F_Cu or B_Cu
     * @return true if the layer is visible, otherwise false.
     */
    bool IsFootprintLayerVisible( PCB_LAYER_ID aLayer ) const;

    /**
     * @return the BOARD_DESIGN_SETTINGS for this BOARD
     */
    BOARD_DESIGN_SETTINGS& GetDesignSettings() const;
    void                   SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings );

    /**
     * Invalidate the clearance cache for a specific item.
     *
     * Called by items when properties that could affect clearance change.
     *
     * @param aUuid the UUID of the item to invalidate.
     */
    void InvalidateClearanceCache( const KIID& aUuid );

    /**
     * Initialize the clearance cache for all board items.
     *
     * Pre-populates the cache to avoid delays during first render.
     */
    void InitializeClearanceCache();

    BOARD_STACKUP GetStackupOrDefault() const;

    const PAGE_INFO& GetPageSettings() const                { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    const PCB_PLOT_PARAMS& GetPlotOptions() const           { return m_plotOptions; }
    void SetPlotOptions( const PCB_PLOT_PARAMS& aOptions )  { m_plotOptions = aOptions; }

    TITLE_BLOCK& GetTitleBlock()                            { return m_titles; }
    const TITLE_BLOCK& GetTitleBlock() const                { return m_titles; }
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )    { m_titles = aTitleBlock; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    EDA_UNITS GetUserUnits()                                { return m_userUnits; }
    void SetUserUnits( EDA_UNITS aUnits )                   { m_userUnits = aUnits; }

    /**
     * Update any references within aItem (or its descendants) to the user units.  Primarily
     * for automatic-unit dimensions.
     */
    void UpdateUserUnits( BOARD_ITEM* aItem, KIGFX::VIEW* aView );

    /**
     * Extract the board outlines and build a closed polygon from lines, arcs and circle items
     * on edge cut layer.
     *
     * Any closed outline inside the main outline is a hole.  All contours should be closed,
     * i.e. have valid vertices to build a closed polygon.
     *
     * @param aOutlines is the #SHAPE_POLY_SET to fill in with outlines/holes.
     * @param aInferOutlineIfNecessary if the edges do not define a closed shape then we'll approximate the
     *                                 bounding box outline based on the edges, or failing that, any other items
     *                                 on the board
     * @param aErrorHandler is an optional DRC_ITEM error handler.
     * @param aAllowUseArcsInPolygons = an optional option to allow adding arcs in
     *  SHAPE_LINE_CHAIN polylines/polygons when building outlines from aShapeList
     *  This is mainly for export to STEP files
     * @param aIncludeNPTHAsOutlines = an optional option to include NPTH pad holes
     * in board outlines. These holes can be seen like holes created by closed shapes
     * drawn on edge cut layer inside the board main outline.
     * @return true if success, false if a contour is not valid
     */
    bool GetBoardPolygonOutlines( SHAPE_POLY_SET& aOutlines, bool aInferOutlineIfNecessary,
                                  OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr,
                                  bool aAllowUseArcsInPolygons = false, bool aIncludeNPTHAsOutlines = false );

    /**
     * @return a epsilon value that is the max distance between 2 points to see them
     * at the same coordinate when building the board outlines and tray to connect 2 end points
     * when buildind the outlines of the board
     * Too small value do not allow connecting 2 shapes (i.e. segments) not exactly connected
     * Too large value do not allow safely connecting 2 shapes like very short segments.
     */
    int GetOutlinesChainingEpsilon() { return( m_outlinesChainingEpsilon ); }
    void SetOutlinesChainingEpsilon( int aValue) { m_outlinesChainingEpsilon = aValue; }

   /**
     * Build a set of polygons which are the outlines of copper items (pads, tracks, vias, texts,
     * zones).
     *
     * Holes in vias or pads are ignored.  The polygons are not merged. This is useful to
     * export the shape of copper layers to dxf polygons or 3D viewer/
     *
     * @param aLayer is a copper layer, like B_Cu, etc.
     * @param aOutlines is the SHAPE_POLY_SET to fill in with items outline.
     * @param aRenderSettings is used to convert a SHAPE_SEGMENT when not a simple segment.
     * if not specified, a default setting will be used
     */
    void ConvertBrdLayerToPolygonalContours( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aOutlines,
                                             KIGFX::RENDER_SETTINGS* aRenderSettings = nullptr ) const;

    /**
     * Return the ID of a layer.
     */
    PCB_LAYER_ID GetLayerID( const wxString& aLayerName ) const;

    /**
     * Return the name of a \a aLayer.
     *
     * @param aLayer is the #PCB_LAYER_ID of the layer.
     * @return a string containing the name of the layer.
     */
    const wxString GetLayerName( PCB_LAYER_ID aLayer ) const;

    /**
     * Changes the name of the layer given by aLayer.
     * @param aLayer A layer, like B_Cu, etc.
     * @param aLayerName The new layer name
     * @return true if aLayerName was legal and unique among other layer names at other layer
     *         indices and aLayer was within range, else false.
     */
    bool SetLayerName( PCB_LAYER_ID aLayer, const wxString& aLayerName );

    /**
     * Return an "English Standard" name of a PCB layer when given \a aLayerNumber.
     *
     * This function is static so it can be called without a BOARD instance.  Use
     * GetLayerName() if want the layer names of a specific BOARD, which could
     * be different than the default if the user has renamed any copper layers.
     *
     * @param  aLayerId is the layer identifier (index) to fetch.
     * @return a string containing the layer name or "BAD INDEX" if aLayerId is not legal.
     */
    static wxString GetStandardLayerName( PCB_LAYER_ID aLayerId )
    {
        // a BOARD's standard layer name is the PCB_LAYER_ID fixed name
        return LayerName( aLayerId );
    }

    /**
     * Return the type of the copper layer given by aLayer.
     *
     * @param aIndex A layer index in m_Layer
     * @param aLayer A reference to a LAYER description.
     * @return false if the index was out of range.
     */
    bool SetLayerDescr( PCB_LAYER_ID aIndex, const LAYER& aLayer );

    /**
     * @return true if the layer is a front layer, or a user layer designated "Off-board, front"
     */
    bool IsFrontLayer( PCB_LAYER_ID aLayer ) const;

    /**
     * @return true if the layer is a back layer, or a user layer designated "Off-board, back"
     */
    bool IsBackLayer( PCB_LAYER_ID aLayer ) const;

    /**
     * Return the type of the copper layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @return the layer type, or LAYER_T(-1) if the index was out of range.
     */
    LAYER_T GetLayerType( PCB_LAYER_ID aLayer ) const;

    /**
     * Change the type of the layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @param aLayerType The new layer type.
     * @return true if aLayerType was legal and aLayer was within range, else false.
     */
    bool SetLayerType( PCB_LAYER_ID aLayer, LAYER_T aLayerType );

    /**
     * @param aNet Only count nodes belonging to this net.
     * @return the number of pads members of nets (i.e. with netcode > 0).
     */
    unsigned GetNodesCount( int aNet = -1 ) const;

    /**
     * Return a reference to a list of all the pads.
     *
     * The returned list is not sorted and contains pointers to PADS, but those pointers do
     * not convey ownership of the respective PADs.
     *
     * @return a full list of pads.
     */
    const std::vector<PAD*> GetPads() const;

    void BuildListOfNets()
    {
        m_NetInfo.buildListOfNets();
    }

    /**
     * Search for a net with the given netcode.
     *
     * @param aNetcode A netcode to search for.
     * @return the net if found or NULL if not found.
     */
    NETINFO_ITEM* FindNet( int aNetcode ) const;

    /**
     * Search for a net with the given name.
     *
     * @param aNetname A Netname to search for.
     * @return the net if found or NULL if not found.
     */
    NETINFO_ITEM* FindNet( const wxString& aNetname ) const;

    /**
     * Fetch the coupled netname for a given net.
     *
     * This accepts netnames suffixed by 'P', 'N', '+', '-', as well as additional numbers and
     * underscores following the suffix.  So NET_P_123 is a valid positive net name matched to
     * NET_N_123.
     *
     * @return the polarity of the given net (or 0 if it is not a diffpair net).
     */
    int MatchDpSuffix( const wxString& aNetName, wxString& aComplementNet );

    /**
     * @return the coupled net for a given net.  If not a diffpair, nullptr is returned.
     */
    NETINFO_ITEM* DpCoupledNet( const NETINFO_ITEM* aNet );

    const NETINFO_LIST& GetNetInfo() const
    {
        return m_NetInfo;
    }

    void RemoveUnusedNets( BOARD_COMMIT* aCommit )
    {
        m_NetInfo.RemoveUnusedNets( aCommit );
    }

#ifndef SWIG
    /**
     * @return iterator to the first element of the NETINFO_ITEMs list.
     */
    NETINFO_LIST::iterator BeginNets() const
    {
        return m_NetInfo.begin();
    }

    /**
     * @return iterator to the last element of the NETINFO_ITEMs list.
     */
    NETINFO_LIST::iterator EndNets() const
    {
        return m_NetInfo.end();
    }
#endif

    /**
     * @return the number of nets (NETINFO_ITEM).
     */
    unsigned GetNetCount() const
    {
        return m_NetInfo.GetNetCount();
    }

    /**
     * @return the number of PTH with Press-Fit fabr attribute
     */
    int GetPadWithPressFitAttrCount();

    /**
     * @return the number of PTH with Castellated fabr attribute
     */
    int GetPadWithCastellatedAttrCount();

    /**
     * Calculate the bounding box containing all board items (or board edge segments).
     *
     * @param aBoardEdgesOnly is true if we are interested in board edge segments only.
     * @return the board's bounding box.
     */
    BOX2I ComputeBoundingBox( bool aBoardEdgesOnly = false ) const;

    const BOX2I GetBoundingBox() const override
    {
        return ComputeBoundingBox( false );
    }

    /**
     * Return the board bounding box calculated using exclusively the board edges (graphics
     * on Edge.Cuts layer).
     *
     * If there are items outside of the area limited by Edge.Cuts graphics, the items will
     * not be taken into account.
     *
     * @return bounding box calculated using exclusively the board edges.
     */
    const BOX2I GetBoardEdgesBoundingBox() const
    {
        return ComputeBoundingBox( true );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * May be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order to process them in.
     * @return SEARCH_QUIT if the Iterator is to stop the scan, else SCAN_CONTINUE, and
     *         determined by the inspector.
     */
    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& scanTypes ) override;

    /**
     * Search for a FOOTPRINT within this board with the given reference designator.
     *
     * Finds only the first one, if there is more than one such FOOTPRINT.
     *
     * @param aReference The reference designator of the FOOTPRINT to find.
     * @return If found the FOOTPRINT having the given reference designator, else nullptr.
     */
    FOOTPRINT* FindFootprintByReference( const wxString& aReference ) const;

    /**
     * Search for a FOOTPRINT within this board with the given path.
     *
     * @param aPath The path ([sheetUUID, .., symbolUUID]) to search for.
     * @return If found, the FOOTPRINT having the given uuid, else NULL.
     */
    FOOTPRINT* FindFootprintByPath( const KIID_PATH& aPath ) const;

    /**
     * Return the set of netname candidates for netclass assignment.
     */
    std::set<wxString> GetNetClassAssignmentCandidates() const;

    /**
     * Copy NETCLASS info to each NET, based on NET membership in a NETCLASS.
     *
     * Must be called after a Design Rules edit, or after reading a netlist (or editing
     * the list of nets)  Also this function removes the non existing nets in netclasses
     * and add net nets in default netclass (this happens after reading a netlist)
     */
    void SynchronizeNetsAndNetClasses( bool aResetTrackAndViaSizes );

    /**
     * Copy component class / component class generator information from the project settings
     */
    bool SynchronizeComponentClasses( const std::unordered_set<wxString>& aNewSheetPaths ) const;

    /**
     * Copy the current project's text variables into the boards property cache.
     */
    void SynchronizeProperties();

    /**
     * Ensure that all time domain properties providers are in sync with current settings
     */
    void SynchronizeTuningProfileProperties();

    /**
     * Return the Similarity.  Because we compare board to board, we just return 1.0 here
    */
    double Similarity( const BOARD_ITEM& aOther ) const override
    {
        return 1.0;
    }

    bool operator==( const BOARD_ITEM& aOther ) const override;

    wxString GetClass() const override
    {
        return wxT( "BOARD" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif


    /*************************/
    /* Copper Areas handling */
    /*************************/

    /**
     * Set the .m_NetCode member of all copper areas, according to the area Net Name
     * The SetNetCodesFromNetNames is an equivalent to net name, for fast comparisons.
     * However the Netcode is an arbitrary equivalence, it must be set after each netlist read
     * or net change
     * Must be called after pad netcodes are calculated
     * @return : error count
     * For non copper areas, netcode is set to 0
     */
    int SetAreasNetCodesFromNetNames();

    /**
     * Return the Zone at a given index.
     *
     * @param index The array type index into a collection of ZONE *.
     * @return a pointer to the Area or NULL if index out of range.
     */
    ZONE* GetArea( int index ) const
    {
        if( (unsigned) index < m_zones.size() )
            return m_zones[index];

        return nullptr;
    }

    /**
     * @return a std::list of pointers to all board zones (possibly including zones in footprints)
     */
    std::list<ZONE*> GetZoneList( bool aIncludeZonesInFootprints = false ) const;

    /**
     * @return The number of copper pour areas or ZONEs.
     */
    int GetAreaCount() const
    {
        return static_cast<int>( m_zones.size() );
    }

    /* Functions used in test, merge and cut outlines */

    /**
     * Add an empty copper area to board areas list.
     *
     * @param aNewZonesList is a PICKED_ITEMS_LIST * where to store new areas  pickers (useful
     *                      in undo commands) can be NULL.
     * @param aNetcode is the netcode of the copper area (0 = no net).
     * @param aLayer is the layer of area.
     * @param aStartPointPosition is position of the first point of the polygon outline of this
     *        area.
     * @param aHatch is the hatch option.
     * @return a reference to the new area.
     */
    ZONE* AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode, PCB_LAYER_ID aLayer,
                   VECTOR2I aStartPointPosition, ZONE_BORDER_DISPLAY_STYLE aHatch );

    /**
     * Test for intersection of 2 copper areas.
     *
     * @param aZone1 is the area reference.
     * @param aZone2 is the area to compare for intersection calculations.
     * @return false if no intersection, true if intersection.
     */
    bool TestZoneIntersection( ZONE* aZone1, ZONE* aZone2 );

    /**
     * Find a pad \a aPosition on \a aLayer.
     *
     * @param aPosition A VECTOR2I object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a PAD object if found or NULL if not found.
     */
    PAD* GetPad( const VECTOR2I& aPosition, const LSET& aLayerMask ) const;
    PAD* GetPad( const VECTOR2I& aPosition ) const
    {
        return GetPad( aPosition, LSET().set() );
    }

    /**
     * Find a pad connected to \a aEndPoint of \a aTrace.
     *
     * @param aTrace A pointer to a PCB_TRACK object to hit test against.
     * @param aEndPoint The end point of \a aTrace the hit test against.
     * @return A pointer to a PAD object if found or NULL if not found.
     */
    PAD* GetPad( const PCB_TRACK* aTrace, ENDPOINT_T aEndPoint ) const;

    /**
     * Locate the pad connected at \a aPosition on \a aLayer starting at list position
     * \a aPad
     * <p>
     * This function uses a fast search in this sorted pad list and it is faster than
     * GetPadFast().  This list is a sorted pad list must be built before calling this
     * function.
     * </p>
     * @note The normal pad list is sorted by increasing netcodes.
     * @param aPadList is the list of pads candidates (a std::vector<PAD*>).
     * @param aPosition A VECTOR2I object containing the position to test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return a PAD object pointer to the connected pad.
     */
    PAD* GetPad( std::vector<PAD*>& aPadList, const VECTOR2I& aPosition, const LSET& aLayerMask ) const;

    /**
     * First empties then fills the vector with all pads and sorts them by increasing x
     * coordinate, and for increasing y coordinate for same values of x coordinates.  The vector
     * only holds pointers to the pads and those pointers are only references to pads which are
     * owned by the BOARD through other links.
     *
     * @param aVector Where to put the pad pointers.
     * @param aNetCode = the netcode filter:
     *                  = -1 to build the full pad list.
     *                  = a given netcode to build the pad list relative to the given net
     */
    void GetSortedPadListByXthenYCoord( std::vector<PAD*>& aVector, int aNetCode = -1 ) const;

    /**
     * Return data on the length and number of track segments connected to a given track.
     * This uses the connectivity data for the board to calculate connections
     *
     * @param aTrack Starting track (can also be a via) to check against for connection.
     * @return a tuple containing <number, length, package length, delay, package delay>
     */
    std::tuple<int, double, double, double, double> GetTrackLength( const PCB_TRACK& aTrack ) const;

    /**
     * Collect all the TRACKs and VIAs that are members of a net given by aNetCode.
     * Used from python.
     *
     * @param aNetCode gives the id of the net.
     * @return list of track which are in the net identified by @a aNetCode.
     */
    TRACKS TracksInNet( int aNetCode );

    /**
     * Get a footprint by its bounding rectangle at \a aPosition on \a aLayer.
     *
     * If more than one footprint is at \a aPosition, then the closest footprint on the
     * active layer is returned.  The distance is calculated via manhattan distance from
     * the center of the bounding rectangle to \a aPosition.
     *
     * @param aPosition A VECTOR2I object containing the position to test.
     * @param aActiveLayer Layer to test.
     * @param aVisibleOnly Search only the visible layers if true.
     * @param aIgnoreLocked Ignore locked footprints when true.
     */
    FOOTPRINT* GetFootprint( const VECTOR2I& aPosition, PCB_LAYER_ID aActiveLayer,
                             bool aVisibleOnly, bool aIgnoreLocked = false ) const;

    /**
     * Returns the maximum clearance value for any object on the board.  This considers
     * the clearances from board design settings as well as embedded clearances in footprints,
     * pads and zones.  Includes electrical, physical, hole and edge clearances.
    */
    int GetMaxClearanceValue() const;

    /**
     * Map all nets in the given board to nets with the same name (if any) in the destination
     * board.  If there are missing nets in the destination board, they will be created.
     *
     */
    void MapNets( BOARD* aDestBoard );

    void SanitizeNetcodes();

    /**
     * Add a listener to the board to receive calls whenever something on the
     * board has been modified.  The board does not take ownership of the
     * listener object.  Make sure to call RemoveListener before deleting the
     * listener object.  The order of listener invocations is not guaranteed.
     * If the specified listener object has been added before, it will not be
     * added again.
     */
    void AddListener( BOARD_LISTENER* aListener );

    /**
     * Remove the specified listener.  If it has not been added before, it
     * will do nothing.
     */
    void RemoveListener( BOARD_LISTENER* aListener );

    /**
     * Remove all listeners
     */
    void RemoveAllListeners();

    /**
      * Notify the board and its listeners that an item on the board has
      * been modified in some way.
      */
    void OnItemChanged( BOARD_ITEM* aItem );

    /**
      * Notify the board and its listeners that an item on the board has
      * been modified in some way.
      */
    void OnItemsChanged( std::vector<BOARD_ITEM*>& aItems );

    /**
      * Notify the board and its listeners that items on the board have
      * been modified in a composite operations
      */
    void OnItemsCompositeUpdate( std::vector<BOARD_ITEM*>& aAddedItems,
                                 std::vector<BOARD_ITEM*>& aRemovedItems,
                                 std::vector<BOARD_ITEM*>& aChangedItems );

    /**
     * Notify the board and its listeners that the ratsnest has been recomputed.
     */
    void OnRatsnestChanged();

    /**
     * Consistency check of internal m_groups structure.
     *
     * @param repair if true, modify groups structure until it passes the sanity check.
     * @return empty string on success.  Or error description if there's a problem.
     */
    wxString GroupsSanityCheck( bool repair = false );

    /**
     * @param repair if true, make one modification to groups structure that brings it
     *        closer to passing the sanity check.
     * @return empty string on success.  Or error description if there's a problem.
     */
    wxString GroupsSanityCheckInternal( bool repair );

    bool LegacyTeardrops() const { return m_legacyTeardrops; }
    void SetLegacyTeardrops( bool aFlag ) { m_legacyTeardrops = aFlag; }

    EMBEDDED_FILES* GetEmbeddedFiles() override;
    const EMBEDDED_FILES* GetEmbeddedFiles() const;

    void SetEmbeddedFilesDelegate( EMBEDDED_FILES* aDelegate ) { m_embeddedFilesDelegate = aDelegate; }

    /**
     * Get the list of all outline fonts used in the board
     */
    std::set<KIFONT::OUTLINE_FONT*> GetFonts() const override;

    /**
     * Finds all fonts used in the board and embeds them in the file if permissions allow
    */
    void EmbedFonts() override;

    /**
     * Returns the track length calculator
     */
    LENGTH_DELAY_CALCULATION* GetLengthCalculation() const { return m_lengthDelayCalc.get(); }

    /**
     * Gets the component class manager
     */
    COMPONENT_CLASS_MANAGER& GetComponentClassManager() { return *m_componentClassManager; }

    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::BOARD; }

    /**
     * Save board file to the .history directory.
     *
     * This method is used as a saver callback for LOCAL_HISTORY during autosave operations.
     *
     * @param aProjectPath The path to check against this board's project path
     * @param aFiles Output vector to append absolute file paths for history inclusion
     */
    void SaveToHistory( const wxString& aProjectPath, std::vector<wxString>& aFiles );

    const std::unordered_map<KIID, BOARD_ITEM*>& GetItemByIdCache() const
    {
        return m_itemByIdCache;
    }

    // --------- Item order comparators ---------

    struct cmp_items
    {
        bool operator() ( const BOARD_ITEM* aFirst, const BOARD_ITEM* aSecond ) const;
    };

    struct cmp_drawings
    {
        bool operator()( const BOARD_ITEM* aFirst, const BOARD_ITEM* aSecond ) const;
    };

public:
    // ------------ Run-time caches -------------
    mutable std::shared_mutex                             m_CachesMutex;
    std::unordered_map<PTR_PTR_CACHE_KEY, bool>           m_IntersectsCourtyardCache;
    std::unordered_map<PTR_PTR_CACHE_KEY, bool>           m_IntersectsFCourtyardCache;
    std::unordered_map<PTR_PTR_CACHE_KEY, bool>           m_IntersectsBCourtyardCache;
    std::unordered_map<PTR_PTR_LAYER_CACHE_KEY, bool>     m_IntersectsAreaCache;
    std::unordered_map<PTR_PTR_LAYER_CACHE_KEY, bool>     m_EnclosedByAreaCache;
    std::unordered_map< wxString, LSET >                  m_LayerExpressionCache;
    std::unordered_map<ZONE*, std::unique_ptr<DRC_RTREE>> m_CopperZoneRTreeCache;
    std::shared_ptr<DRC_RTREE>                            m_CopperItemRTreeCache;
    mutable std::unordered_map<const ZONE*, BOX2I>        m_ZoneBBoxCache;
    mutable std::optional<int>                            m_maxClearanceValue;

    mutable std::unordered_map<const BOARD_ITEM*, wxString> m_ItemNetclassCache;

    // ------------ DRC caches -------------
    std::vector<ZONE*>    m_DRCZones;
    std::vector<ZONE*>    m_DRCCopperZones;
    int                   m_DRCMaxClearance;
    int                   m_DRCMaxPhysicalClearance;
    ZONE*                 m_SolderMaskBridges;  // A container to build bridges on solder mask layers
    std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>> m_ZoneIsolatedIslandsMap;

private:
    // The default copy constructor & operator= are inadequate,
    // either write one or do not use it at all
    BOARD( const BOARD& aOther ) = delete;

    BOARD& operator=( const BOARD& aOther ) = delete;

    template <typename Func, typename... Args>
    void InvokeListeners( Func&& aFunc, Args&&... args )
    {
        for( auto&& l : m_listeners )
            ( l->*aFunc )( std::forward<Args>( args )... );
    }

    // Refresh user layer opposites.
    void recalcOpposites();

    friend class PCB_EDIT_FRAME;

private:
    /// the max distance between 2 end point to see them connected when building the board outlines
    int                 m_outlinesChainingEpsilon;

    /// What is this board being used for
    BOARD_USE           m_boardUse;
    int                 m_timeStamp;                // actually a modification counter

    wxString            m_fileName;

    // These containers only have const accessors and must only be modified by Add()/Remove()
    MARKERS             m_markers;
    DRAWINGS            m_drawings;
    FOOTPRINTS          m_footprints;
    TRACKS              m_tracks;
    GROUPS              m_groups;
    ZONES               m_zones;
    GENERATORS          m_generators;
    PCB_BOARD_OUTLINE*  m_boardOutline;
    PCB_POINTS          m_points;

    // Cache for fast access to items in the containers above by KIID, including children
    std::unordered_map<KIID, BOARD_ITEM*> m_itemByIdCache;

    std::map<int, LAYER> m_layers;                  // layer data

    HIGH_LIGHT_INFO     m_highLight;                // current high light data
    HIGH_LIGHT_INFO     m_highLightPrevious;        // a previously stored high light data

    int                 m_fileFormatVersionAtLoad;  // the version loaded from the file
    wxString            m_generator;                // the generator tag from the file

    std::map<wxString, wxString>        m_properties;
    std::shared_ptr<CONNECTIVITY_DATA>  m_connectivity;

    PAGE_INFO           m_paper;
    TITLE_BLOCK         m_titles;                   // text in lower right of screen and plots
    PCB_PLOT_PARAMS     m_plotOptions;
    PROJECT*            m_project;                  // project this board is a part of
    EDA_UNITS           m_userUnits;

    // Variant system
    wxString                        m_currentVariant;        // Currently active variant (empty = default)
    std::vector<wxString>           m_variantNames;          // All variant names in the board
    std::map<wxString, wxString>    m_variantDescriptions;   // Descriptions for each variant

    /**
     * All of the board design settings are stored as a JSON object inside the project file.  The
     * object itself is located here because the alternative is to require a valid project be
     * passed in when constructing a BOARD, since things in the BOARD constructor rely on access
     * to the BOARD_DESIGN_SETTINGS object.
     *
     * A reference to this object is set up in the PROJECT_FILE for the PROJECT this board is
     * part of, so that the JSON load/store operations work.  This link is established when
     * boards are loaded from disk.
     */
    std::unique_ptr<BOARD_DESIGN_SETTINGS> m_designSettings;

    /**
     * Teardrops in 7.0 were applied as a post-processing step (rather than from pad and via
     * properties).  If this flag is set, then auto-teardrop-generation will be disabled.
     */
    bool                         m_legacyTeardrops = false;

    NETINFO_LIST                 m_NetInfo;         // net info list (name, design constraints...

    std::vector<BOARD_LISTENER*> m_listeners;

    bool                         m_embedFonts;

    // Used for dummy boards, such as a footprint holder, where we don't want to make a copy
    // of all the parent's embedded data.
    EMBEDDED_FILES*              m_embeddedFilesDelegate;

    std::unique_ptr<COMPONENT_CLASS_MANAGER>  m_componentClassManager;
    std::unique_ptr<LENGTH_DELAY_CALCULATION> m_lengthDelayCalc;
};


#endif      // CLASS_BOARD_H_
