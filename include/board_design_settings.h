/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef BOARD_DESIGN_SETTINGS_H_
#define BOARD_DESIGN_SETTINGS_H_

#include <class_pad.h>
#include <class_track.h>
#include <netclass.h>
#include <config_params.h>
#include <board_stackup_manager/class_board_stackup.h>

#define DEFAULT_SILK_LINE_WIDTH       0.12
#define DEFAULT_COPPER_LINE_WIDTH     0.20
#define DEFAULT_EDGE_WIDTH            0.05
#define DEFAULT_COURTYARD_WIDTH       0.05
#define DEFAULT_LINE_WIDTH            0.10

#define DEFAULT_SILK_TEXT_SIZE        1.0
#define DEFAULT_COPPER_TEXT_SIZE      1.5
#define DEFAULT_TEXT_SIZE             1.0

#define DEFAULT_SILK_TEXT_WIDTH       0.15
#define DEFAULT_COPPER_TEXT_WIDTH     0.30
#define DEFAULT_TEXT_WIDTH            0.15

// Board thickness, mainly for 3D view:
#define DEFAULT_BOARD_THICKNESS_MM    1.6

#define DEFAULT_PCB_EDGE_THICKNESS    0.15

#define DEFAULT_SOLDERMASK_CLEARANCE  0.051  // soldermask to pad clearance
#define DEFAULT_SOLDERMASK_MIN_WIDTH  0.25   // soldermask minimum dam size
#define DEFAULT_SOLDERPASTE_CLEARANCE 0.0
#define DEFAULT_SOLDERPASTE_RATIO     0.0

#define DEFAULT_CUSTOMTRACKWIDTH      0.2
#define DEFAULT_CUSTOMDPAIRWIDTH      0.125
#define DEFAULT_CUSTOMDPAIRGAP        0.18
#define DEFAULT_CUSTOMDPAIRVIAGAP     0.18

#define DEFAULT_TRACKMINWIDTH         0.2     // track width min value
#define DEFAULT_VIASMINSIZE           0.4     // vias (not micro vias) min diameter
#define DEFAULT_VIASMINDRILL          0.3     // vias (not micro vias) min drill diameter
#define DEFAULT_MICROVIASMINSIZE      0.2     // micro vias (not vias) min diameter
#define DEFAULT_MICROVIASMINDRILL     0.1     // micro vias (not vias) min drill diameter
#define DEFAULT_HOLETOHOLEMIN         0.25    // separation between drilled hole edges

#define DEFAULT_COPPEREDGECLEARANCE   0.01    // clearance between copper items and edge cuts
#define LEGACY_COPPEREDGECLEARANCE   -0.01    // A flag to indicate the legacy method (based
                                              // on edge cut line thicknesses) should be used.

#define MINIMUM_ERROR_SIZE_MM         0.001
#define MAXIMUM_ERROR_SIZE_MM         0.1

/**
 * Struct VIA_DIMENSION
 * is a small helper container to handle a stock of specific vias each with
 * unique diameter and drill sizes in the BOARD class.
 */
struct VIA_DIMENSION
{
    int m_Diameter;     // <= 0 means use Netclass via diameter
    int m_Drill;        // <= 0 means use Netclass via drill

    VIA_DIMENSION()
    {
        m_Diameter = 0;
        m_Drill    = 0;
    }

    VIA_DIMENSION( int aDiameter, int aDrill )
    {
        m_Diameter = aDiameter;
        m_Drill    = aDrill;
    }

    bool operator==( const VIA_DIMENSION& aOther ) const
    {
        return ( m_Diameter == aOther.m_Diameter ) && ( m_Drill == aOther.m_Drill );
    }

    bool operator<( const VIA_DIMENSION& aOther ) const
    {
        if( m_Diameter != aOther.m_Diameter )
            return m_Diameter < aOther.m_Diameter;

        return m_Drill < aOther.m_Drill;
    }
};


/**
 * Struct DIFF_PAIR_DIMENSION
 * is a small helper container to handle a stock of specific differential pairs each with
 * unique track width, gap and via gap.
 */
struct DIFF_PAIR_DIMENSION
{
    int m_Width;         // <= 0 means use Netclass differential pair width
    int m_Gap;           // <= 0 means use Netclass differential pair gap
    int m_ViaGap;        // <= 0 means use Netclass differential pair via gap

    DIFF_PAIR_DIMENSION()
    {
        m_Width  = 0;
        m_Gap    = 0;
        m_ViaGap = 0;
    }

    DIFF_PAIR_DIMENSION( int aWidth, int aGap, int aViaGap )
    {
        m_Width  = aWidth;
        m_Gap    = aGap;
        m_ViaGap = aViaGap;
    }

    bool operator==( const DIFF_PAIR_DIMENSION& aOther ) const
    {
        return ( m_Width == aOther.m_Width )
                && ( m_Gap == aOther.m_Gap )
                && ( m_ViaGap == aOther.m_ViaGap );
    }

    bool operator<( const DIFF_PAIR_DIMENSION& aOther ) const
    {
        if( m_Width != aOther.m_Width )
            return m_Width < aOther.m_Width;

        if( m_Gap != aOther.m_Gap )
            return m_Gap < aOther.m_Gap;

        return m_ViaGap < aOther.m_ViaGap;
    }
};


enum
{
    LAYER_CLASS_SILK = 0,
    LAYER_CLASS_COPPER,
    LAYER_CLASS_EDGES,
    LAYER_CLASS_COURTYARD,
    LAYER_CLASS_OTHERS,

    LAYER_CLASS_COUNT
};


/**
 * Class BOARD_DESIGN_SETTINGS
 * contains design settings for a BOARD object.
 */
class BOARD_DESIGN_SETTINGS
{
public:
    // Note: the first value in each dimensions list is the current netclass value
    std::vector<int>                 m_TrackWidthList;
    std::vector<VIA_DIMENSION>       m_ViasDimensionsList;
    std::vector<DIFF_PAIR_DIMENSION> m_DiffPairDimensionsList;

    // List of netclasses. There is always the default netclass.
    NETCLASSES m_NetClasses;

    bool       m_MicroViasAllowed;          ///< true to allow micro vias
    bool       m_BlindBuriedViaAllowed;     ///< true to allow blind/buried vias
    VIATYPE_T  m_CurrentViaType;            ///< (VIA_BLIND_BURIED, VIA_THROUGH, VIA_MICROVIA)

    bool       m_RequireCourtyards;         ///< require courtyard definitions in footprints
    bool       m_ProhibitOverlappingCourtyards;  ///< check for overlapping courtyards in DRC

    // if true, when creating a new track starting on an existing track, use this track width
    bool       m_UseConnectedTrackWidth;
    int        m_TrackMinWidth;             ///< track min value for width ((min copper size value
    int        m_ViasMinSize;               ///< vias (not micro vias) min diameter
    int        m_ViasMinDrill;              ///< vias (not micro vias) min drill diameter
    int        m_MicroViasMinSize;          ///< micro vias (not vias) min diameter
    int        m_MicroViasMinDrill;         ///< micro vias (not vias) min drill diameter
    int        m_CopperEdgeClearance;

    /** Option to handle filled polygons in zones:
     * the "legacy" option is using thick outlines around filled polygons: give the best shape
     * the "new" option is using only filled polygons (no outline: give the faster redraw time
     * moreover when exporting zone filled areas, the excatct shape is exported.
     * the legacy option can really create redraw time issues for large boards.
     */
    bool       m_ZoneUseNoOutlineInFill;    ///< true for new zone filling option

    // Maximum error allowed when approximating circles and arcs to segments
    int        m_MaxError;

    // Global mask margins:
    int        m_SolderMaskMargin;          ///< Solder mask margin
    int        m_SolderMaskMinWidth;        ///< Solder mask min width
                                            // 2 areas near than m_SolderMaskMinWidth
                                            // are merged
    int        m_SolderPasteMargin;         ///< Solder paste margin absolute value
    double     m_SolderPasteMarginRatio;    ///< Solder pask margin ratio value of pad size
                                            ///< The final margin is the sum of these 2 values

    int        m_HoleToHoleMin;             ///< Min width of peninsula between two drilled holes

    // Arrays of default values for the various layer classes.
    int        m_LineThickness[ LAYER_CLASS_COUNT ];
    wxSize     m_TextSize[ LAYER_CLASS_COUNT ];
    int        m_TextThickness[ LAYER_CLASS_COUNT ];
    bool       m_TextItalic[ LAYER_CLASS_COUNT ];
    bool       m_TextUpright[ LAYER_CLASS_COUNT ];

    // Variables used in footprint editing (default value in item/footprint creation)

    wxString   m_RefDefaultText;            ///< Default ref text on fp creation
                                            // if empty, use footprint name as default
    bool       m_RefDefaultVisibility;      ///< Default ref text visibility on fp creation
    int        m_RefDefaultlayer;           ///< Default ref text layer on fp creation
                                            // should be a PCB_LAYER_ID, but use an int
                                            // to save this param in config

    wxString   m_ValueDefaultText;          ///< Default value text on fp creation
                                            // if empty, use footprint name as default
    bool       m_ValueDefaultVisibility;    ///< Default value text visibility on fp creation
    int        m_ValueDefaultlayer;         ///< Default value text layer on fp creation
                                            // should be a PCB_LAYER_ID, but use an int
                                            // to save this param in config

    // Miscellaneous
    wxPoint    m_AuxOrigin;                 ///< origin for plot exports
    wxPoint    m_GridOrigin;                ///< origin for grid offsets

    D_PAD      m_Pad_Master;                ///< A dummy pad to store all default parameters
                                            // when importing values or create a new pad

    /** Set to true if the board has a stackup management.
     * if m_hasStackup is false, a default basic stackup witll be used to
     * generate the ;gbrjob file.
     * if m_hasStackup is true, the stackup defined for the board is used.
     * if not up to date, a error message will be set
     * Could be removed later, or at least always set to true
     */
    bool m_HasStackup;

private:
    // Indicies into the trackWidth, viaSizes and diffPairDimensions lists.
    // The 0 index is always the current netclass value(s)
    unsigned   m_trackWidthIndex;
    unsigned   m_viaSizeIndex;
    unsigned   m_diffPairIndex;

    // Custom values for track/via sizes (specified via dialog instead of netclass or lists)
    bool       m_useCustomTrackVia;
    int        m_customTrackWidth;
    VIA_DIMENSION m_customViaSize;

    // Custom values for differential pairs (specified via dialog instead of netclass/lists)
    bool       m_useCustomDiffPair;
    DIFF_PAIR_DIMENSION m_customDiffPair;

    int        m_copperLayerCount; ///< Number of copper layers for this design

    LSET       m_enabledLayers;    ///< Bit-mask for layer enabling
    LSET       m_visibleLayers;    ///< Bit-mask for layer visibility

    int        m_visibleElements;  ///< Bit-mask for element category visibility
    int        m_boardThickness;   ///< Board thickness for 3D viewer

    /// Current net class name used to display netclass info.
    /// This is also the last used netclass after starting a track.
    wxString   m_currentNetClassName;

    /** the description of layers stackup, for board fabrication
     * only physical layers are in layers stackup.
     * It includes not only layers enabled for the board edition, but also dielectic layers
     */
    BOARD_STACKUP m_stackup;

public:
    BOARD_DESIGN_SETTINGS();

    BOARD_STACKUP& GetStackupDescriptor() { return m_stackup; }

    /**
     * Function GetDefault
     * @return the default netclass.
     */
    inline NETCLASSPTR GetDefault() const
    {
        return m_NetClasses.GetDefault();
    }

    /**
     * Function GetCurrentNetClassName
     * @return the current net class name.
     */
    inline const wxString& GetCurrentNetClassName() const
    {
        return m_currentNetClassName;
    }

    /**
     * Function UseNetClassTrack
     * returns true if netclass values should be used to obtain appropriate track width.
     */
    inline bool UseNetClassTrack() const
    {
        return ( m_trackWidthIndex == 0 && !m_useCustomTrackVia );
    }

    /**
     * Function UseNetClassVia
     * returns true if netclass values should be used to obtain appropriate via size.
     */
    inline bool UseNetClassVia() const
    {
        return ( m_viaSizeIndex == 0 && !m_useCustomTrackVia );
    }

    /**
     * Function UseNetClassDiffPair
     * returns true if netclass values should be used to obtain appropriate diff pair dimensions.
     */
    inline bool UseNetClassDiffPair() const
    {
        return ( m_diffPairIndex == 0 && !m_useCustomDiffPair );
    }

    /**
     * Function SetCurrentNetClass
     * Must be called after a netclass selection (or after a netclass parameter change
     * Initialize vias and tracks values displayed in comb boxes of the auxiliary toolbar
     * and some others parameters (netclass name ....)
     * @param aNetClassName = the new netclass name
     * @return true if lists of tracks and vias sizes are modified
     */
    bool SetCurrentNetClass( const wxString& aNetClassName );

    /**
     * Function GetBiggestClearanceValue
     * @return the biggest clearance value found in NetClasses list
     */
    int GetBiggestClearanceValue();

    /**
     * Function GetSmallestClearanceValue
     * @return the smallest clearance value found in NetClasses list
     */
    int GetSmallestClearanceValue();

    /**
     * Function GetCurrentMicroViaSize
     * @return the current micro via size,
     * that is the current netclass value
     */
    int GetCurrentMicroViaSize();

    /**
     * Function GetCurrentMicroViaDrill
     * @return the current micro via drill,
     * that is the current netclass value
     */
    int GetCurrentMicroViaDrill();

    /**
     * Function GetTrackWidthIndex
     * @return the current track width list index.
     */
    inline unsigned GetTrackWidthIndex() const { return m_trackWidthIndex; }

    /**
     * Function SetTrackWidthIndex
     * sets the current track width list index to \a aIndex.
     *
     * @param aIndex is the track width list index.
     */
    void SetTrackWidthIndex( unsigned aIndex );

    /**
     * Function GetCurrentTrackWidth
     * @return the current track width, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    inline int GetCurrentTrackWidth() const
    {
        return m_useCustomTrackVia ? m_customTrackWidth : m_TrackWidthList[m_trackWidthIndex];
    }

    /**
     * Function SetCustomTrackWidth
     * Sets custom width for track (i.e. not available in netclasses or preset list). To have
     * it returned with GetCurrentTrackWidth() you need to enable custom track & via sizes
     * (UseCustomTrackViaSize()).
     * @param aWidth is the new track width.
     */
    inline void SetCustomTrackWidth( int aWidth )
    {
        m_customTrackWidth = aWidth;
    }

    /**
     * Function GetCustomTrackWidth
     * @return Current custom width for a track.
     */
    inline int GetCustomTrackWidth() const
    {
        return m_customTrackWidth;
    }

    /**
     * Function GetViaSizeIndex
     * @return the current via size list index.
     */
    inline unsigned GetViaSizeIndex() const
    {
        return m_viaSizeIndex;
    }

    /**
     * Function SetViaSizeIndex
     * sets the current via size list index to \a aIndex.
     *
     * @param aIndex is the via size list index.
     */
    void SetViaSizeIndex( unsigned aIndex );

    /**
     * Function GetCurrentViaSize
     * @return the current via size, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    inline int GetCurrentViaSize() const
    {
        if( m_useCustomTrackVia )
            return m_customViaSize.m_Diameter;
        else
            return m_ViasDimensionsList[m_viaSizeIndex].m_Diameter;
    }

    /**
     * Function SetCustomViaSize
     * Sets custom size for via diameter (i.e. not available in netclasses or preset list). To have
     * it returned with GetCurrentViaSize() you need to enable custom track & via sizes
     * (UseCustomTrackViaSize()).
     * @param aSize is the new drill diameter.
     */
    inline void SetCustomViaSize( int aSize )
    {
        m_customViaSize.m_Diameter = aSize;
    }

    /**
     * Function GetCustomViaSize
     * @return Current custom size for the via diameter.
     */
    inline int GetCustomViaSize() const
    {
        return m_customViaSize.m_Diameter;
    }

    /**
     * Function GetCurrentViaDrill
     * @return the current via size, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    int GetCurrentViaDrill() const;

    /**
     * Function SetCustomViaDrill
     * Sets custom size for via drill (i.e. not available in netclasses or preset list). To have
     * it returned with GetCurrentViaDrill() you need to enable custom track & via sizes
     * (UseCustomTrackViaSize()).
     * @param aDrill is the new drill size.
     */
    inline void SetCustomViaDrill( int aDrill )
    {
        m_customViaSize.m_Drill = aDrill;
    }

    /**
     * Function GetCustomViaDrill
     * @return Current custom size for the via drill.
     */
    inline int GetCustomViaDrill() const
    {
        return m_customViaSize.m_Drill;
    }

    /**
     * Function UseCustomTrackViaSize
     * Enables/disables custom track/via size settings. If enabled, values set with
     * SetCustomTrackWidth()/SetCustomViaSize()/SetCustomViaDrill() are used for newly created
     * tracks and vias.
     * @param aEnabled decides if custom settings should be used for new tracks/vias.
     */
    inline void UseCustomTrackViaSize( bool aEnabled )
    {
        m_useCustomTrackVia = aEnabled;
    }

    /**
     * Function UseCustomTrackViaSize
     * @return True if custom sizes of tracks & vias are enabled, false otherwise.
     */
    inline bool UseCustomTrackViaSize() const
    {
        return m_useCustomTrackVia;
    }

    /**
     * Function GetDiffPairIndex
     * @return the current diff pair dimension list index.
     */
    inline unsigned GetDiffPairIndex() const { return m_diffPairIndex; }

    /**
     * Function SetDiffPairIndex
     * @param aIndex is the diff pair dimensions list index to set.
     */
    void SetDiffPairIndex( unsigned aIndex );

    /**
     * Function SetCustomDiffPairWidth
     * Sets custom track width for differential pairs (i.e. not available in netclasses or
     * preset list).
     * @param aDrill is the new track wdith.
     */
    inline void SetCustomDiffPairWidth( int aWidth )
    {
        m_customDiffPair.m_Width = aWidth;
    }

    /**
     * Function GetCustomDiffPairWidth
     * @return Current custom track width for differential pairs.
     */
    inline int GetCustomDiffPairWidth()
    {
        return m_customDiffPair.m_Width;
    }

    /**
     * Function SetCustomDiffPairGap
     * Sets custom gap for differential pairs (i.e. not available in netclasses or preset
     * list).
     * @param aGap is the new gap.
     */
    inline void SetCustomDiffPairGap( int aGap )
    {
        m_customDiffPair.m_Gap = aGap;
    }

    /**
     * Function GetCustomDiffPairGap
     * @return Current custom gap width for differential pairs.
     */
    inline int GetCustomDiffPairGap()
    {
        return m_customDiffPair.m_Gap;
    }

    /**
     * Function SetCustomDiffPairViaGap
     * Sets custom via gap for differential pairs (i.e. not available in netclasses or
     * preset list).
     * @param aGap is the new gap.  Specify 0 to use the DiffPairGap for vias as well.
     */
    inline void SetCustomDiffPairViaGap( int aGap )
    {
        m_customDiffPair.m_ViaGap = aGap;
    }

    /**
     * Function GetCustomDiffPairViaGap
     * @return Current custom via gap width for differential pairs.
     */
    inline int GetCustomDiffPairViaGap()
    {
        return m_customDiffPair.m_ViaGap > 0 ? m_customDiffPair.m_ViaGap : m_customDiffPair.m_Gap;
    }

    /**
     * Function UseCustomDiffPairDimensions
     * Enables/disables custom differential pair dimensions.
     * @param aEnabled decides if custom settings should be used for new differential pairs.
     */
    inline void UseCustomDiffPairDimensions( bool aEnabled )
    {
        m_useCustomDiffPair = aEnabled;
    }

    /**
     * Function UseCustomDiffPairDimensions
     * @return True if custom sizes of diff pairs are enabled, false otherwise.
     */
    inline bool UseCustomDiffPairDimensions() const
    {
        return m_useCustomDiffPair;
    }

    /**
     * Function GetCurrentDiffPairWidth
     * @return the current diff pair track width, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_DiffPairDimensionsList[0]
     */
    inline int GetCurrentDiffPairWidth() const
    {
        if( m_useCustomDiffPair )
            return m_customDiffPair.m_Width;
        else
            return m_DiffPairDimensionsList[m_diffPairIndex].m_Width;
    }

    /**
     * Function GetCurrentDiffPairGap
     * @return the current diff pair gap, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_DiffPairDimensionsList[0]
     */
    inline int GetCurrentDiffPairGap() const
    {
        if( m_useCustomDiffPair )
            return m_customDiffPair.m_Gap;
        else
            return m_DiffPairDimensionsList[m_diffPairIndex].m_Gap;
    }

    /**
     * Function GetCurrentDiffPairViaGap
     * @return the current diff pair via gap, according to the selected options
     * ( using the default netclass value or a preset/custom value )
     * the default netclass is always in m_DiffPairDimensionsList[0]
     */
    inline int GetCurrentDiffPairViaGap() const
    {
        if( m_useCustomDiffPair )
            return m_customDiffPair.m_ViaGap;
        else
            return m_DiffPairDimensionsList[m_diffPairIndex].m_ViaGap;
    }

    /**
     * Function SetMinHoleSeparation
     * @param aValue The minimum distance between the edges of two holes or 0 to disable
     * hole-to-hole separation checking.
     */
    void SetMinHoleSeparation( int aDistance );

    /**
     * Function SetCopperEdgeClearance
     * @param aValue The minimum distance between copper items and board edges.
     */
    void SetCopperEdgeClearance( int aDistance );

    /**
     * Function SetRequireCourtyardDefinitions
     * @param aRequire Set to true to generate DRC violations from missing courtyards.
     */
    void SetRequireCourtyardDefinitions( bool aRequire );

    /**
     * Function SetProhibitOverlappingCourtyards
     * @param aProhibit Set to true to generate DRC violations from overlapping courtyards.
     */
    void SetProhibitOverlappingCourtyards( bool aProhibit );

    /**
     * Function GetVisibleLayers
     * returns a bit-mask of all the layers that are visible
     * @return int - the visible layers in bit-mapped form.
     */
    inline LSET GetVisibleLayers() const
    {
        return m_visibleLayers;
    }

    /**
     * Function SetVisibleAlls
     * Set the bit-mask of all visible elements categories,
     * including enabled layers
     */
    void SetVisibleAlls();

    /**
     * Function SetVisibleLayers
     * changes the bit-mask of visible layers
     * @param aMask = The new bit-mask of visible layers
     */
    inline void SetVisibleLayers( LSET aMask )
    {
        m_visibleLayers = aMask & m_enabledLayers;
    }

    /**
     * Function IsLayerVisible
     * tests whether a given layer is visible
     * @param aLayerId = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    inline bool IsLayerVisible( PCB_LAYER_ID aLayerId ) const
    {
        // If a layer is disabled, it is automatically invisible
        return (m_visibleLayers & m_enabledLayers)[aLayerId];
    }

    /**
     * Function SetLayerVisibility
     * changes the visibility of a given layer
     * @param aLayerId = The layer to be changed
     * @param aNewState = The new visibility state of the layer
     */
    void SetLayerVisibility( PCB_LAYER_ID aLayerId, bool aNewState );

    /**
     * Function GetVisibleElements
     * returns a bit-mask of all the element categories that are visible
     * @return int - the visible element categories in bit-mapped form.
     */
    inline int GetVisibleElements() const
    {
        return m_visibleElements;
    }

    /**
     * Function SetVisibleElements
     * changes the bit-mask of visible element categories
     * @param aMask = The new bit-mask of visible element categories
     */
    inline void SetVisibleElements( int aMask )
    {
        m_visibleElements = aMask;
    }

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aElementCategory is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum GAL_LAYER_ID
     */
    inline bool IsElementVisible( GAL_LAYER_ID aElementCategory ) const
    {
        return ( m_visibleElements & ( 1 << GAL_LAYER_INDEX( aElementCategory ) ) );
    }

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aElementCategory is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum GAL_LAYER_ID
     */
    void SetElementVisibility( GAL_LAYER_ID aElementCategory, bool aNewState );

    /**
     * Function GetEnabledLayers
     * returns a bit-mask of all the layers that are enabled
     * @return int - the enabled layers in bit-mapped form.
     */
    inline LSET GetEnabledLayers() const
    {
        return m_enabledLayers;
    }

    /**
     * Function SetEnabledLayers
     * changes the bit-mask of enabled layers
     * @param aMask = The new bit-mask of enabled layers
     */
    void SetEnabledLayers( LSET aMask );

    /**
     * Function IsLayerEnabled
     * tests whether a given layer is enabled
     * @param aLayerId = The layer to be tested
     * @return bool - true if the layer is enabled
     */
    inline bool IsLayerEnabled( PCB_LAYER_ID aLayerId ) const
    {
        return m_enabledLayers[aLayerId];
    }

    /**
     * Function GetCopperLayerCount
     * @return int - the number of neabled copper layers
     */
    inline int GetCopperLayerCount() const
    {
        return m_copperLayerCount;
    }

    /**
     * Function SetCopperLayerCount
     * do what its name says...
     * @param aNewLayerCount = The new number of enabled copper layers
     */
    void SetCopperLayerCount( int aNewLayerCount );

    /**
     * Function AppendConfigs
     * appends to @a aResult the configuration setting accessors which will later
     * allow reading or writing of configuration file information directly into
     * this object.
     */
    void AppendConfigs( BOARD* aBoard, PARAM_CFG_ARRAY* aResult );

    inline int GetBoardThickness() const { return m_boardThickness; }
    inline void SetBoardThickness( int aThickness ) { m_boardThickness = aThickness; }

    /**
     * Function GetLineThickness
     * Returns the default graphic segment thickness from the layer class for the given layer.
     */
    int GetLineThickness( PCB_LAYER_ID aLayer ) const;

    /**
     * Function GetTextSize
     * Returns the default text size from the layer class for the given layer.
     */
    wxSize GetTextSize( PCB_LAYER_ID aLayer ) const;

    /**
     * Function GetTextThickness
     * Returns the default text thickness from the layer class for the given layer.
     */
    int GetTextThickness( PCB_LAYER_ID aLayer ) const;

    bool GetTextItalic( PCB_LAYER_ID aLayer ) const;
    bool GetTextUpright( PCB_LAYER_ID aLayer ) const;

    int GetLayerClass( PCB_LAYER_ID aLayer ) const;

private:
    void formatNetClass( NETCLASS* aNetClass, OUTPUTFORMATTER* aFormatter, int aNestLevel,
                         int aControlBits ) const;
};

#endif  // BOARD_DESIGN_SETTINGS_H_
