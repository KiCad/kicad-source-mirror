/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <layers_id_colors_and_visibility.h>              // LAYER_COUNT, layer defs
#include <view/view.h>
#include <board_item.h>



/**
 * An abstract base class whose derivatives may be passed to a GENERAL_COLLECTOR,
 * telling GENERAL_COLLECTOR what should be collected (aside from HitTest()ing
 * and KICAD_T scanTypes[], information which are provided to the GENERAL_COLLECTOR
 * through attributes or arguments separately).
 * <p>
 * A justification for this class is to keep the structural storage details of
 * the program's "global preferences" or "configuration options" out of
 * GENERAL_COLLECTOR::Inspect().  This class carries all the necessary details
 * in with it to the Inspect() call. The constructors or other functions of
 * this class's derivatives are then the only place where knowledge of the
 * specific structure of the global preference storage is needed.  Thus,
 * GENERAL_COLLECTOR::Inspect() can be kept as simple as possible, and insulated
 * from changes in global preference storage (and even then it is
 * not simple enough).
 * </p>
 * This class introduces the notion of layer locking.
 */
class COLLECTORS_GUIDE
{

public:
    virtual     ~COLLECTORS_GUIDE() {}

    /**
     * @return bool - true if the given layer is locked, else false.
     */
    virtual     bool IsLayerLocked( PCB_LAYER_ID layer ) const = 0;

    /**
     * @return bool - true if the given layer is visible, else false.
     */
    virtual     bool IsLayerVisible( PCB_LAYER_ID layer ) const = 0;

    /**
     * @return bool - true if should ignore locked layers, else false.
     */
    virtual     bool IgnoreLockedLayers() const = 0;

    /**
     * @return bool - true if should ignore non-visible layers, else false.
     */
    virtual     bool IgnoreNonVisibleLayers() const = 0;

    /**
     * @return int - the preferred layer for HitTest()ing.
     */
    virtual     PCB_LAYER_ID GetPreferredLayer() const = 0;

    /**
     * Provide wildcard behavior regarding the preferred layer.
     *
     * @return bool - true if should ignore preferred layer, else false.
     */
    virtual     bool IgnorePreferredLayer() const = 0;

    /**
     * @return bool - true if should ignore locked items, else false.
     */
    virtual     bool IgnoreLockedItems() const = 0;

    /**
     * Determine if the secondary criteria or 2nd choice items should be included.
     *
     * @return bool - true if should include, else false.
     */
    virtual     bool IncludeSecondary() const = 0;

    /**
     * @return bool - true if footprint texts marked as "no show" should be ignored.
     */
    virtual     bool IgnoreHiddenFPText() const = 0;

    /**
     * @return bool - true if should ignore footprint text on back layers
     */
    virtual     bool IgnoreFPTextOnBack() const = 0;

    /**
     * @return bool - true if should ignore footprint text on front layers.
     */
    virtual     bool IgnoreFPTextOnFront() const = 0;

    /**
     * @return bool - true if should ignore FOOTPRINTs on Back Side.
     */
    virtual     bool IgnoreFootprintsOnBack() const = 0;

    /**
     * @return bool - ture if should ignore FOOTPRINTs on Front Side.
     */
    virtual     bool IgnoreFootprintsOnFront() const = 0;

    /**
     * @return bool - true if should ignore Pads on Back Side.
     */
    virtual     bool IgnorePadsOnBack() const = 0;

    /**
     * @return bool - ture if should ignore PADSs on Front Side.
     */
    virtual     bool IgnorePadsOnFront() const = 0;

    /**
     * @return bool - ture if should ignore through-hole PADSs.
     */
    virtual     bool IgnoreThroughHolePads() const = 0;

    /**
     * @return bool - true if should ignore PADSs on Front side and Back side.
     */
    virtual     bool IgnorePads() const
    {
        return IgnorePadsOnFront() && IgnorePadsOnBack() && IgnoreThroughHolePads();
    }

    /**
     * @return bool - true if should ignore footprint values.
     */
    virtual     bool IgnoreFPValues() const = 0;

    /**
     * @return bool - true if should ignore footprint references.
     */
    virtual     bool IgnoreFPReferences() const = 0;

    /**
     * @return true if should ignore through-hole vias
     */
    virtual     bool IgnoreThroughVias() const = 0;

    /**
     * @return true if should ignore blind/buried vias
     */
    virtual     bool IgnoreBlindBuriedVias() const = 0;

    /**
     * @return true if should ignore micro vias
     */
    virtual     bool IgnoreMicroVias() const = 0;

    /**
     * @return true if should ignore tracks
     */
    virtual     bool IgnoreTracks() const = 0;

    /**
     * @return true if should ignore the interiors of zones
     */
    virtual     bool IgnoreZoneFills() const = 0;

    virtual     double OnePixelInIU() const = 0;

    /**
     * @return bool - true if Inspect() should use BOARD_ITEM::HitTest()
     *             or false if Inspect() should use BOARD_ITEM::BoundsTest().
    virtual     bool UseHitTesting() const = 0;
     */
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
     * @return BOARD_ITEM* - or something derived from it, or NULL.
     */
    BOARD_ITEM* operator[]( int ndx ) const override
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (BOARD_ITEM*) m_list[ ndx ];

        return NULL;
    }
};


/**
 * Used when the right click button is pressed, or when the select tool is in effect.
 * This class can be used by window classes such as PCB_EDIT_FRAME.
 *
 * Philosophy: this class knows nothing of the context in which a BOARD is used
 * and that means it knows nothing about which layers are visible or current,
 * but can handle those concerns by the SetPreferredLayer() function and the
 * SetLayerSet() function.
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
    std::vector<BOARD_ITEM*>    m_List2nd;

    /**
     * Determines which items are to be collected by Inspect()
     */
    const COLLECTORS_GUIDE*     m_Guide;

    /**
     * The number of items that were originally in the primary list before the
     * m_List2nd was concatenated onto the end of it.
     */
    int                         m_PrimaryLength;

public:

    /**
     * A scan list for all editable board items
     */
    static const KICAD_T AllBoardItems[];

    /**
     * A scan list for zones outlines only
     */
    static const KICAD_T Zones[];

    /**
     * A scan list for all primary board items, omitting items which are subordinate to
     * a FOOTPRINT, such as PAD and FP_TEXT.
     */
    static const KICAD_T BoardLevelItems[];

    /**
     * A scan list for only FOOTPRINTs
     */
    static const KICAD_T Footprints[];

    /**
     * A scan list for PADs, TRACKs, or VIAs
     */
    static const KICAD_T PadsOrTracks[];

    /**
     * A scan list for primary footprint items.
     */
    static const KICAD_T FootprintItems[];

    /**
     * A scan list for only TRACKs
     */
    static const KICAD_T Tracks[];

    /**
     * A scan list for TRACKs, VIAs, FOOTPRINTs
     */
    static const KICAD_T LockableItems[];

    /**
     * A scan list for dimensions
     */
    static const KICAD_T Dimensions[];

    /**
     * A scan list for items that can be dragged
     */
    static const KICAD_T DraggableItems[];

    /**
     * Constructor GENERALCOLLECTOR
     */
    GENERAL_COLLECTOR()
    {
        m_Guide = NULL;
        m_PrimaryLength = 0;
        SetScanTypes( AllBoardItems );
    }

    void Empty2nd()
    {
        m_List2nd.clear();
    }

    void Append2nd( BOARD_ITEM* item )
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
     * @return int - The number if items which met the primary search criteria
     */
    int GetPrimaryCount() { return m_PrimaryLength; }

    /**
     * The examining function within the INSPECTOR which is passed to the Iterate function.
     *
     * Searches and collects all the objects which match the test data.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, void* testData )  override;

    /**
     * Scan a BOARD_ITEM using this class's Inspector method, which does the collection.
     *
     * @param aItem A BOARD_ITEM to scan, may be a BOARD or FOOTPRINT, or whatever.
     * @param aScanList A list of KICAD_Ts with a terminating EOT, that specs
     *  what is to be collected and the priority order of the resultant
     *  collection in "m_list".
     * @param aRefPos A wxPoint to use in hit-testing.
     * @param aGuide The COLLECTORS_GUIDE to use in collecting items.
     */
    void Collect( BOARD_ITEM* aItem, const KICAD_T aScanList[],
                 const wxPoint& aRefPos, const COLLECTORS_GUIDE& aGuide );
};


/**
 * A general implementation of a COLLECTORS_GUIDE.  One of its constructors is
 * entitled to grab information from the program's global preferences.
 */
class GENERAL_COLLECTORS_GUIDE : public COLLECTORS_GUIDE
{
private:
    // the storage architecture here is not important, since this is only
    // a carrier object and its functions are what is used, and data only indirectly.

    PCB_LAYER_ID m_preferredLayer;
    bool    m_ignorePreferredLayer;

    LSET    m_lockedLayers;                  ///< bit-mapped layer locked bits
    bool    m_ignoreLockedLayers;

    LSET    m_visibleLayers;                 ///< bit-mapped layer visible bits
    bool    m_ignoreNonVisibleLayers;

    bool    m_ignoreLockedItems;
    bool    m_includeSecondary;

    bool    m_ignoreHiddenFPText;
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

    double  m_onePixelInIU;

public:

    /**
     * Grab stuff from global preferences and uses reasonable defaults.
     *
     * Add more constructors as needed.
     *
     * @param aVisibleLayerMask = current visible layers (bit mask)
     * @param aPreferredLayer = the layer to search first
     */
    GENERAL_COLLECTORS_GUIDE( LSET aVisibleLayerMask, PCB_LAYER_ID aPreferredLayer,
                              KIGFX::VIEW* aView )
    {
        VECTOR2I one( 1, 1 );

        m_preferredLayer            = aPreferredLayer;
        m_ignorePreferredLayer      = false;
        m_visibleLayers             = aVisibleLayerMask;
        m_ignoreLockedLayers        = true;
        m_ignoreNonVisibleLayers    = true;
        m_ignoreLockedItems         = false;

#if defined(USE_MATCH_LAYER)
        m_IncludeSecondary          = false;
#else
        m_includeSecondary          = true;
#endif

        m_ignoreHiddenFPText        = true; // g_ModuleTextNOVColor;
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

        m_onePixelInIU              = abs( aView->ToWorld( one, false ).x );
    }

    /**
     * @return bool - true if the given layer is locked, else false.
     */
    bool IsLayerLocked( PCB_LAYER_ID aLayerId ) const override
    {
        return m_lockedLayers[aLayerId];
    }

    void SetLayerLocked( PCB_LAYER_ID aLayerId, bool isLocked )
    {
        m_lockedLayers.set( aLayerId, isLocked );
    }

    /**
     * @return bool - true if the given layer is visible, else false.
     */
    bool IsLayerVisible( PCB_LAYER_ID aLayerId ) const override
    {
        return m_visibleLayers[aLayerId];
    }
    void SetLayerVisible( PCB_LAYER_ID aLayerId, bool isVisible )
    {
        m_visibleLayers.set( aLayerId, isVisible );
    }
    void SetLayerVisibleBits( LSET aLayerBits ) { m_visibleLayers = aLayerBits; }

    /**
     * @return bool - true if should ignore locked layers, else false.
     */
    bool IgnoreLockedLayers() const override        { return m_ignoreLockedLayers; }
    void SetIgnoreLockedLayers( bool ignore )       { m_ignoreLockedLayers = ignore; }

    /**
     * @return bool - true if should ignore non-visible layers, else false.
     */
    bool IgnoreNonVisibleLayers() const override    { return m_ignoreNonVisibleLayers; }
    void SetIgnoreNonVisibleLayers( bool ignore )   { m_ignoreLockedLayers = ignore; }

    /**
     * @return int - the preferred layer for HitTest()ing.
     */
    PCB_LAYER_ID GetPreferredLayer() const override    { return m_preferredLayer; }
    void SetPreferredLayer( PCB_LAYER_ID aLayer )      { m_preferredLayer = aLayer; }

    /**
     * Provide wildcard behavior regarding the preferred layer.
     *
     * @return bool - true if should ignore preferred layer, else false.
     */
    bool IgnorePreferredLayer() const override      { return  m_ignorePreferredLayer; }
    void SetIgnorePreferredLayer( bool ignore )     { m_ignorePreferredLayer = ignore; }

    /**
     * @return bool - true if should ignore locked items, else false.
     */
    bool IgnoreLockedItems() const override         { return m_ignoreLockedItems; }
    void SetIgnoreLockedItems( bool ignore )        { m_ignoreLockedItems = ignore; }

    /**
     * Determine if the secondary criteria, or 2nd choice items should be included.
     *
     * @return bool - true if should include, else false.
     */
    bool IncludeSecondary() const override { return m_includeSecondary; }
    void SetIncludeSecondary( bool include ) { m_includeSecondary = include; }

    /**
     * @return bool - true if MTexts marked as "no show" should be ignored.
     */
    bool IgnoreHiddenFPText() const override { return m_ignoreHiddenFPText; }
    void SetIgnoreMTextsMarkedNoShow( bool ignore ) { m_ignoreHiddenFPText = ignore; }

    /**
     * @return bool - true if should ignore MTexts on back layers
     */
    bool IgnoreFPTextOnBack() const override { return m_ignoreFPTextOnBack; }
    void SetIgnoreMTextsOnBack( bool ignore ) { m_ignoreFPTextOnBack = ignore; }

    /**
     * @return bool - true if should ignore MTexts on front layers
     */
    bool IgnoreFPTextOnFront() const override { return m_ignoreFPTextOnFront; }
    void SetIgnoreMTextsOnFront( bool ignore ) { m_ignoreFPTextOnFront = ignore; }

    /**
     * @return bool - true if should ignore MODULEs on the back side
     */
    bool IgnoreFootprintsOnBack() const override { return m_ignoreFootprintsOnBack; }
    void SetIgnoreModulesOnBack( bool ignore ) { m_ignoreFootprintsOnBack = ignore; }

    /**
     * @return bool - true if should ignore MODULEs on component layer.
     */
    bool IgnoreFootprintsOnFront() const override { return m_ignoreFootprintsOnFront; }
    void SetIgnoreModulesOnFront( bool ignore ) { m_ignoreFootprintsOnFront = ignore; }

    /**
     * @return bool - true if should ignore Pads on Back Side.
     */
    bool IgnorePadsOnBack() const override { return m_ignorePadsOnBack; }
    void SetIgnorePadsOnBack(bool ignore) { m_ignorePadsOnBack = ignore; }

    /**
     * @return bool - true if should ignore PADSs on Front Side.
     */
    bool IgnorePadsOnFront() const override { return m_ignorePadsOnFront; }
    void SetIgnorePadsOnFront(bool ignore) { m_ignorePadsOnFront = ignore; }

    /**
     * @return bool - true if should ignore through-hole PADSs.
     */
    bool IgnoreThroughHolePads() const override { return m_ignoreThroughHolePads; }
    void SetIgnoreThroughHolePads(bool ignore) { m_ignoreThroughHolePads = ignore; }

    /**
     * @return bool - true if should ignore footprints values.
     */
    bool IgnoreFPValues() const override { return m_ignoreFPValues; }
    void SetIgnoreModulesVals(bool ignore) { m_ignoreFPValues = ignore; }

    /**
     * @return bool - true if should ignore footprints references.
     */
    bool IgnoreFPReferences() const override { return m_ignoreFPReferences; }
    void SetIgnoreModulesRefs(bool ignore) { m_ignoreFPReferences = ignore; }

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

    double OnePixelInIU() const override { return m_onePixelInIU; }
    void SetOnePixelInIU( double aValue ) { m_onePixelInIU = aValue; }
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
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, void* testData ) override;

    /**
     * Collect #BOARD_ITEM objects using this class's Inspector method, which does the collection.
     *
     * @param aBoard The BOARD_ITEM to scan.
     * @param aScanList The KICAD_Ts to gather up.
     */
    void Collect( BOARD_ITEM* aBoard, const KICAD_T aScanList[] );
};


/**
 * Collect all #BOARD_ITEM objects on a given layer.
 *
 * This only uses the primary object layer for comparison.
 */
class PCB_LAYER_COLLECTOR : public PCB_COLLECTOR
{
    PCB_LAYER_ID m_layer_id;

public:
    PCB_LAYER_COLLECTOR( PCB_LAYER_ID aLayerId = UNDEFINED_LAYER ) :
        m_layer_id( aLayerId )
    {
    }

    void SetLayerId( PCB_LAYER_ID aLayerId ) { m_layer_id = aLayerId; }

    /**
     * The examining function within the INSPECTOR which is passed to the iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, void* testData ) override;

    /**
     * Tests a BOARD_ITEM using this class's Inspector method, which does the collection.
     *
     * @param aBoard The BOARD_ITEM to scan.
     * @param aScanList The KICAD_Ts to gather up.
     */
    void Collect( BOARD_ITEM* aBoard, const KICAD_T aScanList[] );
};

#endif // COLLECTORS_H
