/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004-2007 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file collectors.h
 */

#ifndef COLLECTORS_H
#define COLLECTORS_H


/*  This module contains a number of COLLECTOR implementations which are used
    to augment the functionality of class PCB_EDIT_FRAME.
*/


#include <class_collector.h>
#include <layers_id_colors_and_visibility.h>              // LAYER_COUNT, layer defs


class BOARD_ITEM;


/**
 * Class COLLECTORS_GUIDE
 * is an abstract base class whose derivatives may be passed to a GENERAL_COLLECTOR,
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
 * <p>
 * This class introduces the notion of layer locking.
 */
class COLLECTORS_GUIDE
{

public:
    virtual     ~COLLECTORS_GUIDE() {}

    /**
     * Function IsLayerLocked
     * @return bool - true if the given layer is locked, else false.
     */
    virtual     bool IsLayerLocked( LAYER_ID layer ) const = 0;

    /**
     * Function IsLayerVisible
     * @return bool - true if the given layer is visible, else false.
     */
    virtual     bool IsLayerVisible( LAYER_ID layer ) const = 0;

    /**
     * Function IgnoreLockedLayers
     * @return bool - true if should ignore locked layers, else false.
     */
    virtual     bool IgnoreLockedLayers() const = 0;

    /**
     * Function IgnoredNonVisibleLayers
     * @return bool - true if should ignore non-visible layers, else false.
     */
    virtual     bool IgnoreNonVisibleLayers() const = 0;

    /**
     * Function GetPreferredLayer
     * @return int - the preferred layer for HitTest()ing.
     */
    virtual     LAYER_ID GetPreferredLayer() const = 0;

    /**
     * Function IgnorePreferredLayer
     * provides wildcard behavior regarding the preferred layer.
     * @return bool - true if should ignore preferred layer, else false.
     */
    virtual     bool IgnorePreferredLayer() const = 0;

    /**
     * Function IgnoreLockedItems
     * @return bool - true if should ignore locked items, else false.
     */
    virtual     bool IgnoreLockedItems() const = 0;

    /**
     * Function IncludeSecondary
     * determines if the secondary criteria, or 2nd choice items should be
     * included.
     * @return bool - true if should include, else false.
     */
    virtual     bool IncludeSecondary() const = 0;

    /**
     * Function IgnoreMTextsMarkedNoShow
     * @return bool - true if MTexts marked as "no show" should be ignored.
     */
    virtual     bool IgnoreMTextsMarkedNoShow() const = 0;

    /**
     * Function IgnoreZones
     * @return bool - true if should ignore zones.
    virtual     bool IgnoreZones() const = 0;
     can simply omit from scanTypes[] PCB_ZONE_T */

    /**
     * Function IgnoreMTextsOnCu
     * @return bool - true if should ignore MTexts on copper layer.
     */
    virtual     bool IgnoreMTextsOnCopper() const = 0;

    /**
     * Function IgnoreMTextsOnCmp
     * @return bool - true if should ignore MTexts on component layer.
     */
    virtual     bool IgnoreMTextsOnCmp() const = 0;

    /**
     * Function IgnoreModulesOnCu
     * @return bool - true if should ignore MODULEs on Back Side.
     */
    virtual     bool IgnoreModulesOnCu() const = 0;

    /**
     * Function IgnoreModulesOnCmp
     * @return bool - ture if should ignore MODULEs on Front Side.
     */
    virtual     bool IgnoreModulesOnCmp() const = 0;

    /**
     * Function IgnorePadsOnBack
     * @return bool - true if should ignore Pads on Back Side.
     */
    virtual     bool IgnorePadsOnBack() const = 0;

    /**
     * Function IgnorePadsOnFront
     * @return bool - ture if should ignore PADSs on Front Side.
     */
    virtual     bool IgnorePadsOnFront() const = 0;

    /**
     * Function IgnorePads
     * @return bool - true if should ignore PADSs on Front side and Back side.
     */
    virtual     bool IgnorePads() const
    {
        return IgnorePadsOnFront() && IgnorePadsOnBack();
    }

    /**
     * Function IgnoreModulesVals
     * @return bool - true if should ignore modules values.
     */
    virtual     bool IgnoreModulesVals() const = 0;

    /**
     * Function IgnoreModulesRefs
     * @return bool - true if should ignore module references.
     */
    virtual     bool IgnoreModulesRefs() const = 0;

    /**
     * Function UseHitTesting
     * @return bool - true if Inspect() should use BOARD_ITEM::HitTest()
     *             or false if Inspect() should use BOARD_ITEM::BoundsTest().
    virtual     bool UseHitTesting() const = 0;
     */
};



/**
 * Class GENERAL_COLLECTOR
 * is intended for use when the right click button is pressed, or when the
 * plain "arrow" tool is in effect.  This class can be used by window classes
 * such as PCB_EDIT_FRAME.
 *
 * Philosophy: this class knows nothing of the context in which a BOARD is used
 * and that means it knows nothing about which layers are visible or current,
 * but can handle those concerns by the SetPreferredLayer() function and the
 * SetLayerSet() function.
 */
class GENERAL_COLLECTOR : public COLLECTOR
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
     * m_List2nd was concatonated onto the end of it.
     */
    int                         m_PrimaryLength;


public:

    /**
     * A scan list for all editable board items, like PcbGeneralLocateAndDisplay()
     */
    static const KICAD_T AllBoardItems[];


    /**
     * A scan list for all editable board items, except zones
     */
    static const KICAD_T AllButZones[];

    /**
     * A scan list for zones outlines only
     */
    static const KICAD_T Zones[];


    /**
     * A scan list for all primary board items, omitting items which are subordinate to
     * a MODULE, such as D_PAD and TEXTEMODULE.
    static const KICAD_T PrimaryItems[];
     */


    /**
     * A scan list for only MODULEs
     */
    static const KICAD_T ModuleItems[];


    /**
     * A scan list for PADs or MODULEs
     */
    static const KICAD_T PadsOrModules[];

    /**
     * A scan list for PADs, TRACKs, VIAs, or ZONEs
     */
    static const KICAD_T PadsTracksOrZones[];


    /**
     * A scan list for MODULEs and their items (for Modedit)
     */
    static const KICAD_T ModulesAndTheirItems[];


    /**
     * A scan list for only TRACKS
     */
    static const KICAD_T Tracks[];


    /**
     * Constructor GENERALCOLLECTOR
     */
    GENERAL_COLLECTOR()
    {
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
     * Function SetGuide
     * records which COLLECTORS_GUIDE to use.
     * @param aGuide Which guide to use in the collection.
     */
    void SetGuide( const COLLECTORS_GUIDE* aGuide ) { m_Guide = aGuide; }


    /**
     * Function operator[int]
     * overloads COLLECTOR::operator[](int) to return a BOARD_ITEM* instead of
     * an EDA_ITEM* type.
     * @param ndx The index into the list.
     * @return BOARD_ITEM* - or something derived from it, or NULL.
     */
    BOARD_ITEM* operator[]( int ndx ) const
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (BOARD_ITEM*) m_List[ ndx ];
        return NULL;
    }


    /**
     * Function GetPrimaryCount
     * @return int - The number if items which met the primary search criteria
     */
    int GetPrimaryCount() { return m_PrimaryLength; }


    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the
     * Iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, const void* testData );


    /**
     * Function Collect
     * scans a BOARD_ITEM using this class's Inspector method, which does the collection.
     * @param aItem A BOARD_ITEM to scan, may be a BOARD or MODULE, or whatever.
     * @param aScanList A list of KICAD_Ts with a terminating EOT, that specs
     *  what is to be collected and the priority order of the resultant
     *  collection in "m_List".
     * @param aRefPos A wxPoint to use in hit-testing.
     * @param aGuide The COLLECTORS_GUIDE to use in collecting items.
     */
    void Collect( BOARD_ITEM* aItem, const KICAD_T aScanList[],
                 const wxPoint& aRefPos, const COLLECTORS_GUIDE& aGuide );
};


/**
 * Class GENERAL_COLLECTORS_GUIDE
 * is a general implementation of a COLLECTORS_GUIDE.  One of its constructors is
 * entitled to grab information from the program's global preferences.
 */
class GENERAL_COLLECTORS_GUIDE : public COLLECTORS_GUIDE
{
private:
    // the storage architecture here is not important, since this is only
    // a carrier object and its functions are what is used, and data only indirectly.

    LAYER_ID m_PreferredLayer;
    bool    m_IgnorePreferredLayer;

    LSET    m_LayerLocked;                  ///< bit-mapped layer locked bits
    bool    m_IgnoreLockedLayers;

    LSET    m_LayerVisible;                 ///< bit-mapped layer visible bits
    bool    m_IgnoreNonVisibleLayers;

    bool    m_IgnoreLockedItems;
    bool    m_IncludeSecondary;

    bool    m_IgnoreMTextsMarkedNoShow;
    bool    m_IgnoreMTextsOnCopper;
    bool    m_IgnoreMTextsOnCmp;
    bool    m_IgnoreModulesOnCu;
    bool    m_IgnoreModulesOnCmp;
    bool    m_IgnorePadsOnFront;
    bool    m_IgnorePadsOnBack;
    bool    m_IgnoreModulesVals;
    bool    m_IgnoreModulesRefs;

public:

    /**
     * Constructor GENERAL_COLLECTORS_GUIDE
     * grabs stuff from global preferences and uses reasonable defaults.
     * Add more constructors as needed.
     * @param aVisibleLayerMask = current visible layers (bit mask)
     * @param aPreferredLayer = the layer to search first
     */
    GENERAL_COLLECTORS_GUIDE( LSET aVisibleLayerMask, LAYER_ID aPreferredLayer )
    {
        m_PreferredLayer            = aPreferredLayer;
        m_IgnorePreferredLayer      = false;
        m_LayerVisible              = aVisibleLayerMask;
        m_IgnoreLockedLayers        = true;
        m_IgnoreNonVisibleLayers    = true;
        m_IgnoreLockedItems         = false;

#if defined(USE_MATCH_LAYER)
        m_IncludeSecondary          = false;
#else
        m_IncludeSecondary          = true;
#endif

        m_IgnoreMTextsMarkedNoShow  = true; // g_ModuleTextNOVColor;
        m_IgnoreMTextsOnCopper      = true;
        m_IgnoreMTextsOnCmp         = false;
        m_IgnoreModulesOnCu         = true; // !Show_Modules_Cmp;
        m_IgnoreModulesOnCmp        = false;

        m_IgnorePadsOnFront         = false;
        m_IgnorePadsOnBack          = false;

        m_IgnoreModulesVals         = false;
        m_IgnoreModulesRefs         = false;
    }


    /**
     * Function IsLayerLocked
     * @return bool - true if the given layer is locked, else false.
     */
    bool IsLayerLocked( LAYER_ID aLayerId ) const
    {
        return m_LayerLocked[aLayerId];
    }

    void SetLayerLocked( LAYER_ID aLayerId, bool isLocked )
    {
        m_LayerLocked.set( aLayerId, isLocked );
    }

    /**
     * Function IsLayerVisible
     * @return bool - true if the given layer is visible, else false.
     */
    bool IsLayerVisible( LAYER_ID aLayerId ) const
    {
        return m_LayerVisible[aLayerId];
    }
    void SetLayerVisible( LAYER_ID aLayerId, bool isVisible )
    {
        m_LayerVisible.set( aLayerId, isVisible );
    }
    void SetLayerVisibleBits( LSET aLayerBits ) { m_LayerVisible = aLayerBits; }

    /**
     * Function IgnoreLockedLayers
     * @return bool - true if should ignore locked layers, else false.
     */
    bool IgnoreLockedLayers() const                 { return m_IgnoreLockedLayers; }
    void SetIgnoreLockedLayers( bool ignore )       { m_IgnoreLockedLayers = ignore; }


    /**
     * Function IgnoredNonVisibleLayers
     * @return bool - true if should ignore non-visible layers, else false.
     */
    bool IgnoreNonVisibleLayers() const             { return m_IgnoreNonVisibleLayers; }
    void SetIgnoreNonVisibleLayers( bool ignore )   { m_IgnoreLockedLayers = ignore; }


    /**
     * Function GetPreferredLayer
     * @return int - the preferred layer for HitTest()ing.
     */
    LAYER_ID GetPreferredLayer() const             { return m_PreferredLayer; }
    void SetPreferredLayer( LAYER_ID aLayer )      { m_PreferredLayer = aLayer; }


    /**
     * Function IgnorePreferredLayer
     * provides wildcard behavior regarding the preferred layer.
     * @return bool - true if should ignore preferred layer, else false.
     */
    bool IgnorePreferredLayer() const               { return  m_IgnorePreferredLayer; }
    void SetIgnorePreferredLayer( bool ignore )     { m_IgnorePreferredLayer = ignore; }


    /**
     * Function IgnoreLockedItems
     * @return bool - true if should ignore locked items, else false.
     */
    bool IgnoreLockedItems() const                  { return m_IgnoreLockedItems; }
    void SetIgnoreLockedItems( bool ignore )        { m_IgnoreLockedItems = ignore; }


    /**
     * Function IncludeSecondary
     * determines if the secondary criteria, or 2nd choice items should be
     * included.
     * @return bool - true if should include, else false.
     */
    bool IncludeSecondary() const  { return m_IncludeSecondary; }
    void SetIncludeSecondary( bool include ) { m_IncludeSecondary = include; }


    /**
     * Function IgnoreMTextsMarkedNoShow
     * @return bool - true if MTexts marked as "no show" should be ignored.
     */
    bool IgnoreMTextsMarkedNoShow() const { return m_IgnoreMTextsMarkedNoShow; }
    void SetIgnoreMTextsMarkedNoShow( bool ignore ) { m_IgnoreMTextsMarkedNoShow = ignore; }

    /**
     * Function IgnoreMTextsOnCu
     * @return bool - true if should ignore MTexts on copper layer.
     */
    bool IgnoreMTextsOnCopper() const { return m_IgnoreMTextsOnCopper; }
    void SetIgnoreMTextsOnCopper( bool ignore ) { m_IgnoreMTextsOnCopper = ignore; }

    /**
     * Function IgnoreMTextsOnCmp
     * @return bool - true if should ignore MTexts on component layer.
     */
    bool IgnoreMTextsOnCmp() const { return m_IgnoreMTextsOnCmp; }
    void SetIgnoreMTextsOnCmp( bool ignore ) { m_IgnoreMTextsOnCmp = ignore; }

    /**
     * Function IgnoreModulesOnCu
     * @return bool - true if should ignore MODULEs on copper layer.
     */
    bool IgnoreModulesOnCu() const { return m_IgnoreModulesOnCu; }
    void SetIgnoreModulesOnCu( bool ignore ) { m_IgnoreModulesOnCu = ignore; }

    /**
     * Function IgnoreModulesOnCmp
     * @return bool - true if should ignore MODULEs on component layer.
     */
    bool IgnoreModulesOnCmp() const { return m_IgnoreModulesOnCmp; }
    void SetIgnoreModulesOnCmp( bool ignore ) { m_IgnoreModulesOnCmp = ignore; }

    /**
     * Function IgnorePadsOnBack
     * @return bool - true if should ignore Pads on Back Side.
     */
    bool IgnorePadsOnBack() const { return m_IgnorePadsOnBack; }
    void SetIgnorePadsOnBack(bool ignore) { m_IgnorePadsOnBack = ignore; }

    /**
     * Function IgnorePadsOnFront
     * @return bool - true if should ignore PADSs on Front Side.
     */
    bool IgnorePadsOnFront() const { return m_IgnorePadsOnFront; }
    void SetIgnorePadsOnFront(bool ignore) { m_IgnorePadsOnFront = ignore; }

    /**
     * Function IgnoreModulesVals
     * @return bool - true if should ignore modules values.
     */
    bool IgnoreModulesVals() const { return m_IgnoreModulesVals; }
    void SetIgnoreModulesVals(bool ignore) { m_IgnoreModulesVals = ignore; }

    /**
     * Function IgnoreModulesRefs
     * @return bool - true if should ignore modules references.
     */
    bool IgnoreModulesRefs() const { return m_IgnoreModulesRefs; }
    void SetIgnoreModulesRefs(bool ignore) { m_IgnoreModulesRefs = ignore; }
};


/**
 * Class TYPE_COLLECTOR
 * merely gathers up all BOARD_ITEMs of a given set of KICAD_T type(s).
 * @see class COLLECTOR
 */
class TYPE_COLLECTOR : public COLLECTOR
{

public:

    /**
     * Function operator[int]
     * overloads COLLECTOR::operator[](int) to return a BOARD_ITEM* instead of
     * an EDA_ITEM* type.
     * @param ndx The index into the list.
     * @return BOARD_ITEM* - or something derived from it, or NULL.
     */
    BOARD_ITEM* operator[]( int ndx ) const
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (BOARD_ITEM*) m_List[ ndx ];
        return NULL;
    }


    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the
     * Iterate function.
     *
     * @param testItem An EDA_ITEM to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */
    SEARCH_RESULT Inspect( EDA_ITEM* testItem, const void* testData );


    /**
     * Function Collect
     * scans a BOARD_ITEM using this class's Inspector method, which does
     * the collection.
     * @param aBoard The BOARD_ITEM to scan.
     * @param aScanList The KICAD_Ts to gather up.
     */
    void Collect( BOARD_ITEM* aBoard, const KICAD_T aScanList[] );
};



#endif // COLLECTORS_H
