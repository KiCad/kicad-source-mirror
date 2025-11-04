/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008-2018 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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

/**
 * @file zone_settings.h
 * @brief Class ZONE_SETTINGS used to handle zones parameters in dialogs.
 */

#pragma once

#include <optional>
#include <map>
#include <layer_ids.h>
#include <lset.h>
#include <zones.h>
#include <geometry/eda_angle.h>
#include <teardrop/teardrop_types.h>
#include <widgets/wx_grid.h>


class PCB_BASE_FRAME;
class wxDataViewListCtrl;


enum class ZONE_FILL_MODE
{
    POLYGONS = 0,     // fill zone with polygons
    HATCH_PATTERN = 1 // fill zone using a grid pattern
};

struct ZONE_LAYER_PROPERTIES
{
    std::optional<VECTOR2I> hatching_offset;

    bool operator==( const ZONE_LAYER_PROPERTIES& aOther ) const;
};


/// Zone border styles
enum class ZONE_BORDER_DISPLAY_STYLE
{
    NO_HATCH,
    DIAGONAL_FULL,
    DIAGONAL_EDGE,
    INVISIBLE_BORDER        // Disable outline drawing for very special cases
};

/// Whether or not to remove isolated islands from a zone
enum class ISLAND_REMOVAL_MODE
{
    ALWAYS = 0,
    NEVER,
    AREA
};

enum class PLACEMENT_SOURCE_T
{
    SHEETNAME = 0,
    COMPONENT_CLASS,
    GROUP_PLACEMENT,
    DESIGN_BLOCK
};

/**
 * ZONE_SETTINGS
 * handles zones parameters.
 * Because a zone can be on copper or non copper layers, and can be also
 * a keepout area, some parameters are irrelevant depending on the type of zone
 */
class ZONE_SETTINGS
{
public:
    // the actual zone outline shape can be slightly modified (smoothed):
    enum {
        SMOOTHING_UNDEFINED = -1,
        SMOOTHING_NONE = 0,         // Zone outline is used without change
        SMOOTHING_CHAMFER,          // Zone outline is used after chamfering corners
        SMOOTHING_FILLET,           // Zone outline is used after rounding corners
        SMOOTHING_LAST              // sentinel
    };

    unsigned        m_ZonePriority;          // Priority (0 ... N) of the zone

    ZONE_FILL_MODE  m_FillMode;
    int             m_ZoneClearance;         // Minimal clearance value
    int             m_ZoneMinThickness;      // Min thickness value in filled areas
    int             m_HatchThickness;        // HatchBorder thickness of lines (if 0 -> solid shape)
    int             m_HatchGap;              // HatchBorder clearance between lines (0 -> solid shape)
    EDA_ANGLE       m_HatchOrientation;      // HatchBorder orientation of grid lines
    int             m_HatchSmoothingLevel;   // HatchBorder smoothing type, similar to corner smoothing type
                                             // 0 = no smoothing, 1 = fillet, >= 2 = arc
    double          m_HatchSmoothingValue;   // HatchBorder chamfer/fillet size as a ratio of hole size
    double          m_HatchHoleMinArea;      // min size before holes are dropped (ratio)
    int             m_HatchBorderAlgorithm;  // 0 = use min zone thickness

    int             m_Netcode;               // Net code selection for the current zone

    wxString        m_Name;                  // Unique name for the current zone (can be blank)

    LSET            m_Layers;                // Layers that this zone exists on

    /// Option to show the zone area (outlines only, short hatches or full hatches
    ZONE_BORDER_DISPLAY_STYLE m_ZoneBorderDisplayStyle;
    int             m_BorderHatchPitch;     // for hatched outlines: dist between 2 hatches

    long            m_ThermalReliefGap;        // thickness of the gap in thermal reliefs
    long            m_ThermalReliefSpokeWidth; // thickness of the copper bridge in thermal reliefs

    bool            m_Locked;

    /* A zone outline can be a teardrop zone with different rules
     * priority, smoothed corners, thermal relief...
     */
    TEARDROP_TYPE   m_TeardropType;

    std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES> m_LayerProperties;

private:
    int             m_cornerSmoothingType;   // Corner smoothing type
    unsigned int    m_cornerRadius;          // Corner chamfer distance / fillet radius
    ZONE_CONNECTION m_padConnection;

    /*
     * Keepout zones and keepout flags.
     * Note that DRC rules can set keepouts on zones whether they're a keepout or not.
     */
    bool            m_isRuleArea;

    /**
     * Placement rule area data
     */
    bool                m_placementAreaEnabled;
    PLACEMENT_SOURCE_T  m_placementAreaSourceType;
    wxString            m_placementAreaSource;

    bool                m_keepoutDoNotAllowZoneFills;
    bool                m_keepoutDoNotAllowVias;
    bool                m_keepoutDoNotAllowTracks;
    bool                m_keepoutDoNotAllowPads;
    bool                m_keepoutDoNotAllowFootprints;

    ISLAND_REMOVAL_MODE m_removeIslands;
    long long int       m_minIslandArea;

public:
    ZONE_SETTINGS();

    bool operator==( const ZONE_SETTINGS& aOther ) const;

    bool operator!=( const ZONE_SETTINGS& aOther ) const { return !operator==( aOther ); }

    /**
     * operator << ( const ZONE& )
     * was Function ImportSetting
     * copies settings from a given zone into this object.
     * @param aSource: the given zone
     */
    ZONE_SETTINGS& operator << ( const ZONE& aSource );

    /**
     * @return Default ZONE_SETTINGS
     */
    static const ZONE_SETTINGS& GetDefaultSettings();

    /**
     * A helper routine for the various zone dialogs (copper, non-copper, keepout).
     * @param aList the wxDataViewListCtrl to populate
     * @param aFrame the parent editor frame
     */
    void SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame, LSET aLayers );

    /**
     * Function ExportSetting
     * copy settings to a given zone
     * @param aTarget: the given zone
     * @param aFullExport: if false: some parameters are NOT exported
     *   because they must not be  exported when export settings from a zone to others zones
     *   Currently: m_ZonePriority, m_Layers & m_LayersProperties, m_Name and m_Netcode
     */
    void ExportSetting( ZONE& aTarget, bool aFullExport = true ) const;

    /**
     * Function CopyFrom
     * copy settings from a different ZONE_SETTINGS object
     *
     * @param aOther the other ZONE_SETTINGS
     * @param aCopyFull if false: some parameters are not copied.
     * This option is used specifically to copy zone settings from
     * a zone to the default zone settings.
     * There, the layer information is not needed, plus layer specific
     * properties should not be overridden in the zone default settings.
     */
    void CopyFrom( const ZONE_SETTINGS& aOther, bool aCopyFull = true );

    void SetCornerSmoothingType( int aType) { m_cornerSmoothingType = aType; }
    int GetCornerSmoothingType() const { return m_cornerSmoothingType; }

    void SetCornerRadius( int aRadius );
    unsigned int GetCornerRadius() const { return m_cornerRadius; }

    ZONE_CONNECTION GetPadConnection() const { return m_padConnection; }
    void SetPadConnection( ZONE_CONNECTION aPadConnection ) { m_padConnection = aPadConnection; }

    /**
     * Accessor to determine if any keepout parameters are set
     */
    bool HasKeepoutParametersSet() const
    {
        return m_keepoutDoNotAllowTracks || m_keepoutDoNotAllowVias || m_keepoutDoNotAllowPads
               || m_keepoutDoNotAllowFootprints || m_keepoutDoNotAllowZoneFills;
    }

    /**
     * Accessors to parameters used in Rule Area zones:
     */
    bool GetPlacementAreaEnabled() const                  { return m_placementAreaEnabled; }
    PLACEMENT_SOURCE_T GetPlacementAreaSourceType() const { return m_placementAreaSourceType; }
    wxString GetPlacementAreaSource() const               { return m_placementAreaSource; }
    bool GetIsRuleArea() const           { return m_isRuleArea; }
    bool GetDoNotAllowZoneFills() const  { return m_keepoutDoNotAllowZoneFills; }
    bool GetDoNotAllowVias() const       { return m_keepoutDoNotAllowVias; }
    bool GetDoNotAllowTracks() const     { return m_keepoutDoNotAllowTracks; }
    bool GetDoNotAllowPads() const       { return m_keepoutDoNotAllowPads; }
    bool GetDoNotAllowFootprints() const { return m_keepoutDoNotAllowFootprints; }

    void SetPlacementAreaEnabled( bool aEnabled )               { m_placementAreaEnabled = aEnabled; }
    void SetPlacementAreaSourceType( PLACEMENT_SOURCE_T aType ) { m_placementAreaSourceType = aType; }
    void SetPlacementAreaSource( const wxString& aSource )      { m_placementAreaSource = aSource; }
    void SetIsRuleArea( bool aEnable )           { m_isRuleArea = aEnable; }
    void SetDoNotAllowZoneFills( bool aEnable )  { m_keepoutDoNotAllowZoneFills = aEnable; }
    void SetDoNotAllowVias( bool aEnable )       { m_keepoutDoNotAllowVias = aEnable; }
    void SetDoNotAllowTracks( bool aEnable )     { m_keepoutDoNotAllowTracks = aEnable; }
    void SetDoNotAllowPads( bool aEnable )       { m_keepoutDoNotAllowPads = aEnable; }
    void SetDoNotAllowFootprints( bool aEnable ) { m_keepoutDoNotAllowFootprints = aEnable; }

    ISLAND_REMOVAL_MODE GetIslandRemovalMode() const         { return m_removeIslands; }
    void SetIslandRemovalMode( ISLAND_REMOVAL_MODE aRemove ) { m_removeIslands = aRemove; }

    long long int GetMinIslandArea() const       { return m_minIslandArea; }
    void SetMinIslandArea( long long int aArea ) { m_minIslandArea = aArea; }
};


class LAYER_PROPERTIES_GRID_TABLE : public WX_GRID_TABLE_BASE
{
public:
    LAYER_PROPERTIES_GRID_TABLE( PCB_BASE_FRAME* aFrame, std::function<LSET()> getLayers );
    ~LAYER_PROPERTIES_GRID_TABLE() override;

    int GetNumberRows() override { return (int) m_items.size(); }
    int GetNumberCols() override { return 3; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return _( "Layer" );
        case 1: return _( "Offset X" );
        case 2: return _( "Offset Y" );
        default: return wxEmptyString;
        }
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case 0: return aTypeName == wxGRID_VALUE_NUMBER;
        case 1: return aTypeName == wxGRID_VALUE_STRING;
        case 2: return aTypeName == wxGRID_VALUE_STRING;
        default: wxFAIL; return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int aRow, int aCol ) override;
    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    long GetValueAsLong( int aRow, int aCol ) override;
    void SetValueAsLong( int aRow, int aCol, long aValue ) override;

    void AddItem( PCB_LAYER_ID aLayer, const ZONE_LAYER_PROPERTIES& aProps );
    bool AppendRows( size_t aNumRows = 1 ) override;
    bool DeleteRows( size_t aPos, size_t aNumRows ) override;

    const std::vector<std::pair<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>>& GetItems() { return m_items; }

protected:
    void onUnitsChanged( wxCommandEvent& aEvent );

private:
    PCB_BASE_FRAME*                                             m_frame;
    std::function<LSET()>                                       m_getLayersFunc;
    std::vector<std::pair<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>> m_items;
};


