/**********************************************************/
/*  class_board_design_settings.h :  handle board options */
/**********************************************************/

#ifndef BOARD_DESIGN_SETTINGS_H_
#define BOARD_DESIGN_SETTINGS_H_

#include <pcbstruct.h>      // NB_COLORS
#include <class_pad.h>
#include <class_track.h>
#include <config_params.h>

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
 * Class BOARD_DESIGN_SETTINGS
 * contains design settings for a BOARD object.
 */
class BOARD_DESIGN_SETTINGS
{
public:
    // The first value is the current netclass via size
    /// Vias size and drill list
    std::vector<VIA_DIMENSION> m_ViasDimensionsList;

    // The first value is the current netclass track width
    /// Track width list
    std::vector<int> m_TrackWidthList;

    bool    m_MicroViasAllowed;             ///< true to allow micro vias
    bool    m_BlindBuriedViaAllowed;        ///< true to allow blind/buried vias
    VIATYPE_T m_CurrentViaType;             ///< via type (VIA_BLIND_BURIED, VIA_THROUGH VIA_MICROVIA)

    /// if true, when creating a new track starting on an existing track, use this track width
    bool    m_UseConnectedTrackWidth;
    int     m_DrawSegmentWidth;             ///< current graphic line width (not EDGE layer)
    int     m_EdgeSegmentWidth;             ///< current graphic line width (EDGE layer only)
    int     m_PcbTextWidth;                 ///< current Pcb (not module) Text width
    wxSize  m_PcbTextSize;                  ///< current Pcb (not module) Text size
    int     m_TrackMinWidth;                ///< track min value for width ((min copper size value
    int     m_ViasMinSize;                  ///< vias (not micro vias) min diameter
    int     m_ViasMinDrill;                 ///< vias (not micro vias) min drill diameter
    int     m_MicroViasMinSize;             ///< micro vias (not vias) min diameter
    int     m_MicroViasMinDrill;            ///< micro vias (not vias) min drill diameter

    // Global mask margins:
    int     m_SolderMaskMargin;             ///< Solder mask margin
    int     m_SolderMaskMinWidth;           ///< Solder mask min width
                                            // 2 areas near than m_SolderMaskMinWidth
                                            // are merged
    int     m_SolderPasteMargin;            ///< Solder paste margin absolute value
    double  m_SolderPasteMarginRatio;       ///< Solder pask margin ratio value of pad size
                                            ///< The final margin is the sum of these 2 values

    // Variables used in footprint handling
    wxSize  m_ModuleTextSize;               ///< Default footprint texts size
    int     m_ModuleTextWidth;
    int     m_ModuleSegmentWidth;
    wxPoint m_AuxOrigin;                    ///< origin for plot exports
    wxPoint m_GridOrigin;                   ///< origin for grid offsets

    D_PAD   m_Pad_Master;

    BOARD_DESIGN_SETTINGS();

    /**
     * Function GetTrackWidthIndex
     * @return the current track width list index.
     */
    unsigned GetTrackWidthIndex() const { return m_trackWidthIndex; }

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
    int GetCurrentTrackWidth() const
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
    void SetCustomTrackWidth( int aWidth )
    {
        m_customTrackWidth = aWidth;
    }

    /**
     * Function GetCustomTrackWidth
     * @return Current custom width for a track.
     */
    int GetCustomTrackWidth() const
    {
        return m_customTrackWidth;
    }

    /**
     * Function GetViaSizeIndex
     * @return the current via size list index.
     */
    unsigned GetViaSizeIndex() const { return m_viaSizeIndex; }

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
    int GetCurrentViaSize() const
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
    void SetCustomViaSize( int aSize )
    {
        m_customViaSize.m_Diameter = aSize;
    }

    /**
     * Function GetCustomViaSize
     * @return Current custom size for the via diameter.
     */
    int GetCustomViaSize() const
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
    void SetCustomViaDrill( int aDrill )
    {
        m_customViaSize.m_Drill = aDrill;
    }

    /**
     * Function GetCustomViaDrill
     * @return Current custom size for the via drill.
     */
    int GetCustomViaDrill() const
    {
        return m_customViaSize.m_Drill;
    }

    // TODO microvia methods should go here

    /**
     * Function UseCustomTrackViaSize
     * Enables/disables custom track/via size settings. If enabled, values set with
     * SetCustomTrackWidth()/SetCustomViaSize()/SetCustomViaDrill() are used for newly created
     * tracks and vias.
     * @param aEnabled decides if custom settings should be used for new tracks/vias.
     */
    void UseCustomTrackViaSize( bool aEnabled )
    {
        m_useCustomTrackVia = aEnabled;
    }

    /**
     * Function UseCustomTrackViaSize
     * @return True if custom sizes of tracks & vias are enabled, false otherwise.
     */
    bool UseCustomTrackViaSize() const
    {
        return m_useCustomTrackVia;
    }

    /**
     * Function GetVisibleLayers
     * returns a bit-mask of all the layers that are visible
     * @return int - the visible layers in bit-mapped form.
     */
    LAYER_MSK GetVisibleLayers() const;

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
    void SetVisibleLayers( LAYER_MSK aMask );

    /**
     * Function IsLayerVisible
     * tests whether a given layer is visible
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerVisible( LAYER_NUM aLayer ) const
    {
        // If a layer is disabled, it is automatically invisible
        return m_VisibleLayers & m_EnabledLayers & GetLayerMask( aLayer );
    }

    /**
     * Function SetLayerVisibility
     * changes the visibility of a given layer
     * @param aLayer = The layer to be changed
     * @param aNewState = The new visibility state of the layer
     */
    void SetLayerVisibility( LAYER_NUM aLayer, bool aNewState );

    /**
     * Function GetVisibleElements
     * returns a bit-mask of all the element categories that are visible
     * @return int - the visible element categories in bit-mapped form.
     */
    int GetVisibleElements() const
    {
        return m_VisibleElements;
    }

    /**
     * Function SetVisibleElements
     * changes the bit-mask of visible element categories
     * @param aMask = The new bit-mask of visible element categories
     */
    void SetVisibleElements( int aMask )
    {
        m_VisibleElements = aMask;
    }

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aElementCategory is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aElementCategory ) const
    {
        assert( aElementCategory >= 0 && aElementCategory < END_PCB_VISIBLE_LIST );

        return ( m_VisibleElements & ( 1 << aElementCategory ) );
    }

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aElementCategory is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aElementCategory, bool aNewState );

    /**
     * Function GetEnabledLayers
     * returns a bit-mask of all the layers that are enabled
     * @return int - the enabled layers in bit-mapped form.
     */
    inline LAYER_MSK GetEnabledLayers() const
    {
        return m_EnabledLayers;
    }

    /**
     * Function SetEnabledLayers
     * changes the bit-mask of enabled layers
     * @param aMask = The new bit-mask of enabled layers
     */
    void SetEnabledLayers( LAYER_MSK aMask );

    /**
     * Function IsLayerEnabled
     * tests whether a given layer is enabled
     * @param aLayer = The of the layer to be tested
     * @return bool - true if the layer is enabled
     */
    bool IsLayerEnabled( LAYER_NUM aLayer ) const
    {
        return m_EnabledLayers & GetLayerMask( aLayer );
    }

    /**
     * Function GetCopperLayerCount
     * @return int - the number of neabled copper layers
     */
    int GetCopperLayerCount() const
    {
        return m_CopperLayerCount;
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
    void AppendConfigs( PARAM_CFG_ARRAY* aResult );

    int GetBoardThickness() const { return m_boardThickness; }
    void SetBoardThickness( int aThickness ) { m_boardThickness = aThickness; }

private:
    /// Index for #m_ViasDimensionsList to select the current via size.
    /// 0 is the index selection of the default value Netclass
    unsigned m_viaSizeIndex;

    // Index for m_TrackWidthList to select the value.
    /// 0 is the index selection of the default value Netclass
    unsigned m_trackWidthIndex;

    ///> Use custom values for track/via sizes (not specified in net class nor in the size lists).
    bool m_useCustomTrackVia;

    ///> Custom track width (used after UseCustomTrackViaSize( true ) was called).
    int m_customTrackWidth;

    ///> Custom via size (used after UseCustomTrackViaSize( true ) was called).
    VIA_DIMENSION m_customViaSize;

    int       m_CopperLayerCount;   ///< Number of copper layers for this design
    LAYER_MSK m_EnabledLayers;      ///< Bit-mask for layer enabling
    LAYER_MSK m_VisibleLayers;      ///< Bit-mask for layer visibility
    int       m_VisibleElements;    ///< Bit-mask for element category visibility
    int       m_boardThickness;     ///< Board thickness for 3D viewer
};

#endif  // BOARD_DESIGN_SETTINGS_H_
