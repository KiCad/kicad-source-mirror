/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef COLLECTORS_H
#define COLLECTORS_H

/*
 *  This module contains a number of COLLECTOR implementations which are used
 *  to augment the functionality of class PCB_EDIT_FRAME.
 */

#include <collector.h>
#include <layer_ids.h>              // LAYER_COUNT, layer defs
#include <lset.h>
#include <view/view.h>
#include <board_item.h>

/**
 * An abstract base class whose derivatives may be passed to a GENERAL_COLLECTOR telling it what
 * should be collected (aside from HitTest()ing and KICAD_T scanTypes, which are provided to the
 * GENERAL_COLLECTOR through attributes or arguments separately).
 * <p>
 * A justification for this class is to keep the structural storage details of the program's
 * "configuration options" out of GENERAL_COLLECTOR::Inspect().  This class carries all the
 * necessary details with it into the Inspect() call. The constructors or other functions of this
 * class's derivatives are then the only place where knowledge of the specific structure of the
 * global preference storage is needed.  Thus, GENERAL_COLLECTOR::Inspect() can be kept as simple
 * as possible, and insulated from changes in global preference storage.
 * </p>
 * This class introduces the notion of layer locking.
 */
class COLLECTORS_GUIDE
{
public:
    virtual ~COLLECTORS_GUIDE() {}

    /**
     * @return true if the given layer is visible, else false.
     */
    virtual bool IsLayerVisible( PCB_LAYER_ID layer ) const = 0;

    /**
     * @return the preferred layer for HitTest()ing.
     */
    virtual PCB_LAYER_ID GetPreferredLayer() const = 0;

    /**
     * @return true if should ignore locked items, else false.
     */
    virtual bool IgnoreLockedItems() const = 0;

    /**
     * Determine if the secondary criteria or 2nd choice items should be included.
     *
     * @return true if should include, else false.
     */
    virtual bool IncludeSecondary() const = 0;

    /**
     * @return true if should ignore footprint text on back layers
     */
    virtual bool IgnoreFPTextOnBack() const = 0;

    /**
     * @return true if should ignore footprint text on front layers.
     */
    virtual bool IgnoreFPTextOnFront() const = 0;

    /**
     * @return true if should ignore FOOTPRINTs on Back Side.
     */
    virtual bool IgnoreFootprintsOnBack() const = 0;

    /**
     * @return true if should ignore FOOTPRINTs on Front Side.
     */
    virtual bool IgnoreFootprintsOnFront() const = 0;

    /**
     * @return true if should ignore Pads on Back Side.
     */
    virtual bool IgnorePadsOnBack() const = 0;

    /**
     * @return true if should ignore PADSs on Front Side.
     */
    virtual bool IgnorePadsOnFront() const = 0;

    /**
     * @return true if should ignore through-hole PADSs.
     */
    virtual bool IgnoreThroughHolePads() const = 0;

    /**
     * @return true if should ignore PADSs on Front side and Back side.
     */
    virtual bool IgnorePads() const
    {
        return IgnorePadsOnFront() && IgnorePadsOnBack() && IgnoreThroughHolePads();
    }

    /**
     * @return true if should ignore footprint values.
     */
    virtual bool IgnoreFPValues() const = 0;

    /**
     * @return true if should ignore footprint references.
     */
    virtual bool IgnoreFPReferences() const = 0;

    /**
     * @return true if should ignore through-hole vias
     */
    virtual bool IgnoreThroughVias() const = 0;

    /**
     * @return true if should ignore blind/buried vias
     */
    virtual bool IgnoreBlindBuriedVias() const = 0;

    /**
     * @return true if should ignore micro vias
     */
    virtual bool IgnoreMicroVias() const = 0;

    /**
     * @return true if should ignore tracks
     */
    virtual bool IgnoreTracks() const = 0;

    /**
     * @return true if should ignore the interiors of zones
     */
    virtual bool IgnoreZoneFills() const = 0;

    /**
     * @return true if should ignore items with no net.
     */
    virtual bool IgnoreNoNets() const = 0;

    virtual int Accuracy() const = 0;

    virtual double OnePixelInIU() const = 0;
};



/**
 * Collect #BOARD_ITEM objects.
 *
 * All this object really does is override the [] operator and return a #BOARD_ITEM instead
 * of a #EDA_ITEM.  Derive all board collector objects from this class instead of the base
 * #COLLECTOR object.
 *
 * @see class COLLECTOR
 */
class PCB_COLLECTOR : public COLLECTOR
{
public:
    /**
     * Overload the COLLECTOR::operator[](int) to return a #BOARD_ITEM instead of an #EDA_ITEM.
     *
     * @param ndx The index into the list.
     * @return a board item or NULL.
     */
    BOARD_ITEM* operator[]( int ndx ) const override
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (BOARD_ITEM*) m_list[ ndx ];

        return nullptr;
    }
};


/**
 * Used when the right click button is pressed, or when the select tool is in effect.
 * This class can be used by window classes such as PCB_EDIT_FRAME.
 *
 * Philosophy: this class knows nothing of the context in which a BOARD is used and that means
 * it knows nothing about which layers are visible or current, but can handle those concerns by
 * the SetPreferredLayer() function and the SetLayerSet() function.
 */
class GENERAL_COLLECTOR : public PCB_COLLECTOR
{
protected:
    /**
     * A place to hold collected objects which don't match precisely the search
     * criteria, but would be acceptable if nothing else is found.
     * "2nd" choice, which will be appended to the end of COLLECTOR's prime
     * "list" at the end of the search.
     */
    std::vector<EDA_ITEM*>      m_List2nd;

    /**
     * Determine which items are to be collected by Inspect().
     */
    const COLLECTORS_GUIDE*     m_Guide;

public:

    /**
     * A scan list for all editable board items
     */
    static const std::vector<KICAD_T> AllBoardItems;

    /**
     * A scan list for zones outlines only
     */
    static const std::vector<KICAD_T> Zones;

    /**
     * A scan list for all primary board items, omitting items which are subordinate to
     * a FOOTPRINT, such as PAD and PCB_TEXT.
     */
    static const std::vector<KICAD_T> BoardLevelItems;

    /**
     * A scan list for only FOOTPRINTs
     */
    static const std::vector<KICAD_T> Footprints;

    /**
     * A scan list for PADs, TRACKs, or VIAs
     */
    static const std::vector<KICAD_T> PadsOrTracks;

    /**
     * A scan list for primary footprint items.
     */
    static const std::vector<KICAD_T> FootprintItems;

    /**
     * A scan list for only TRACKs and ARCs
     */
    static const std::vector<KICAD_T> Tracks;

    /**
     * A scan list for dimensions
     */
    static const std::vector<KICAD_T> Dimensions;

    /**
     * A scan list for items that can be dragged
     */
    static const std::vector<KICAD_T> DraggableItems;

    GENERAL_COLLECTOR() :
            m_Guide( nullptr )
    {
        SetScanTypes( AllBoardItems );
    }

    void Empty2nd()
    {
        m_List2nd.clear();
    }

    void Append2nd( EDA_ITEM* item )
    {
        m_List2nd.push_back( item );
    }

    /**
     * Record which COLLECTORS_GUIDE to use.
     *
     * @param aGuide Which guide to use in the collection.
     */
    void SetGuide( const COLLECTORS_GUIDE* aGuide ) { m_Guide = aGuide; }

    const COLLECTORS_GUIDE* GetGuide() const { return m_Guide; }

    /**
     * The examining function within the INSPECTOR which is passed to the Iterate function.
     * Search and collect all the objects which match the test data.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param aTestData is not used in this class.
     * @return SEARCH_QUIT if the Iterator is to stop the scan, else SCAN_CONTINUE
     */
    INSPECT_RESULT Inspect( EDA_ITEM* aTestItem, void* aTestData )  override;

    /**
     * Scan a BOARD_ITEM using this class's Inspector method, which does the collection.
     *
     * @param aItem A BOARD_ITEM to scan, may be a BOARD or FOOTPRINT, or whatever.
     * @param aScanList A list of KICAD_Ts that specs what is to be collected and the priority
     *                  order of the resultant collection in "m_list".
     * @param aRefPos A wxPoint to use in hit-testing.
     * @param aGuide The COLLECTORS_GUIDE to use in collecting items.
     */
    void Collect( BOARD_ITEM* aItem, const std::vector<KICAD_T>& aScanList,
                  const VECTOR2I& aRefPos, const COLLECTORS_GUIDE& aGuide );
};


/**
 * A general implementation of a COLLECTORS_GUIDE.  One of its constructors is
 * entitled to grab information from the program's global preferences.
 */
class GENERAL_COLLECTORS_GUIDE : public COLLECTORS_GUIDE
{
public:

    /**
     * Grab stuff from global preferences and uses reasonable defaults.
     *
     * Add more constructors as needed.
     *
     * @param aVisibleLayerMask is the current visible layers (bit mask).
     * @param aPreferredLayer is the layer to search first.
     */
    GENERAL_COLLECTORS_GUIDE( const LSET& aVisibleLayerMask, PCB_LAYER_ID aPreferredLayer,
                              KIGFX::VIEW* aView )
    {
        static const VECTOR2I one( 1, 1 );

        m_preferredLayer            = aPreferredLayer;
        m_visibleLayers             = aVisibleLayerMask;
        m_ignoreLockedItems         = false;

#if defined(USE_MATCH_LAYER)
        m_includeSecondary          = false;
#else
        m_includeSecondary          = true;
#endif

        m_ignoreFPTextOnBack        = true;
        m_ignoreFPTextOnFront       = false;
        m_ignoreFootprintsOnBack    = true; // !Show_footprints_Cmp;
        m_ignoreFootprintsOnFront   = false;

        m_ignorePadsOnFront         = false;
        m_ignorePadsOnBack          = false;
        m_ignoreThroughHolePads     = false;

        m_ignoreFPValues            = false;
        m_ignoreFPReferences        = false;

        m_ignoreThroughVias         = false;
        m_ignoreBlindBuriedVias     = false;
        m_ignoreMicroVias           = false;
        m_ignoreTracks              = false;
        m_ignoreZoneFills           = true;
        m_ignoreNoNets              = false;

        m_onePixelInIU = abs( aView->ToWorld( one, false ).x );
        m_accuracy = KiROUND( 5 * m_onePixelInIU );
    }

    /**
     * @return true if the given layer is visible, else false.
     */
    bool IsLayerVisible( PCB_LAYER_ID aLayerId ) const override
    {
        return m_visibleLayers[aLayerId];
    }
    void SetLayerVisible( PCB_LAYER_ID aLayerId, bool isVisible )
    {
        m_visibleLayers.set( aLayerId, isVisible );
    }
    void SetLayerVisibleBits( const LSET& aLayerBits ) { m_visibleLayers = aLayerBits; }

    /**
     * @return int - the preferred layer for HitTest()ing.
     */
    PCB_LAYER_ID GetPreferredLayer() const override    { return m_preferredLayer; }
    void SetPreferredLayer( PCB_LAYER_ID aLayer )      { m_preferredLayer = aLayer; }

    /**
     * @return true if should ignore locked items, else false.
     */
    bool IgnoreLockedItems() const override         { return m_ignoreLockedItems; }
    void SetIgnoreLockedItems( bool ignore )        { m_ignoreLockedItems = ignore; }

    /**
     * Determine if the secondary criteria, or 2nd choice items should be included.
     *
     * @return true if should include, else false.
     */
    bool IncludeSecondary() const override { return m_includeSecondary; }
    void SetIncludeSecondary( bool include ) { m_includeSecondary = include; }

    /**
     * @return true if should ignore Footprint Text on back layers
     */
    bool IgnoreFPTextOnBack() const override { return m_ignoreFPTextOnBack; }
    void SetIgnoreFPTextOnBack( bool ignore ) { m_ignoreFPTextOnBack = ignore; }

    /**
     * @return true if should ignore Footprint Text on front layers
     */
    bool IgnoreFPTextOnFront() const override { return m_ignoreFPTextOnFront; }
    void SetIgnoreFPTextOnFront( bool ignore ) { m_ignoreFPTextOnFront = ignore; }

    /**
     * @return true if should ignore Footprints on the back side.
     */
    bool IgnoreFootprintsOnBack() const override { return m_ignoreFootprintsOnBack; }
    void SetIgnoreFootprintsOnBack( bool ignore ) { m_ignoreFootprintsOnBack = ignore; }

    /**
     * @return true if should ignore Footprints on the front side.
     */
    bool IgnoreFootprintsOnFront() const override { return m_ignoreFootprintsOnFront; }
    void SetIgnoreFootprintsOnFront( bool ignore ) { m_ignoreFootprintsOnFront = ignore; }

    /**
     * @return true if should ignore pads on back side.
     */
    bool IgnorePadsOnBack() const override { return m_ignorePadsOnBack; }
    void SetIgnorePadsOnBack(bool ignore) { m_ignorePadsOnBack = ignore; }

    /**
     * @return true if should ignore pads on front side.
     */
    bool IgnorePadsOnFront() const override { return m_ignorePadsOnFront; }
    void SetIgnorePadsOnFront(bool ignore) { m_ignorePadsOnFront = ignore; }

    /**
     * @return true if should ignore through-hole pads.
     */
    bool IgnoreThroughHolePads() const override { return m_ignoreThroughHolePads; }
    void SetIgnoreThroughHolePads(bool ignore) { m_ignoreThroughHolePads = ignore; }

    /**
     * @return true if should ignore footprints values.
     */
    bool IgnoreFPValues() const override { return m_ignoreFPValues; }
    void SetIgnoreFPValues( bool ignore) { m_ignoreFPValues = ignore; }

    /**
     * @return true if should ignore footprints references.
     */
    bool IgnoreFPReferences() const override { return m_ignoreFPReferences; }
    void SetIgnoreFPReferences( bool ignore) { m_ignoreFPReferences = ignore; }

    bool IgnoreThroughVias() const override { return m_ignoreThroughVias; }
    void SetIgnoreThroughVias( bool ignore ) { m_ignoreThroughVias = ignore; }

    bool IgnoreBlindBuriedVias() const override { return m_ignoreBlindBuriedVias; }
    void SetIgnoreBlindBuriedVias( bool ignore ) { m_ignoreBlindBuriedVias = ignore; }

    bool IgnoreMicroVias() const override { return m_ignoreMicroVias; }
    void SetIgnoreMicroVias( bool ignore ) { m_ignoreMicroVias = ignore; }

    bool IgnoreTracks() const override { return m_ignoreTracks; }
    void SetIgnoreTracks( bool ignore ) { m_ignoreTracks = ignore; }

    bool IgnoreZoneFills() const override { return m_ignoreZoneFills; }
    void SetIgnoreZoneFills( bool ignore ) { m_ignoreZoneFills = ignore; }

    bool IgnoreNoNets() const override { return m_ignoreNoNets; }
    void SetIgnoreNoNets( bool ignore ) { m_ignoreNoNets = ignore; }

    int  Accuracy() const override { return m_accuracy; }
    void SetAccuracy( int aValue ) { m_accuracy = aValue; }

    double OnePixelInIU() const override { return m_onePixelInIU; }

private:
    // the storage architecture here is not important, since this is only
    // a carrier object and its functions are what is used, and data only indirectly.

    PCB_LAYER_ID m_preferredLayer;

    LSET    m_visibleLayers;                 ///< bit-mapped layer visible bits

    bool    m_ignoreLockedItems;
    bool    m_includeSecondary;

    bool    m_ignoreFPTextOnBack;
    bool    m_ignoreFPTextOnFront;
    bool    m_ignoreFootprintsOnBack;
    bool    m_ignoreFootprintsOnFront;
    bool    m_ignorePadsOnFront;
    bool    m_ignorePadsOnBack;
    bool    m_ignoreThroughHolePads;
    bool    m_ignoreFPValues;
    bool    m_ignoreFPReferences;
    bool    m_ignoreThroughVias;
    bool    m_ignoreBlindBuriedVias;
    bool    m_ignoreMicroVias;
    bool    m_ignoreTracks;
    bool    m_ignoreZoneFills;
    bool    m_ignoreNoNets;

    double m_onePixelInIU;
    int    m_accuracy;
};


/**
 * Collect all #BOARD_ITEM objects of a given set of #KICAD_T type(s).
 *
 * @see class COLLECTOR
 */
class PCB_TYPE_COLLECTOR : public PCB_COLLECTOR
{
public:

    /**
     * The examining function within the INSPECTOR which is passed to the Iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_QUIT if the Iterator is to stop the scan, else SCAN_CONTINUE
     */
    INSPECT_RESULT Inspect( EDA_ITEM* testItem, void* testData ) override;

    /**
     * Collect #BOARD_ITEM objects using this class's Inspector method, which does the collection.
     *
     * @param aBoard The BOARD_ITEM to scan.
     * @param aTypes The KICAD_Ts to gather up.
     */
    void Collect( BOARD_ITEM* aBoard, const std::vector<KICAD_T>& aTypes );
};


/**
 * Collect all #BOARD_ITEM objects on a given layer.
 *
 * This only uses the primary object layer for comparison.
 */
class PCB_LAYER_COLLECTOR : public PCB_COLLECTOR
{
public:
    PCB_LAYER_COLLECTOR( PCB_LAYER_ID aLayerId = UNDEFINED_LAYER ) :
        m_layer_id( aLayerId )
    { }

    void SetLayerId( PCB_LAYER_ID aLayerId ) { m_layer_id = aLayerId; }

    /**
     * The examining function within the INSPECTOR which is passed to the iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_QUIT if the Iterator is to stop the scan, else SCAN_CONTINUE
     */
    INSPECT_RESULT Inspect( EDA_ITEM* testItem, void* testData ) override;

    /**
     * Test a BOARD_ITEM using this class's Inspector method, which does the collection.
     *
     * @param aBoard The BOARD_ITEM to scan.
     * @param aTypes The KICAD_Ts to gather up.
     */
    void Collect( BOARD_ITEM* aBoard, const std::vector<KICAD_T>& aTypes );

private:
    PCB_LAYER_ID m_layer_id;
};

#endif // COLLECTORS_H
