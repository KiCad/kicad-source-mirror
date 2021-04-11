/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_item_container.h>
#include <board_item.h>
#include <collectors.h>
#include <convert_to_biu.h>
#include <layers_id_colors_and_visibility.h> // ALL_LAYERS definition.
#include <lib_id.h>
#include <list>

#include <zones.h>
#include <convert_drawsegment_list_to_polygon.h>
#include <fp_text.h>
#include <zone.h>
#include <functional>

class LINE_READER;
class EDA_3D_CANVAS;
class PAD;
class BOARD;
class MSG_PANEL_ITEM;
class SHAPE;

namespace KIGFX {
class VIEW;
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
    FP_THROUGH_HOLE           = 0x0001,
    FP_SMD                    = 0x0002,
    FP_EXCLUDE_FROM_POS_FILES = 0x0004,
    FP_EXCLUDE_FROM_BOM       = 0x0008,
    FP_BOARD_ONLY             = 0x0010    // Footprint has no corresponding symbol
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

    struct VECTOR3D
    {
        double x, y, z;
    };

    VECTOR3D m_Scale;       ///< 3D model scaling factor (dimensionless)
    VECTOR3D m_Rotation;    ///< 3D model rotation (degrees)
    VECTOR3D m_Offset;      ///< 3D model offset (mm)
    double   m_Opacity;
    wxString m_Filename;    ///< The 3D shape filename in 3D library
    bool     m_Show;        ///< Include model in rendering
};

DECL_DEQ_FOR_SWIG( PADS, PAD* )
DECL_DEQ_FOR_SWIG( DRAWINGS, BOARD_ITEM* )
DECL_VEC_FOR_SWIG( FP_ZONES, FP_ZONE* )
DECL_VEC_FOR_SWIG( FP_GROUPS, PCB_GROUP* )
DECL_DEQ_FOR_SWIG( FOOTPRINTS, FOOTPRINT* )

class FOOTPRINT : public BOARD_ITEM_CONTAINER
{
public:
    FOOTPRINT( BOARD* parent );

    FOOTPRINT( const FOOTPRINT& aFootprint );

    // Move constructor and operator needed due to std containers inside the footprint
    FOOTPRINT( FOOTPRINT&& aFootprint );

    ~FOOTPRINT();

    FOOTPRINT& operator=( const FOOTPRINT& aOther );
    FOOTPRINT& operator=( FOOTPRINT&& aOther );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_FOOTPRINT_T;
    }

    ///< @copydoc BOARD_ITEM_CONTAINER::Add()
    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_MODE::INSERT ) override;

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
     * Return the bounding box containing pads when the footprint is on the front side,
     * orientation 0, position 0,0.
     *
     * Mainly used in Gerber place file to draw a footprint outline when the courtyard
     * is missing or broken.
     *
     * @return The rectangle containing the pads for the normalized footprint.
     */
    EDA_RECT GetFpPadsLocalBbox() const;

    /**
     * Return a bounding polygon for the shapes and pads in the footprint.
     *
     * This operation is slower but more accurate than calculating a bounding box.
     */
    SHAPE_POLY_SET GetBoundingHull() const;

    // Virtual function
    const EDA_RECT GetBoundingBox() const override;
    const EDA_RECT GetBoundingBox( bool aIncludeText, bool aIncludeInvisibleText ) const;

    PADS& Pads()             { return m_pads; }
    const PADS& Pads() const { return m_pads; }

    DRAWINGS& GraphicalItems()             { return m_drawings; }
    const DRAWINGS& GraphicalItems() const { return m_drawings; }

    FP_ZONES& Zones()             { return m_fp_zones; }
    const FP_ZONES& Zones() const { return m_fp_zones; }

    FP_GROUPS& Groups()             { return m_fp_groups; }
    const FP_GROUPS& Groups() const { return m_fp_groups; }

    bool HasThroughHolePads() const;

    std::list<FP_3DMODEL>& Models()             { return m_3D_Drawings; }
    const std::list<FP_3DMODEL>& Models() const { return m_3D_Drawings; }

    void SetPosition( const wxPoint& aPos ) override;
    wxPoint GetPosition() const override { return m_pos; }

    void SetOrientation( double aNewAngle );
    void SetOrientationDegrees( double aOrientation ) { SetOrientation( aOrientation * 10.0 ); }
    double GetOrientation() const { return m_orient; }
    double GetOrientationDegrees() const { return m_orient / 10.0; }
    double GetOrientationRadians() const { return m_orient * M_PI / 1800; }

    const LIB_ID& GetFPID() const { return m_fpid; }
    void SetFPID( const LIB_ID& aFPID ) { m_fpid = aFPID; }

    const wxString& GetDescription() const { return m_doc; }
    void SetDescription( const wxString& aDoc ) { m_doc = aDoc; }

    const wxString& GetKeywords() const { return m_keywords; }
    void SetKeywords( const wxString& aKeywords ) { m_keywords = aKeywords; }

    const KIID_PATH& GetPath() const { return m_path; }
    void SetPath( const KIID_PATH& aPath ) { m_path = aPath; }

    int GetLocalSolderMaskMargin() const { return m_localSolderMaskMargin; }
    void SetLocalSolderMaskMargin( int aMargin ) { m_localSolderMaskMargin = aMargin; }

    int GetLocalClearance() const { return m_localClearance; }
    void SetLocalClearance( int aClearance ) { m_localClearance = aClearance; }

    int GetLocalClearance( wxString* aSource ) const
    {
        if( aSource )
            *aSource = wxString::Format( _( "footprint %s" ), GetReference() );

        return m_localClearance;
    }

    int GetLocalSolderPasteMargin() const { return m_localSolderPasteMargin; }
    void SetLocalSolderPasteMargin( int aMargin ) { m_localSolderPasteMargin = aMargin; }

    double GetLocalSolderPasteMarginRatio() const { return m_localSolderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( double aRatio ) { m_localSolderPasteMarginRatio = aRatio; }

    void SetZoneConnection( ZONE_CONNECTION aType ) { m_zoneConnection = aType; }
    ZONE_CONNECTION GetZoneConnection() const { return m_zoneConnection; }

    void SetThermalWidth( int aWidth ) { m_thermalWidth = aWidth; }
    int GetThermalWidth() const { return m_thermalWidth; }

    void SetThermalGap( int aGap ) { m_thermalGap = aGap; }
    int GetThermalGap() const { return m_thermalGap; }

    int GetAttributes() const { return m_attributes; }
    void SetAttributes( int aAttributes ) { m_attributes = aAttributes; }

    void SetFlag( int aFlag ) { m_arflag = aFlag; }
    void IncrementFlag() { m_arflag += 1; }
    int GetFlag() const { return m_arflag; }

    // A bit of a hack until net ties are supported as first class citizens
    bool IsNetTie() const
    {
        return GetKeywords().StartsWith( wxT( "net tie" ) );
    }

    void Move( const wxPoint& aMoveVector ) override;

    void Rotate( const wxPoint& aRotCentre, double aAngle ) override;

    void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

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
    void MoveAnchorPosition( const wxPoint& aMoveVector );

    /**
     * @return true if the footprint is flipped, i.e. on the back side of the board
     */
    bool IsFlipped() const { return GetLayer() == B_Cu; }

    /**
     * A special IsOnLayer for footprints: return true if the footprint contains only items on the
     * given layer, even if that layer is not one of the valid footprint layers F_Cu and B_Cu.
     * This allows selection of "graphic" footprints that contain only silkscreen, for example.
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

    /*
    void SetPadsLocked( bool aPadsLocked )
    {
        if( aPadsLocked )
            m_fpStatus |= FP_PADS_are_LOCKED;
        else
            m_fpStatus &= ~FP_PADS_are_LOCKED;
    }
    */

    void SetLastEditTime( timestamp_t aTime ) { m_lastEditTime = aTime; }
    void SetLastEditTime() { m_lastEditTime = time( NULL ); }
    timestamp_t GetLastEditTime() const { return m_lastEditTime; }

    /**
     * Generate pads shapes on layer \a aLayer as polygons and adds these polygons to
     * \a aCornerBuffer.
     *
     * Useful to generate a polygonal representation of a footprint in 3D view and plot functions,
     * when a full polygonal approach is needed.
     *
     * @param aLayer is the layer to consider, or #UNDEFINED_LAYER to consider all layers.
     * @param aCornerBuffer i the buffer to store polygons.
     * @param aClearance is an additional size to add to pad shapes.
     * @param aMaxError is the maximum deviation from true for arcs.
     * @param aSkipNPTHPadsWihNoCopper if true, do not add a NPTH pad shape, if the shape has
     *          same size and position as the hole. Usually, these pads are not drawn on copper
     *          layers, because there is actually no copper
     *          Due to diff between layers and holes, these pads must be skipped to be sure
     *          there is no copper left on the board (for instance when creating Gerber Files or
     *          3D shapes).  Defaults to false.
     * @param aSkipPlatedPads is used on 3D-Viewer to extract plated and non-plated pads.
     * @param aSkipNonPlatedPads is used on 3D-Viewer to extract plated and plated pads.
     */
    void TransformPadsWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                              PCB_LAYER_ID aLayer, int aClearance,
                                              int aMaxError, ERROR_LOC aErrorLoc,
                                              bool aSkipNPTHPadsWihNoCopper = false,
                                              bool aSkipPlatedPads = false,
                                              bool aSkipNonPlatedPads = false ) const;

    /**
     * Generate shapes of graphic items (outlines) on layer \a aLayer as polygons and adds these
     * polygons to \a aCornerBuffer.
     *
     * Useful to generate a polygonal representation of a footprint in 3D view and plot functions,
     * when a full polygonal approach is needed.
     *
     * @param aLayer is the layer to consider, or #UNDEFINED_LAYER to consider all.
     * @param aCornerBuffer is the buffer to store polygons.
     * @param aClearance is a value to inflate shapes.
     * @param aError is the maximum error between true arc and polygon approximation.
     * @param aIncludeText set to true to transform text shapes.
     * @param aIncludeShapes set to true to transform footprint shapes.
     */
    void TransformFPShapesWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                  PCB_LAYER_ID aLayer, int aClearance,
                                                  int aError, ERROR_LOC aErrorLoc,
                                                  bool aIncludeText = true,
                                                  bool aIncludeShapes = true ) const;

    /**
     * This function is the same as TransformGraphicShapesWithClearanceToPolygonSet
     * but only generate text.
     */
    void TransformFPTextWithClearanceToPolygonSet( SHAPE_POLY_SET& aCornerBuffer,
                                                   PCB_LAYER_ID aLayer, int aClearance,
                                                   int aError, ERROR_LOC aErrorLoc ) const
    {
        TransformFPShapesWithClearanceToPolygon( aCornerBuffer, aLayer, aClearance, aError,
                                                 aErrorLoc, true, false );
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

    ///< @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Test if a point is inside the bounding polygon of the footprint.
     *
     * The other hit test methods are just checking the bounding box, which can be quite
     * inaccurate for rotated or oddly-shaped footprints.
     *
     * @param aPosition is the point to test
     * @return true if aPosition is inside the bounding polygon
     */
    bool HitTestAccurate( const wxPoint& aPosition, int aAccuracy = 0 ) const;

    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    /**
     * @return reference designator text.
     */
    const wxString& GetReference() const
    {
        return m_reference->GetText();
    }

    /**
     * @param aReference A reference to a wxString object containing the reference designator
     *                   text.
     */
    void SetReference( const wxString& aReference )
    {
        m_reference->SetText( aReference );
    }

    /**
     * Bump the current reference by \a aDelta.
     */
    void IncrementReference( int aDelta );

    /**
     * @return the value text.
     */
    const wxString& GetValue() const
    {
        return m_value->GetText();
    }

    /**
     * @param aValue A reference to a wxString object containing the value text.
     */
    void SetValue( const wxString& aValue )
    {
        m_value->SetText( aValue );
    }

    /// read/write accessors:
    FP_TEXT& Value()           { return *m_value; }
    FP_TEXT& Reference()       { return *m_reference; }

    /// The const versions to keep the compiler happy.
    FP_TEXT& Value() const     { return *m_value; }
    FP_TEXT& Reference() const { return *m_reference; }

    const std::map<wxString, wxString>& GetProperties() const { return m_properties; }
    void SetProperties( const std::map<wxString, wxString>& aProps ) { m_properties = aProps; }
    const wxString& GetProperty( const wxString& aKey) { return m_properties[ aKey ]; }
    bool HasProperty( const wxString& aKey)
    {
        return m_properties.find( aKey ) != m_properties.end();
    }
    void SetProperty( const wxString& aKey, const wxString& aVal ) { m_properties[ aKey ] = aVal; }

    /**
     * Return a #PAD with a matching name.
     *
     * @note Names may not be unique depending on how the footprint was created.
     *
     * @param aPadName the pad name to find.
     * @return the first matching named #PAD is returned or NULL if not found.
     */
    PAD* FindPadByName( const wxString& aPadName ) const;

    /**
     * Get a pad at \a aPosition on \a aLayerMask in the footprint.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a #PAD object if found otherwise NULL.
     */
    PAD* GetPad( const wxPoint& aPosition, LSET aLayerMask = LSET::AllLayersMask() );

    PAD* GetTopLeftPad();

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
     * Return the next available pad name in the footprint.
     *
     * @param aFillSequenceGaps true if the numbering should "fill in" gaps in the sequence,
     *                          else return the highest value + 1
     * @return the next available pad name
     */
    wxString GetNextPadName( const wxString& aLastPadName ) const;

    double GetArea( int aPadding = 0 ) const;

    KIID GetLink() const { return m_link; }
    void SetLink( const KIID& aLink ) { m_link = aLink; }

    int GetPlacementCost180() const { return m_rot180Cost; }
    void SetPlacementCost180( int aCost )   { m_rot180Cost = aCost; }

    int GetPlacementCost90() const { return m_rot90Cost; }
    void SetPlacementCost90( int aCost )    { m_rot90Cost = aCost; }

    BOARD_ITEM* Duplicate() const override;

    /**
     * Duplicate a given item within the footprint, optionally adding it to the board.
     *
     * @return the new item, or NULL if the item could not be duplicated.
     */
    BOARD_ITEM* DuplicateItem( const BOARD_ITEM* aItem, bool aAddToFootprint = false );

    /**
     * Add \a a3DModel definition to the end of the 3D model list.
     *
     * @param a3DModel A pointer to a #FP_3DMODEL to add to the list.
     */
    void Add3DModel( FP_3DMODEL* a3DModel );

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    wxString GetClass() const override
    {
        return wxT( "FOOTPRINT" );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    /**
     * Invoke a function on all BOARD_ITEMs that belong to the footprint (pads, drawings, texts).
     *
     * @note This function should not add or remove items to the footprint.
     *
     * @param aFunction is the function to be invoked.
     */
    void RunOnChildren( const std::function<void (BOARD_ITEM*)>& aFunction ) const;

    /**
     * Return a set of all layers that this footprint has drawings on similar to ViewGetLayers().
     *
     * @param aLayers is an array to store layer ids.
     * @param aCount is the number of layers stored in the array.
     * @param aIncludePads controls whether to also include pad layers.
     */
    void GetAllDrawingLayers( int aLayers[], int& aCount, bool aIncludePads = true ) const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

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

    ///< Return the initial comments block or NULL if none, without transfer of ownership.
    const wxArrayString* GetInitialComments() const { return m_initial_comments; }

    /**
     * Used in DRC to test the courtyard area (a complex polygon).
     *
     * @return the courtyard polygon.
     */
    const SHAPE_POLY_SET& GetPolyCourtyardFront() const { return m_poly_courtyard_front; }
    const SHAPE_POLY_SET& GetPolyCourtyardBack() const { return m_poly_courtyard_back; }

    /**
     * Build complex polygons of the courtyard areas from graphic items on the courtyard layers.
     *
     * @note Set the #MALFORMED_F_COURTYARD and #MALFORMED_B_COURTYARD status flags if the given
     *       courtyard layer does not contain a (single) closed shape.
     */
    void BuildPolyCourtyards( OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );

    virtual std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER ) const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

    struct cmp_drawings
    {
        bool operator()( const BOARD_ITEM* aFirst, const BOARD_ITEM* aSecond ) const;
    };

    struct cmp_pads
    {
        bool operator()( const PAD* aFirst, const PAD* aSecond ) const;
    };


#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    DRAWINGS        m_drawings;          // BOARD_ITEMs for drawings on the board, owned by pointer.
    PADS            m_pads;              // PAD items, owned by pointer
    FP_ZONES        m_fp_zones;          // FP_ZONE items, owned by pointer
    FP_GROUPS       m_fp_groups;         // PCB_GROUP items, owned by pointer

    double          m_orient;            // Orientation in tenths of a degree, 900=90.0 degrees.
    wxPoint         m_pos;               // Position of footprint on the board in internal units.
    FP_TEXT*        m_reference;         // Component reference designator value (U34, R18..)
    FP_TEXT*        m_value;             // Component value (74LS00, 22K..)
    LIB_ID          m_fpid;              // The #LIB_ID of the FOOTPRINT.
    int             m_attributes;        // Flag bits ( see FOOTPRINT_ATTR_T )
    int             m_fpStatus;          // For autoplace: flags (LOCKED, FIELDS_AUTOPLACED)

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
    mutable EDA_RECT       m_cachedBoundingBox;
    mutable int            m_boundingBoxCacheTimeStamp;
    mutable EDA_RECT       m_cachedVisibleBBox;
    mutable int            m_visibleBBoxCacheTimeStamp;
    mutable EDA_RECT       m_cachedTextExcludedBBox;
    mutable int            m_textExcludedBBoxCacheTimeStamp;
    mutable SHAPE_POLY_SET m_cachedHull;
    mutable int            m_hullCacheTimeStamp;

    ZONE_CONNECTION m_zoneConnection;
    int             m_thermalWidth;
    int             m_thermalGap;
    int             m_localClearance;
    int             m_localSolderMaskMargin;       // Solder mask margin
    int             m_localSolderPasteMargin;      // Solder paste margin absolute value
    double          m_localSolderPasteMarginRatio; // Solder mask margin ratio value of pad size

    wxString        m_doc;               // File name and path for documentation file.
    wxString        m_keywords;          // Search keywords to find footprint in library.
    KIID_PATH       m_path;              // Path to associated symbol ([sheetUUID, .., symbolUUID]).
    timestamp_t     m_lastEditTime;
    int             m_arflag;            // Use to trace ratsnest and auto routing.
    KIID            m_link;              // Temporary logical link used during editing
    int             m_rot90Cost;         // Horizontal automatic placement cost ( 0..10 ).
    int             m_rot180Cost;        // Vertical automatic placement cost ( 0..10 ).

    std::list<FP_3DMODEL>         m_3D_Drawings;       // Linked list of 3D models.
    std::map<wxString, wxString>  m_properties;
    wxArrayString*                m_initial_comments;  // s-expression comments in the footprint,
                                                       // lazily allocated only if needed for speed

    SHAPE_POLY_SET  m_poly_courtyard_front;  // Note that a footprint can have both front and back
    SHAPE_POLY_SET  m_poly_courtyard_back;   // courtyards populated.
};

#endif     // FOOTPRINT_H
