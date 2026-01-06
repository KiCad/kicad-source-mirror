/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef FOOTPRINT_H
#define FOOTPRINT_H

#include <deque>

#include <template_fieldnames.h>

#include <board_item_container.h>
#include <board_item.h>
#include <collectors.h>
#include <component_classes/component_class_manager.h>
#include <embedded_files.h>
#include <layer_ids.h> // ALL_LAYERS definition.
#include <lset.h>
#include <lib_id.h>
#include <list>

#include <zones.h>
#include <convert_shape_list_to_polygon.h>
#include <pcb_item_containers.h>
#include <pcb_text.h>
#include <pcb_field.h>
#include <functional>
#include <math/vector3.h>
#include <case_insensitive_map.h>

class LINE_READER;
class EDA_3D_CANVAS;
class PAD;
class BOARD;
class MSG_PANEL_ITEM;
class SHAPE;
class REPORTER;
class COMPONENT_CLASS_CACHE_PROXY;
class PCB_POINT;

namespace KIGFX {
class VIEW;
}

namespace KIFONT {
class OUTLINE_FONT;
}

enum INCLUDE_NPTH_T
{
    DO_NOT_INCLUDE_NPTH = false,
    INCLUDE_NPTH = true
};

/**
 * The set of attributes allowed within a FOOTPRINT, using FOOTPRINT::SetAttributes()
 * and FOOTPRINT::GetAttributes().  These are to be ORed together when calling
 * FOOTPRINT::SetAttributes()
 */
enum FOOTPRINT_ATTR_T
{
    FP_THROUGH_HOLE             = 0x0001,
    FP_SMD                      = 0x0002,
    FP_EXCLUDE_FROM_POS_FILES   = 0x0004,
    FP_EXCLUDE_FROM_BOM         = 0x0008,
    FP_BOARD_ONLY               = 0x0010,   // Footprint has no corresponding symbol
    FP_JUST_ADDED               = 0x0020,   // Footprint just added by netlist update
    FP_DNP                      = 0x0040
};

enum class FOOTPRINT_STACKUP
{
    /**
     * The 'normal' stackup handling, where there is a single inner layer
     * (In1) and rule areas using it expand to all inner layer on the host PCB.
     */
    EXPAND_INNER_LAYERS,
    /**
     * Stackup handling where the footprint can have any number of copper layers,
     * and objects on those layers go to the matching inner layer on the host PCB.
     */
    CUSTOM_LAYERS,
};

class FP_3DMODEL
{
public:
    FP_3DMODEL() :
        // Initialize with sensible values
        m_Scale { 1, 1, 1 },
        m_Rotation { 0, 0, 0 },
        m_Offset { 0, 0, 0 },
        m_Opacity( 1.0 ),
        m_Show( true )
    {
    }

    VECTOR3D m_Scale;       ///< 3D model scaling factor (dimensionless)
    VECTOR3D m_Rotation;    ///< 3D model rotation (degrees)
    VECTOR3D m_Offset;      ///< 3D model offset (mm)
    double   m_Opacity;
    wxString m_Filename;    ///< The 3D shape filename in 3D library
    bool     m_Show;        ///< Include model in rendering

    bool operator==( const FP_3DMODEL& aOther ) const
    {
        return m_Scale == aOther.m_Scale
                && m_Rotation == aOther.m_Rotation
                && m_Offset == aOther.m_Offset
                && m_Opacity == aOther.m_Opacity
                && m_Filename == aOther.m_Filename
                && m_Show == aOther.m_Show;
    }
};


/**
 * Variant information for a footprint.
 *
 * Footprint variants store per-variant overrides for DNP, exclusion flags, and field values.
 * These are synchronized with SCH_SYMBOL_VARIANT during Update PCB from Schematic operations.
 */
class FOOTPRINT_VARIANT
{
public:
    FOOTPRINT_VARIANT( const wxString& aName = wxEmptyString ) :
            m_name( aName ),
            m_dnp( false ),
            m_excludedFromBOM( false ),
            m_excludedFromPosFiles( false )
    {
    }

    wxString GetName() const { return m_name; }
    void SetName( const wxString& aName ) { m_name = aName; }

    bool GetDNP() const { return m_dnp; }
    void SetDNP( bool aDNP ) { m_dnp = aDNP; }

    bool GetExcludedFromBOM() const { return m_excludedFromBOM; }
    void SetExcludedFromBOM( bool aExclude ) { m_excludedFromBOM = aExclude; }

    bool GetExcludedFromPosFiles() const { return m_excludedFromPosFiles; }
    void SetExcludedFromPosFiles( bool aExclude ) { m_excludedFromPosFiles = aExclude; }

    /**
     * Get a field value override for this variant.
     * @param aFieldName The name of the field.
     * @return The field value, or empty string if not overridden or overridden to empty.
     */
    wxString GetFieldValue( const wxString& aFieldName ) const
    {
        auto it = m_fields.find( aFieldName );

        if( it != m_fields.end() )
            return it->second;

        return wxString();
    }

    /**
     * Set a field value override for this variant.
     * @param aFieldName The name of the field.
     * @param aValue The value to set.
     */
    void SetFieldValue( const wxString& aFieldName, const wxString& aValue )
    {
        m_fields[aFieldName] = aValue;
    }

    bool HasFieldValue( const wxString& aFieldName ) const
    {
        return m_fields.find( aFieldName ) != m_fields.end();
    }

    const std::map<wxString, wxString>& GetFields() const { return m_fields; }

    bool operator==( const FOOTPRINT_VARIANT& aOther ) const
    {
        return m_name == aOther.m_name
                && m_dnp == aOther.m_dnp
                && m_excludedFromBOM == aOther.m_excludedFromBOM
                && m_excludedFromPosFiles == aOther.m_excludedFromPosFiles
                && m_fields == aOther.m_fields;
    }

private:
    wxString                     m_name;
    bool                         m_dnp;
    bool                         m_excludedFromBOM;
    bool                         m_excludedFromPosFiles;
    std::map<wxString, wxString> m_fields;  ///< Field value overrides for this variant
};


class FOOTPRINT : public BOARD_ITEM_CONTAINER, public EMBEDDED_FILES
{
public:
    FOOTPRINT( BOARD* parent );

    FOOTPRINT( const FOOTPRINT& aFootprint );

    // Move constructor and operator needed due to std containers inside the footprint
    FOOTPRINT( FOOTPRINT&& aFootprint );

    ~FOOTPRINT();

    FOOTPRINT& operator=( const FOOTPRINT& aOther );
    FOOTPRINT& operator=( FOOTPRINT&& aOther );

    void CopyFrom( const BOARD_ITEM* aOther ) override;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_FOOTPRINT_T;
    }

    /// Resets the caches for this footprint, for example if it was modified via the API
    void InvalidateGeometryCaches();

    LSET GetPrivateLayers() const { return m_privateLayers; }
    void SetPrivateLayers( const LSET& aLayers ) { m_privateLayers = aLayers; }

    ///< @copydoc BOARD_ITEM_CONTAINER::Add()
    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_MODE::INSERT,
              bool aSkipConnectivity = false ) override;

    ///< @copydoc BOARD_ITEM_CONTAINER::Remove()
    void Remove( BOARD_ITEM* aItem, REMOVE_MODE aMode = REMOVE_MODE::NORMAL ) override;

    /**
     * Clear (i.e. force the ORPHANED dummy net info) the net info which
     * depends on a given board for all pads of the footprint.
     *
     * This is needed when a footprint is copied between the fp editor and
     * the board editor for instance, because net info become fully broken
     */
    void ClearAllNets();

    /**
     * Old footprints do not always have a valid UUID (some can be set to null uuid)
     * However null UUIDs, having a special meaning in editor, create issues when
     * editing a footprint
     * So all null uuids a re replaced by a valid uuid
     * @return true if at least one uuid is changed, false if no change
     */
    bool FixUuids();

    /**
     * Return the bounding box containing pads when the footprint is on the front side,
     * orientation 0, position 0,0.
     *
     * Mainly used in Gerber place file to draw a footprint outline when the courtyard
     * is missing or broken.
     *
     * @return The rectangle containing the pads for the normalized footprint.
     */
    BOX2I GetFpPadsLocalBbox() const;

    /**
     * Return a bounding polygon for the shapes and pads in the footprint.
     *
     * This operation is slower but more accurate than calculating a bounding box.
     */
    SHAPE_POLY_SET GetBoundingHull() const;
    SHAPE_POLY_SET GetBoundingHull( PCB_LAYER_ID aLayer ) const;

    bool TextOnly() const;

    // Virtual function
    const BOX2I GetBoundingBox() const override;
    const BOX2I GetBoundingBox( bool aIncludeText ) const;

    /**
     * Return the bounding box of the footprint on a given set of layers
    */
    const BOX2I GetLayerBoundingBox( const LSET& aLayers ) const;

    VECTOR2I GetCenter() const override { return GetBoundingBox( false ).GetCenter(); }

    std::deque<PAD*>& Pads()               { return m_pads; }
    const std::deque<PAD*>& Pads() const   { return m_pads; }

    DRAWINGS& GraphicalItems()             { return m_drawings; }
    const DRAWINGS& GraphicalItems() const { return m_drawings; }

    ZONES& Zones()                         { return m_zones; }
    const ZONES& Zones() const             { return m_zones; }

    GROUPS& Groups()                       { return m_groups; }
    const GROUPS& Groups() const           { return m_groups; }

    PCB_POINTS& Points()                   { return m_points; }
    const PCB_POINTS& Points() const       { return m_points; }

    bool HasThroughHolePads() const;

    std::vector<FP_3DMODEL>& Models()             { return m_3D_Drawings; }
    const std::vector<FP_3DMODEL>& Models() const { return m_3D_Drawings; }

    void     SetPosition( const VECTOR2I& aPos ) override;
    VECTOR2I GetPosition() const override { return m_pos; }

    void SetOrientation( const EDA_ANGLE& aNewAngle );
    EDA_ANGLE GetOrientation() const { return m_orient; }

    /**
     * Used as Layer property setter -- performs a flip if necessary to set the footprint layer
     * @param aLayer is the target layer (F_Cu or B_Cu)
     */
    void SetLayerAndFlip( PCB_LAYER_ID aLayer );

    // to make property magic work
    PCB_LAYER_ID GetLayer() const override { return BOARD_ITEM::GetLayer(); }

    // For property system:
    void SetOrientationDegrees( double aOrientation )
    {
        SetOrientation( EDA_ANGLE( aOrientation, DEGREES_T ) );
    }
    double GetOrientationDegrees() const
    {
        return m_orient.AsDegrees();
    }

    const LIB_ID& GetFPID() const { return m_fpid; }
    void          SetFPID( const LIB_ID& aFPID )
    {
        m_fpid = aFPID;
    }

    wxString GetFPIDAsString() const { return m_fpid.Format(); }
    void SetFPIDAsString( const wxString& aFPID ) { m_fpid.Parse( aFPID ); }

    wxString GetLibDescription() const { return m_libDescription; }
    void     SetLibDescription( const wxString& aDesc ) { m_libDescription = aDesc; }

    wxString GetKeywords() const { return m_keywords; }
    void SetKeywords( const wxString& aKeywords ) { m_keywords = aKeywords; }

    const KIID_PATH& GetPath() const { return m_path; }
    void SetPath( const KIID_PATH& aPath ) { m_path = aPath; }

    wxString GetSheetname() const { return m_sheetname; }
    void SetSheetname( const wxString& aSheetname ) { m_sheetname = aSheetname; }

    wxString GetSheetfile() const { return m_sheetfile; }
    void SetSheetfile( const wxString& aSheetfile ) { m_sheetfile = aSheetfile; }

    wxString GetFilters() const { return m_filters; }
    void SetFilters( const wxString& aFilters ) { m_filters = aFilters; }

    std::optional<int> GetLocalClearance() const                 { return m_clearance; }
    void SetLocalClearance( std::optional<int> aClearance )      { m_clearance = aClearance; }

    std::optional<int> GetLocalSolderMaskMargin() const          { return m_solderMaskMargin; }
    void SetLocalSolderMaskMargin( std::optional<int> aMargin )  { m_solderMaskMargin = aMargin; }

    std::optional<int> GetLocalSolderPasteMargin() const         { return m_solderPasteMargin; }
    void SetLocalSolderPasteMargin( std::optional<int> aMargin ) { m_solderPasteMargin = aMargin; }

    std::optional<double> GetLocalSolderPasteMarginRatio() const { return m_solderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( std::optional<double> aRatio ) { m_solderPasteMarginRatio = aRatio; }

    void SetLocalZoneConnection( ZONE_CONNECTION aType ) { m_zoneConnection = aType; }
    ZONE_CONNECTION GetLocalZoneConnection() const { return m_zoneConnection; }

    /**
     * Set the stackup mode for this footprint.
     *
     * This determines if the footprint lists its own layers or uses a default stackup,
     * with "expansion" of inner layers to the PCB's inner layers.
     */
    void SetStackupMode( FOOTPRINT_STACKUP aMode );
    FOOTPRINT_STACKUP GetStackupMode() const { return m_stackupMode; }

    /**
     * If the footprint has a non-default stackup, set the layers that
     * should be used for the stackup.
     */
    void SetStackupLayers( LSET aLayers );
    const LSET& GetStackupLayers() const { return m_stackupLayers; }

    int GetAttributes() const { return m_attributes; }
    void SetAttributes( int aAttributes ) { m_attributes = aAttributes; }

    bool AllowMissingCourtyard() const { return m_allowMissingCourtyard; }
    void SetAllowMissingCourtyard( bool aAllow ) { m_allowMissingCourtyard = aAllow; }

    bool AllowSolderMaskBridges() const { return m_allowSolderMaskBridges; }
    void SetAllowSolderMaskBridges( bool aAllow ) { m_allowSolderMaskBridges = aAllow; }

    void SetFlag( int aFlag ) { m_arflag = aFlag; }
    void IncrementFlag() { m_arflag += 1; }
    int GetFlag() const { return m_arflag; }

    bool IsNetTie() const
    {
        for( const wxString& group : m_netTiePadGroups )
        {
            if( !group.IsEmpty() )
                return true;
        }

        return false;
    }

    std::optional<int> GetLocalClearance( wxString* aSource ) const
    {
        if( m_clearance.has_value() && aSource )
            *aSource = wxString::Format( _( "footprint %s" ), GetReference() );

        return m_clearance;
    }

    /**
     * Return any local clearance overrides set in the "classic" (ie: pre-rule) system.
     *
     * @param aSource [out] optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
    std::optional<int> GetClearanceOverrides( wxString* aSource ) const
    {
        return GetLocalClearance( aSource );
    }

    ZONE_CONNECTION GetZoneConnectionOverrides( wxString* aSource ) const
    {
        if( m_zoneConnection != ZONE_CONNECTION::INHERITED && aSource )
            *aSource = wxString::Format( _( "footprint %s" ), GetReference() );

        return m_zoneConnection;
    }

    /**
     * @return a list of pad groups, each of which is allowed to short nets within their group.
     *         A pad group is a comma-separated list of pad numbers.
     */
    const std::vector<wxString>& GetNetTiePadGroups() const { return m_netTiePadGroups; }

    void ClearNetTiePadGroups()
    {
        m_netTiePadGroups.clear();
    }

    void AddNetTiePadGroup( const wxString& aGroup )
    {
        m_netTiePadGroups.emplace_back( aGroup );
    }

    /**
     * @return a map from pad numbers to net-tie group indices.  If a pad is not a member of
     *         a net-tie group its index will be -1.
     */
    std::map<wxString, int> MapPadNumbersToNetTieGroups() const;

    /**
     * @return a list of pads that appear in \a aPad's net-tie pad group.
     */
    std::vector<PAD*> GetNetTiePads( PAD* aPad ) const;

    /**
     * Returns the most likely attribute based on pads
     * Either FP_THROUGH_HOLE/FP_SMD/OTHER(0)
     * @return 0/FP_SMD/FP_THROUGH_HOLE
     */
    int GetLikelyAttribute() const;

    void Move( const VECTOR2I& aMoveVector ) override;

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /**
     * Move the reference point of the footprint.
     *
     * It looks like a move footprint:
     * the footprints elements (pads, outlines, edges .. ) are moved
     * However:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     * (a move footprint does not change these local coordinates,
     * but changes the footprint position)
     */
    void MoveAnchorPosition( const VECTOR2I& aMoveVector );

    /**
     * @return true if the footprint is flipped, i.e. on the back side of the board
     */
    bool IsFlipped() const { return GetLayer() == B_Cu; }

    /**
     * Use instead of IsFlipped() when you also need to account for unsided footprints (those
     * purely on user-layers, etc.).
     */
    PCB_LAYER_ID GetSide() const;

    /**
     * @copydoc BOARD_ITEM::IsOnLayer
     */
    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override;

// m_footprintStatus bits:
#define FP_is_LOCKED        0x01        ///< footprint LOCKED: no autoplace allowed
#define FP_is_PLACED        0x02        ///< In autoplace: footprint automatically placed
#define FP_to_PLACE         0x04        ///< In autoplace: footprint waiting for autoplace
#define FP_PADS_are_LOCKED  0x08


    bool IsLocked() const override
    {
        return ( m_fpStatus & FP_is_LOCKED ) != 0;
    }

    /**
     * Set the #MODULE_is_LOCKED bit in the m_ModuleStatus.
     *
     * @param isLocked true means turn on locked status, else unlock
     */
    void SetLocked( bool isLocked ) override
    {
        if( isLocked )
            m_fpStatus |= FP_is_LOCKED;
        else
            m_fpStatus &= ~FP_is_LOCKED;
    }

    /**
     * @return true if the footprint is flagged with conflicting with some item
     */
    bool IsConflicting() const;

    bool IsPlaced() const { return m_fpStatus & FP_is_PLACED;  }
    void SetIsPlaced( bool isPlaced )
    {
        if( isPlaced )
            m_fpStatus |= FP_is_PLACED;
        else
            m_fpStatus &= ~FP_is_PLACED;
    }

    bool NeedsPlaced() const { return m_fpStatus & FP_to_PLACE;  }
    void SetNeedsPlaced( bool needsPlaced )
    {
        if( needsPlaced )
            m_fpStatus |= FP_to_PLACE;
        else
            m_fpStatus &= ~FP_to_PLACE;
    }

    bool LegacyPadsLocked() const { return m_fpStatus & FP_PADS_are_LOCKED;  }

    /**
     * Test if footprint attributes for type (SMD/Through hole/Other) match the expected
     * type based on the pads in the footprint.
     * Footprints with plated through-hole pads should usually be marked through hole even if they
     * also have SMD because they might not be auto-placed.  Exceptions to this might be shielded
     * connectors.  Otherwise, footprints with SMD pads should be marked SMD.
     * Footprints with no connecting pads should be marked "Other"
     *
     * @param aErrorHandler callback to handle the error messages generated
     */
    void CheckFootprintAttributes( const std::function<void( const wxString& )>& aErrorHandler );

    /**
     * Run non-board-specific DRC checks on footprint's pads.  These are the checks supported by
     * both the PCB DRC and the Footprint Editor Footprint Checker.
     *
     * @param aErrorHandler callback to handle the error messages generated
     */
    void CheckPads( UNITS_PROVIDER* aUnitsProvider,
                    const std::function<void( const PAD*, int, const wxString& )>& aErrorHandler );

    /**
     * Check for overlapping, different-numbered, non-net-tie pads.
     *
     * @param aErrorHandler callback to handle the error messages generated
     */
    void CheckShortingPads( const std::function<void( const PAD*, const PAD*, int aErrorCode,
                                                      const VECTOR2I& )>& aErrorHandler );

    /**
     * Check for un-allowed shorting of pads in net-tie footprints.  If two pads are shorted,
     * they must both appear in one of the allowed-shorting lists.
     *
     * @param aErrorHandler callback to handle the error messages generated
     */
    void CheckNetTies( const std::function<void( const BOARD_ITEM* aItem,
                                                 const BOARD_ITEM* bItem,
                                                 const BOARD_ITEM* cItem,
                                                 const VECTOR2I& )>& aErrorHandler );

    /**
     * Sanity check net-tie pad groups.  Pads cannot be listed more than once, and pad numbers
     * must correspond to a pad.
     *
     * @param aErrorHandler callback to handle the error messages generated
     */
    void CheckNetTiePadGroups( const std::function<void( const wxString& )>& aErrorHandler );

    void CheckClippedSilk( const std::function<void( BOARD_ITEM* aItemA,
                                                     BOARD_ITEM* aItemB,
                                                     const VECTOR2I& aPt )>& aErrorHandler );
    /**
     * Cache the pads that are allowed to connect to each other in the footprint.
     */
    void BuildNetTieCache();

    /**
     * Get the set of net codes that are allowed to connect to a footprint item
     */
    const std::set<int>& GetNetTieCache( const BOARD_ITEM* aItem ) const
    {
        static const std::set<int> emptySet;

        auto it = m_netTieCache.find( aItem );

        if( it == m_netTieCache.end() )
            return emptySet;

        return it->second;
    }

    /**
     * Generate pads shapes on layer \a aLayer as polygons and adds these polygons to
     * \a aBuffer.
     *
     * Useful to generate a polygonal representation of a footprint in 3D view and plot functions,
     * when a full polygonal approach is needed.
     *
     * @param aLayer is the layer to consider, or #UNDEFINED_LAYER to consider all layers.
     * @param aBuffer i the buffer to store polygons.
     * @param aClearance is an additional size to add to pad shapes.
     * @param aMaxError is the maximum deviation from true for arcs.
     */
    void TransformPadsToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                 int aMaxError, ERROR_LOC aErrorLoc ) const;

    /**
     * Generate shapes of graphic items (outlines) on layer \a aLayer as polygons and adds these
     * polygons to \a aBuffer.
     *
     * Useful to generate a polygonal representation of a footprint in 3D view and plot functions,
     * when a full polygonal approach is needed.
     *
     * @param aLayer is the layer to consider, or #UNDEFINED_LAYER to consider all.
     * @param aBuffer is the buffer to store polygons.
     * @param aClearance is a value to inflate shapes.
     * @param aError is the maximum error between true arc and polygon approximation.
     * @param aIncludeText set to true to transform text shapes.
     * @param aIncludeShapes set to true to transform footprint shapes.
     */
    void TransformFPShapesToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                     int aError, ERROR_LOC aErrorLoc,
                                     bool aIncludeText = true,
                                     bool aIncludeShapes = true,
                                     bool aIncludePrivateItems = false ) const;

    /**
     * This function is the same as TransformFPShapesToPolySet but only generates text.
     */
    void TransformFPTextToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                   int aError, ERROR_LOC aErrorLoc ) const
    {
        TransformFPShapesToPolySet( aBuffer, aLayer, aClearance, aError, aErrorLoc, true, false );
    }

    /**
     * Return the list of system text vars for this footprint.
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    /**
     * Resolve any references to system tokens supported by the component.
     *
     * @param aDepth a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( wxString* token, int aDepth = 0 ) const;

    /// @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    /**
     * Test if a point is inside the bounding polygon of the footprint.
     *
     * The other hit test methods are just checking the bounding box, which can be quite
     * inaccurate for rotated or oddly-shaped footprints.
     *
     * @param aPosition is the point to test
     * @return true if aPosition is inside the bounding polygon
     */
    bool HitTestAccurate( const VECTOR2I& aPosition, int aAccuracy = 0 ) const;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    /**
     * Test if the point hits one or more of the footprint elements on a given layer.
     *
     * @param aPosition is the point to test
     * @param aAccuracy is the hit test accuracy
     * @param aLayer is the layer to test
     * @return true if aPosition hits a footprint element on aLayer
     */
    bool HitTestOnLayer( const VECTOR2I& aPosition, PCB_LAYER_ID aLayer, int aAccuracy = 0 ) const;

    bool HitTestOnLayer( const BOX2I& aRect, bool aContained, PCB_LAYER_ID aLayer, int aAccuracy = 0 ) const;

    /**
     * @return reference designator text.
     */
    const wxString& GetReference() const { return Reference().GetText(); }

    /**
     * @param aReference A reference to a wxString object containing the reference designator
     *                   text.
     */
    void SetReference( const wxString& aReference ) { Reference().SetText( aReference ); }

    // Property system doesn't like const references
    wxString GetReferenceAsString() const
    {
        return GetReference();
    }

    /**
     * Bump the current reference by \a aDelta.
     */
    void IncrementReference( int aDelta );

    /**
     * @return the value text.
     */
    const wxString& GetValue() const { return Value().GetText(); }

    /**
     * @param aValue A reference to a wxString object containing the value text.
     */
    void SetValue( const wxString& aValue ) { Value().SetText( aValue ); }

    // Property system doesn't like const references
    wxString GetValueAsString() const
    {
        return GetValue();
    }

    /// read/write accessors:
    PCB_FIELD& Value()           { return *GetField( FIELD_T::VALUE ); }
    PCB_FIELD& Reference()       { return *GetField( FIELD_T::REFERENCE ); }

    /// The const versions to keep the compiler happy.
    const PCB_FIELD& Value() const     { return *GetField( FIELD_T::VALUE ); }
    const PCB_FIELD& Reference() const { return *GetField( FIELD_T::REFERENCE ); }

    //-----<Fields>-----------------------------------------------------------

    /**
     * Return a mandatory field in this footprint.  The const version will return nullptr if
     * the field doesn't exist; the non-const version will create the field and return it.
     */
    PCB_FIELD*       GetField( FIELD_T aFieldType );
    const PCB_FIELD* GetField( FIELD_T aFieldNdx ) const;

    /**
     * Return a field in this symbol.
     *
     * @param aFieldName is the name of the field
     *
     * @return is the field with \a aFieldName or NULL if the field does not exist.
     */
    PCB_FIELD* GetField( const wxString& aFieldName ) const;

    bool HasField( const wxString& aFieldName ) const;

    /**
     * Populate a std::vector with PCB_TEXTs.
     *
     * @param aVector is the vector to populate.
     * @param aVisibleOnly is used to add only the fields that are visible and contain text.
     */
    void GetFields( std::vector<PCB_FIELD*>& aVector, bool aVisibleOnly ) const;

    /**
     * Return a reference to the deque holding the footprint's fields
     */
    const std::deque<PCB_FIELD*>& GetFields() const { return m_fields; }
    std::deque<PCB_FIELD*>& GetFields() { return m_fields; }

    /**
     * Return the next ordinal for a user field for this footprint
     */
    int GetNextFieldOrdinal() const;

    /**
     * @brief Apply default board settings to the footprint field text properties.
     *
     * This is needed because the board settings are not available when the footprint is
     * being created in the footprint library cache, and we want these fields to have
     * the correct default text properties.
     */
    void ApplyDefaultSettings( const BOARD& board, bool aStyleFields, bool aStyleText,
                               bool aStyleShapes, bool aStyleDimensions, bool aStyleBarcodes );

    struct FP_UNIT_INFO
    {
        wxString              m_unitName; // e.g. A
        std::vector<wxString> m_pins;     // pin numbers in this unit
    };

    void                             SetUnitInfo( const std::vector<FP_UNIT_INFO>& aUnits ) { m_unitInfo = aUnits; }
    const std::vector<FP_UNIT_INFO>& GetUnitInfo() const { return m_unitInfo; }

    bool IsBoardOnly() const { return m_attributes & FP_BOARD_ONLY; }
    void SetBoardOnly( bool aIsBoardOnly = true )
    {
        if( aIsBoardOnly )
            m_attributes |= FP_BOARD_ONLY;
        else
            m_attributes &= ~FP_BOARD_ONLY;
    }

    bool IsExcludedFromPosFiles() const { return m_attributes & FP_EXCLUDE_FROM_POS_FILES; }
    void SetExcludedFromPosFiles( bool aExclude = true )
    {
        if( aExclude )
            m_attributes |= FP_EXCLUDE_FROM_POS_FILES;
        else
            m_attributes &= ~FP_EXCLUDE_FROM_POS_FILES;
    }

    bool IsExcludedFromBOM() const { return m_attributes & FP_EXCLUDE_FROM_BOM; }
    void SetExcludedFromBOM( bool aExclude = true )
    {
        if( aExclude )
            m_attributes |= FP_EXCLUDE_FROM_BOM;
        else
            m_attributes &= ~FP_EXCLUDE_FROM_BOM;
    }

    bool IsDNP() const { return m_attributes & FP_DNP; }
    void SetDNP( bool aDNP = true )
    {
        if( aDNP )
            m_attributes |= FP_DNP;
        else
            m_attributes &= ~FP_DNP;
    }

    // =====================================================================
    // Variant Support
    // =====================================================================

    /**
     * Get a variant by name.
     * @param aVariantName The name of the variant.
     * @return Pointer to the variant, or nullptr if not found.
     */
    const FOOTPRINT_VARIANT* GetVariant( const wxString& aVariantName ) const;

    /**
     * Get a mutable variant by name.
     * @param aVariantName The name of the variant.
     * @return Pointer to the variant, or nullptr if not found.
     */
    FOOTPRINT_VARIANT* GetVariant( const wxString& aVariantName );

    /**
     * Add or update a variant.
     * @param aVariant The variant to add or update.
     */
    void SetVariant( const FOOTPRINT_VARIANT& aVariant );

    /**
     * Add a new variant with the given name.
     * @param aVariantName The name of the variant to add.
     * @return Reference to the newly created variant.
     */
    FOOTPRINT_VARIANT* AddVariant( const wxString& aVariantName );

    /**
     * Delete a variant by name.
     * @param aVariantName The name of the variant to delete.
     */
    void DeleteVariant( const wxString& aVariantName );

    /**
     * Rename a variant.
     * @param aOldName The current name of the variant.
     * @param aNewName The new name for the variant.
     */
    void RenameVariant( const wxString& aOldName, const wxString& aNewName );

    /**
     * Check if a variant exists.
     * @param aVariantName The name of the variant.
     * @return true if the variant exists.
     */
    bool HasVariant( const wxString& aVariantName ) const;

    /**
     * Get all variants.
     * @return Map of variant name to variant data.
     */
    const CASE_INSENSITIVE_MAP<FOOTPRINT_VARIANT>& GetVariants() const { return m_variants; }

    /**
     * Get the DNP status for a specific variant.
     *
     * If the variant doesn't exist, returns the default DNP status.
     *
     * @param aVariantName The variant name (empty for default).
     * @return true if DNP is set for the specified variant.
     */
    bool GetDNPForVariant( const wxString& aVariantName ) const;

    /**
     * Get the exclude-from-BOM status for a specific variant.
     *
     * If the variant doesn't exist, returns the default exclude-from-BOM status.
     *
     * @param aVariantName The variant name (empty for default).
     * @return true if excluded from BOM for the specified variant.
     */
    bool GetExcludedFromBOMForVariant( const wxString& aVariantName ) const;

    /**
     * Get the exclude-from-position-files status for a specific variant.
     *
     * If the variant doesn't exist, returns the default exclude-from-position-files status.
     *
     * @param aVariantName The variant name (empty for default).
     * @return true if excluded from position files for the specified variant.
     */
    bool GetExcludedFromPosFilesForVariant( const wxString& aVariantName ) const;

    /**
     * Get a field value for a specific variant.
     *
     * If the variant doesn't exist or doesn't override the field, returns the default field value.
     *
     * @param aVariantName The variant name (empty for default).
     * @param aFieldName The field name.
     * @return The field value for the specified variant.
     */
    wxString GetFieldValueForVariant( const wxString& aVariantName,
                                      const wxString& aFieldName ) const;

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad() const { return m_fileFormatVersionAtLoad; }

    /**
     * Return a #PAD with a matching number.
     *
     * @note Numbers may not be unique depending on how the footprint was created.
     *
     * @param aPadNumber the pad number to find.
     * @param aSearchAfterMe = not nullptr to find a pad living after aAfterMe
     * @return the first matching numbered #PAD is returned or NULL if not found.
     */
    PAD* FindPadByNumber( const wxString& aPadNumber, PAD* aSearchAfterMe = nullptr ) const;

    /**
     * Get a pad at \a aPosition on \a aLayerMask in the footprint.
     *
     * @param aPosition A VECTOR2I object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a #PAD object if found otherwise NULL.
     */
    PAD* GetPad( const VECTOR2I& aPosition, const LSET& aLayerMask = LSET::AllLayersMask() );

    std::vector<const PAD*> GetPads( const wxString& aPadNumber,
                                     const PAD* aIgnore = nullptr ) const;

    /**
     * Return the number of pads.
     *
     * @param aIncludeNPTH includes non-plated through holes when true.  Does not include
     *                     non-plated through holes when false.
     * @return the number of pads according to \a aIncludeNPTH.
     */
    unsigned GetPadCount( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH_T(INCLUDE_NPTH) ) const;

    /**
     * Return the number of unique non-blank pads.
     *
     * A complex pad can be built with many pads having the same pad name to create a complex
     * shape or fragmented solder paste areas.
     *
     * @param aIncludeNPTH includes non-plated through holes when true.  Does not include
     *                     non-plated through holes when false.
     * @return the number of unique pads according to \a aIncludeNPTH.
     */
    unsigned GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH_T(INCLUDE_NPTH) ) const;

    /**
     * Return the names of the unique, non-blank pads.
     */
    std::set<wxString>
    GetUniquePadNumbers( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH_T(INCLUDE_NPTH) ) const;

    /**
     * Return the next available pad number in the footprint.
     *
     * @param aFillSequenceGaps true if the numbering should "fill in" gaps in the sequence,
     *                          else return the highest value + 1
     * @return the next available pad number
     */
    wxString GetNextPadNumber( const wxString& aLastPadName ) const;

    bool GetDuplicatePadNumbersAreJumpers() const { return m_duplicatePadNumbersAreJumpers; }
    void SetDuplicatePadNumbersAreJumpers( bool aEnabled ) { m_duplicatePadNumbersAreJumpers = aEnabled; }

    /**
     * Each jumper pad group is a set of pad numbers that should be treated as internally connected.
     * @return The list of jumper pad groups in this footprint
     */
    std::vector<std::set<wxString>>& JumperPadGroups() { return m_jumperPadGroups; }
    const std::vector<std::set<wxString>>& JumperPadGroups() const { return m_jumperPadGroups; }

    /// Retrieves the jumper group containing the specified pad number, if one exists
    std::optional<const std::set<wxString>> GetJumperPadGroup( const wxString& aPadNumber ) const;

    /**
     * Position Reference and Value fields at the top and bottom of footprint's bounding box.
     */
    void AutoPositionFields();

    /**
     * Get the type of footprint
     * @return "SMD"/"Through hole"/"Other" based on attributes
     */
    wxString GetTypeName() const;

    double GetArea( int aPadding = 0 ) const;

    KIID GetLink() const { return m_link; }
    void SetLink( const KIID& aLink ) { m_link = aLink; }

    BOARD_ITEM* Duplicate( bool addToParentGroup, BOARD_COMMIT* aCommit = nullptr ) const override;

    /**
     * Duplicate a given item within the footprint, optionally adding it to the board.
     *
     * @return the new item, or NULL if the item could not be duplicated.
     */
    BOARD_ITEM* DuplicateItem( bool addToParentGroup, BOARD_COMMIT* aCommit, const BOARD_ITEM* aItem,
                               bool addToFootprint = false );

    /**
     * Add \a a3DModel definition to the end of the 3D model list.
     *
     * @param a3DModel A pointer to a #FP_3DMODEL to add to the list.
     */
    void Add3DModel( FP_3DMODEL* a3DModel );

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    wxString GetClass() const override
    {
        return wxT( "FOOTPRINT" );
    }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    ///< @copydoc BOARD_ITEM::RunOnChildren
    void RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const override;

    virtual std::vector<int> ViewGetLayers() const override;

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    virtual const BOX2I ViewBBox() const override;

    /**
     * Test for validity of a name of a footprint to be used in a footprint library
     * ( no spaces, dir separators ... ).
     *
     * @param aName is the name in library to validate.
     * @return true if the given name is valid
     */
    static bool IsLibNameValid( const wxString& aName );

    /**
     * Test for validity of the name in a library of the footprint ( no spaces, dir
     * separators ... ).
     *
     * @param aUserReadable set to false to get the list of invalid characters or  true to get
     *                      a readable form (i.e ' ' = 'space' '\\t'= 'tab').
     *
     * @return the list of invalid chars in the library name.
     */
    static const wxChar* StringLibNameInvalidChars( bool aUserReadable );

    /**
     * Return true if a board footprint differs from the library version.
     */
    bool FootprintNeedsUpdate( const FOOTPRINT* aLibFP, int aCompareFlags = 0,
                               REPORTER* aReporter = nullptr );

    /**
     * Take ownership of caller's heap allocated aInitialComments block.
     *
     * The comments are single line strings already containing the s-expression comments with
     * optional leading whitespace and then a '#' character followed by optional single line
     * text (text with no line endings, not even one).  This block of single line comments
     * will be output upfront of any generated s-expression text in the PCBIO::Format() function.
     *
     * @note A block of single line comments constitutes a multiline block of single line
     *       comments.  That is, the block is made of consecutive single line comments.
     *
     * @param aInitialComments is a heap allocated wxArrayString or NULL, which the caller
     *                         gives up ownership of over to this FOOTPRINT.
     */
    void SetInitialComments( wxArrayString* aInitialComments )
    {
        delete m_initial_comments;
        m_initial_comments = aInitialComments;
    }

    /**
     * Calculate the ratio of total area of the footprint pads and graphical items to the
     * area of the footprint. Used by selection tool heuristics.
     *
     * @return the ratio.
     */
    double CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const;

    static double GetCoverageArea( const BOARD_ITEM* aItem, const GENERAL_COLLECTOR& aCollector );

    /// Return the initial comments block or NULL if none, without transfer of ownership.
    const wxArrayString* GetInitialComments() const { return m_initial_comments; }

    /**
     * Used in DRC to test the courtyard area (a complex polygon).
     *
     * @return the courtyard polygon.
     */
    const SHAPE_POLY_SET& GetCourtyard( PCB_LAYER_ID aLayer ) const;

    /**
     * Return the cached courtyard area. No checks are performed.
     *
     * @return the cached courtyard polygon.
     */
    const SHAPE_POLY_SET& GetCachedCourtyard( PCB_LAYER_ID aLayer ) const;

    /**
     * Build complex polygons of the courtyard areas from graphic items on the courtyard layers.
     *
     * @note Set the #MALFORMED_F_COURTYARD and #MALFORMED_B_COURTYARD status flags if the given
     *       courtyard layer does not contain a (single) closed shape.
     */
    void BuildCourtyardCaches( OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );

    // @copydoc BOARD_ITEM::GetEffectiveShape
    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    EMBEDDED_FILES* GetEmbeddedFiles() override
    {
        return static_cast<EMBEDDED_FILES*>( this );
    }

    const EMBEDDED_FILES* GetEmbeddedFiles() const
    {
        return static_cast<const EMBEDDED_FILES*>( this );
    }

    /**
     * Get a list of outline fonts referenced in the footprint
     */
    std::set<KIFONT::OUTLINE_FONT*> GetFonts() const override;

    void EmbedFonts() override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    /// Sets the component class object pointer for this footprint
    void SetStaticComponentClass( const COMPONENT_CLASS* aClass ) const;

    /// Returns the component class for this footprint
    const COMPONENT_CLASS* GetStaticComponentClass() const;

    /// Returns the component class for this footprint
    const COMPONENT_CLASS* GetComponentClass() const;

    /// Used for display in the properties panel
    wxString GetComponentClassAsString() const;

    /// Forces immediate recalculation of the component class for this footprint
    void RecomputeComponentClass() const;

    /// Forces deferred (on next access) recalculation of the component class for this footprint
    void InvalidateComponentClassCache() const;

    /**
     * @brief Sets the transient component class names
     *
     * This is used during paste operations as we can't resolve the component classes immediately
     * until we have the true board context once the pasted items have been placed
     */
    void SetTransientComponentClassNames( const std::unordered_set<wxString>& classNames )
    {
        m_transientComponentClassNames = classNames;
    }

    /// Gets the transient component class names
    const std::unordered_set<wxString>& GetTransientComponentClassNames()
    {
        return m_transientComponentClassNames;
    }

    /// Remove the transient component class names
    void ClearTransientComponentClassNames() { m_transientComponentClassNames.clear(); };

    /// Resolves a set of component class names to this footprint's actual component class
    void ResolveComponentClassNames( BOARD*                              aBoard,
                                     const std::unordered_set<wxString>& aComponentClassNames );

    /// Used post-loading of a footprint to adjust the layers on pads to match board inner layers
    void FixUpPadsForBoard( BOARD* aBoard );

    bool operator==( const BOARD_ITEM& aOther ) const override;
    bool operator==( const FOOTPRINT& aOther ) const;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    struct cmp_drawings
    {
        bool operator()( const BOARD_ITEM* itemA, const BOARD_ITEM* itemB ) const;
    };

    struct cmp_pads
    {
        bool operator()( const PAD* aFirst, const PAD* aSecond ) const;
    };
  // TODO(JE) padstacks -- this code isn't used anywhere though
#if 0
    struct cmp_padstack
    {
        bool operator()( const PAD* aFirst, const PAD* aSecond ) const;
    };
#endif
    struct cmp_zones
    {
        bool operator()( const ZONE* aFirst, const ZONE* aSecond ) const;
    };

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

    void addMandatoryFields();

private:
    std::deque<PCB_FIELD*>  m_fields;    // Fields, mapped by name, owned by pointer
    std::deque<BOARD_ITEM*> m_drawings;  // Drawings in the footprint, owned by pointer
    std::deque<PAD*>        m_pads;      // Pads, owned by pointer
    std::vector<ZONE*>      m_zones;     // Rule area zones, owned by pointer
    std::deque<PCB_GROUP*>  m_groups;    // Groups, owned by pointer
    std::deque<PCB_POINT*>  m_points;    // Points, owned by pointer

    EDA_ANGLE       m_orient;            // Orientation
    VECTOR2I        m_pos;               // Position of footprint on the board in internal units.
    LIB_ID          m_fpid;              // The #LIB_ID of the FOOTPRINT.
    int             m_attributes;        // Flag bits (see FOOTPRINT_ATTR_T)
    int             m_fpStatus;          // For autoplace: flags (LOCKED, FIELDS_AUTOPLACED)
    int             m_fileFormatVersionAtLoad;

    /// Variant data for this footprint, keyed by variant name
    CASE_INSENSITIVE_MAP<FOOTPRINT_VARIANT> m_variants;

    // Bounding box caching strategy:
    // While we attempt to notice the low-hanging fruit operations and update the bounding boxes
    // accordingly, we rely mostly on a "if anything changed then the caches are stale" approach.
    // We implement this by having PCB_BASE_FRAME's OnModify() method increment an operation
    // counter, and storing that as a timestamp for the various caches.
    // This means caches will get regenerated often -- but still far less often than if we had no
    // caches at all.  The principal opitmization would be to change to dirty flag and make sure
    // that any edit that could affect the bounding boxes (including edits to the footprint
    // children) marked the bounding boxes dirty.  It would definitely be faster -- but also more
    // fragile.
    mutable BOX2I          m_cachedBoundingBox;
    mutable int            m_boundingBoxCacheTimeStamp;
    mutable BOX2I          m_cachedTextExcludedBBox;
    mutable int            m_textExcludedBBoxCacheTimeStamp;
    mutable SHAPE_POLY_SET m_cachedHull;
    mutable int            m_hullCacheTimeStamp;

    // A list of pad groups, each of which is allowed to short nets within their group.
    // A pad group is a comma-separated list of pad numbers.
    std::vector<wxString>  m_netTiePadGroups;

    // A list of 1:N footprint item to allowed net numbers
    std::map<const BOARD_ITEM*, std::set<int>> m_netTieCache;

    /// A list of jumper pad groups, each of which is a set of pad numbers that should be jumpered
    /// together (treated as internally connected for the purposes of connectivity)
    std::vector<std::set<wxString>> m_jumperPadGroups;

    /// Flag that this footprint should automatically treat sets of two or more pads with the same
    /// number as jumpered pin groups
    bool                  m_duplicatePadNumbersAreJumpers;

    bool                  m_allowMissingCourtyard;
    bool                  m_allowSolderMaskBridges;

    // Optional overrides
    ZONE_CONNECTION       m_zoneConnection;
    std::optional<int>    m_clearance;
    std::optional<int>    m_solderMaskMargin;       // Solder mask margin
    std::optional<int>    m_solderPasteMargin;      // Solder paste margin absolute value
    std::optional<double> m_solderPasteMarginRatio; // Solder mask margin ratio of pad size
                                                    // The final margin is the sum of these 2 values

    LSET              m_stackupLayers; // Layers in the stackup
    FOOTPRINT_STACKUP m_stackupMode;   // Stackup mode for this footprint
    LSET              m_privateLayers; // Layers visible only in the footprint editor

    wxString        m_libDescription;    // File name and path for documentation file.
    wxString        m_keywords;          // Search keywords to find footprint in library.
    KIID_PATH       m_path;              // Path to associated symbol ([sheetUUID, .., symbolUUID]).
    wxString        m_sheetname;         // Name of the sheet containing the symbol for this footprint
    wxString        m_sheetfile;         // File of the sheet containing the symbol for this footprint
    wxString        m_filters;           // Footprint filters from symbol
    timestamp_t     m_lastEditTime;
    int             m_arflag;            // Use to trace ratsnest and auto routing.
    KIID            m_link;              // Temporary logical link used during editing

    std::vector<FP_3DMODEL> m_3D_Drawings;       // 3D models.
    wxArrayString*          m_initial_comments;  // s-expression comments in the footprint,
                                                 //   lazily allocated only if needed for speed

    SHAPE_POLY_SET     m_courtyard_cache_front;  // Note that a footprint can have both front and back
    SHAPE_POLY_SET     m_courtyard_cache_back;   //   courtyards populated.
    mutable HASH_128   m_courtyard_cache_front_hash;
    mutable HASH_128   m_courtyard_cache_back_hash;
    mutable std::mutex m_courtyard_cache_mutex;

    std::unordered_set<wxString> m_transientComponentClassNames;
    std::unique_ptr<COMPONENT_CLASS_CACHE_PROXY> m_componentClassCacheProxy;

    // Optional unit mapping information for multi-unit symbols
    std::vector<FP_UNIT_INFO> m_unitInfo;
};

#endif     // FOOTPRINT_H
