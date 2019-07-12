/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file class_module.h
 * @brief Module description (excepted pads)
 */


#ifndef MODULE_H_
#define MODULE_H_

#include <deque>

#include <board_item_container.h>
#include <class_board_item.h>
#include <collectors.h>
#include <convert_to_biu.h>
#include <layers_id_colors_and_visibility.h> // ALL_LAYERS definition.
#include <lib_id.h>
#include <list>

#include <class_text_mod.h>
#include "zones.h"

#include <core/iterators.h>

#include <functional>

class LINE_READER;
class EDA_3D_CANVAS;
class D_PAD;
class BOARD;
class MSG_PANEL_ITEM;

namespace KIGFX {
class VIEW;
}

enum INCLUDE_NPTH_T
{
    DO_NOT_INCLUDE_NPTH = false,
    INCLUDE_NPTH = true
};

/**
 * Enum MODULE_ATTR_T
 * is the set of attributes allowed within a MODULE, using MODULE::SetAttributes()
 * and MODULE::GetAttributes().  These are to be ORed together when calling
 * MODULE::SetAttributes()
 */
enum MODULE_ATTR_T
{
    MOD_DEFAULT = 0,    ///< default
    MOD_CMS     = 1,    ///< Set for modules listed in the automatic insertion list
                        ///< (usually SMD footprints)
    MOD_VIRTUAL = 2     ///< Virtual component: when created by copper shapes on
                        ///<  board (Like edge card connectors, mounting hole...)
};

class MODULE_3D_SETTINGS
{
    public:
        MODULE_3D_SETTINGS() :
            // Initialize with sensible values
            m_Scale { 1, 1, 1 },
            m_Rotation { 0, 0, 0 },
            m_Offset { 0, 0, 0 },
            m_Preview( true )
        {
        }

        struct VECTOR3D
        {
            double x, y, z;
        };

        VECTOR3D m_Scale;       ///< 3D model scaling factor (dimensionless)
        VECTOR3D m_Rotation;    ///< 3D model rotation (degrees)
        VECTOR3D m_Offset;      ///< 3D model offset (mm)
        wxString m_Filename;    ///< The 3D shape filename in 3D library
        bool     m_Preview;     ///< Include module in 3D preview
};

DECL_DEQ_FOR_SWIG( PADS, D_PAD* )
DECL_DEQ_FOR_SWIG( DRAWINGS, BOARD_ITEM* )
DECL_DEQ_FOR_SWIG( MODULES, MODULE* )

class MODULE : public BOARD_ITEM_CONTAINER
{
public:
    MODULE( BOARD* parent );

    MODULE( const MODULE& aModule );

    ~MODULE();

    MODULE& operator=( const MODULE& aOther );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_MODULE_T == aItem->Type();
    }

    ///> @copydoc BOARD_ITEM_CONTAINER::Add()
    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_INSERT ) override;

    ///> @copydoc BOARD_ITEM_CONTAINER::Remove()
    void Remove( BOARD_ITEM* aItem ) override;

    /**
     * Function ClearAllNets
     * Clear (i.e. force the ORPHANED dummy net info) the net info which
     * depends on a given board for all pads of the footprint.
     * This is needed when a footprint is copied between the fp editor and
     * the board editor for instance, because net info become fully broken
     */
    void ClearAllNets();

    /**
     * Function CalculateBoundingBox
     * calculates the bounding box in board coordinates.
     */
    void CalculateBoundingBox();

    /**
     * Function GetFootprintRect()
     * Returns the area of the module footprint excluding any text.
     * @return EDA_RECT - The rectangle containing the footprint.
     */
    EDA_RECT GetFootprintRect() const;

    /**
     * Returns a bounding polygon for the shapes and pads in the module
     * This operation is slower but more accurate than calculating a bounding box
     */
    SHAPE_POLY_SET GetBoundingPoly() const;

    // Virtual function
    const EDA_RECT GetBoundingBox() const override;

    PADS& Pads()
    {
         return m_pads;
    }

    const PADS& Pads() const
    {
         return m_pads;
    }

    DRAWINGS& GraphicalItems()
    {
        return m_drawings;
    }

    const DRAWINGS& GraphicalItems() const
    {
        return m_drawings;
    }

    std::list<MODULE_3D_SETTINGS>& Models()             { return m_3D_Drawings; }
    const std::list<MODULE_3D_SETTINGS>& Models() const { return m_3D_Drawings; }

    void SetPosition( const wxPoint& aPos ) override;

    const wxPoint GetPosition() const override { return m_Pos; }

    void SetOrientation( double newangle );

    void SetOrientationDegrees( double aOrientation ) { SetOrientation( aOrientation * 10.0 ); }
    double GetOrientation() const { return m_Orient; }
    double GetOrientationDegrees() const { return m_Orient / 10.0; }
    double GetOrientationRadians() const { return m_Orient * M_PI / 1800; }

    const LIB_ID& GetFPID() const { return m_fpid; }
    void SetFPID( const LIB_ID& aFPID ) { m_fpid = aFPID; }

    const wxString& GetDescription() const { return m_Doc; }
    void SetDescription( const wxString& aDoc ) { m_Doc = aDoc; }

    const wxString& GetKeywords() const { return m_KeyWord; }
    void SetKeywords( const wxString& aKeywords ) { m_KeyWord = aKeywords; }

    const wxString& GetPath() const { return m_Path; }
    void SetPath( const wxString& aPath ) { m_Path = aPath; }

    int GetLocalSolderMaskMargin() const { return m_LocalSolderMaskMargin; }
    void SetLocalSolderMaskMargin( int aMargin ) { m_LocalSolderMaskMargin = aMargin; }

    int GetLocalClearance() const { return m_LocalClearance; }
    void SetLocalClearance( int aClearance ) { m_LocalClearance = aClearance; }

    int GetLocalSolderPasteMargin() const { return m_LocalSolderPasteMargin; }
    void SetLocalSolderPasteMargin( int aMargin ) { m_LocalSolderPasteMargin = aMargin; }

    double GetLocalSolderPasteMarginRatio() const { return m_LocalSolderPasteMarginRatio; }
    void SetLocalSolderPasteMarginRatio( double aRatio ) { m_LocalSolderPasteMarginRatio = aRatio; }

    void SetZoneConnection( ZoneConnection aType ) { m_ZoneConnection = aType; }
    ZoneConnection GetZoneConnection() const { return m_ZoneConnection; }

    void SetThermalWidth( int aWidth ) { m_ThermalWidth = aWidth; }
    int GetThermalWidth() const { return m_ThermalWidth; }

    void SetThermalGap( int aGap ) { m_ThermalGap = aGap; }
    int GetThermalGap() const { return m_ThermalGap; }

    int GetAttributes() const { return m_Attributs; }
    void SetAttributes( int aAttributes ) { m_Attributs = aAttributes; }

    void SetFlag( int aFlag ) { m_arflag = aFlag; }
    void IncrementFlag() { m_arflag += 1; }
    int GetFlag() const { return m_arflag; }

    // A bit of a hack until net ties are supported as first class citizens
    bool IsNetTie() const { return GetKeywords().StartsWith( wxT( "net tie" ) ); }

    void Move( const wxPoint& aMoveVector ) override;

    void Rotate( const wxPoint& aRotCentre, double aAngle ) override;

    void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    /**
     * Function MoveAnchorPosition
     * Move the reference point of the footprint
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
     * function IsFlipped
     * @return true if the module is flipped, i.e. on the back side of the board
     */
    bool IsFlipped() const { return GetLayer() == B_Cu; }

// m_ModuleStatus bits:
#define MODULE_is_LOCKED    0x01        ///< module LOCKED: no autoplace allowed
#define MODULE_is_PLACED    0x02        ///< In autoplace: module automatically placed
#define MODULE_to_PLACE     0x04        ///< In autoplace: module waiting for autoplace
#define MODULE_PADS_LOCKED  0x08        ///< In autoplace: module waiting for autoplace


    bool IsLocked() const override
    {
        return ( m_ModuleStatus & MODULE_is_LOCKED ) != 0;
    }

    /**
     * Function SetLocked
     * sets the MODULE_is_LOCKED bit in the m_ModuleStatus
     * @param isLocked When true means turn on locked status, else unlock
     */
    void SetLocked( bool isLocked ) override
    {
        if( isLocked )
            m_ModuleStatus |= MODULE_is_LOCKED;
        else
            m_ModuleStatus &= ~MODULE_is_LOCKED;
    }

    bool IsPlaced() const { return m_ModuleStatus & MODULE_is_PLACED;  }
    void SetIsPlaced( bool isPlaced )
    {
        if( isPlaced )
            m_ModuleStatus |= MODULE_is_PLACED;
        else
            m_ModuleStatus &= ~MODULE_is_PLACED;
    }

    bool NeedsPlaced() const { return m_ModuleStatus & MODULE_to_PLACE;  }
    void SetNeedsPlaced( bool needsPlaced )
    {
        if( needsPlaced )
            m_ModuleStatus |= MODULE_to_PLACE;
        else
            m_ModuleStatus &= ~MODULE_to_PLACE;
    }

    bool PadsLocked() const { return m_ModuleStatus & MODULE_PADS_LOCKED;  }

    void SetPadsLocked( bool aPadsLocked )
    {
        if( aPadsLocked )
            m_ModuleStatus |= MODULE_PADS_LOCKED;
        else
            m_ModuleStatus &= ~MODULE_PADS_LOCKED;
    }

    void SetLastEditTime( timestamp_t aTime ) { m_LastEditTime = aTime; }
    void SetLastEditTime() { m_LastEditTime = time( NULL ); }
    timestamp_t GetLastEditTime() const { return m_LastEditTime; }

    /* drawing functions */

    /**
     * Function Print
     * Prints the footprint to the \a aDC.
     * @param aFrame = the current Frame
     * @param aDC = Current Device Context
     * @param aOffset = draw offset (usually wxPoint(0,0)
     */
    void Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset = ZeroOffset ) override;

    /**
     * function TransformPadsShapesWithClearanceToPolygon
     * generate pads shapes on layer aLayer as polygons,
     * and adds these polygons to aCornerBuffer
     * Useful to generate a polygonal representation of a footprint
     * in 3D view and plot functions, when a full polygonal approach is needed
     * @param aLayer = the layer to consider, or UNDEFINED_LAYER to consider all
     * @param aCornerBuffer = the buffer to store polygons
     * @param aInflateValue = an additionnal size to add to pad shapes
     *          aInflateValue = 0 to have the exact pad size
     * @param aMaxError = Maximum deviation from true for arcs
     * @param aSkipNPTHPadsWihNoCopper = if true, do not add a NPTH pad shape,
     *  if the shape has same size and position as the hole. Usually, these
     *  pads are not drawn on copper layers, because there is actually no copper
     *  Due to diff between layers and holes, these pads must be skipped to be sure
     *  there is no copper left on the board (for instance when creating Gerber Files or 3D shapes)
     *  default = false
     */
    void TransformPadsShapesWithClearanceToPolygon( PCB_LAYER_ID aLayer,
            SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aMaxError = ARC_HIGH_DEF,
            bool aSkipNPTHPadsWihNoCopper = false ) const;

    /**
     * function TransformGraphicShapesWithClearanceToPolygonSet
     * generate shapes of graphic items (outlines) on layer aLayer as polygons,
     * and adds these polygons to aCornerBuffer
     * Useful to generate a polygonal representation of a footprint
     * in 3D view and plot functions, when a full polygonal approach is needed
     * @param aLayer = the layer to consider, or UNDEFINED_LAYER to consider all
     * @param aCornerBuffer = the buffer to store polygons
     * @param aInflateValue = a value to inflate shapes
     *          aInflateValue = 0 to have the exact shape size
     * @param aError = Maximum error between true arc and polygon approx
     * @param aIncludeText = True to transform text shapes
     */
    void TransformGraphicShapesWithClearanceToPolygonSet( PCB_LAYER_ID aLayer,
            SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aError = ARC_HIGH_DEF,
            bool aIncludeText = true ) const;

    /**
     * @brief TransformGraphicTextWithClearanceToPolygonSet
     * This function is the same as TransformGraphicShapesWithClearanceToPolygonSet
     * but only generate text
     * @param aLayer = the layer to consider, or UNDEFINED_LAYER to consider all
     * @param aCornerBuffer = the buffer to store polygons
     * @param aInflateValue = a value to inflate shapes
     *          aInflateValue = 0 to have the exact shape size
     * @param aError = Maximum error between true arc and polygon approx
     */
    void TransformGraphicTextWithClearanceToPolygonSet( PCB_LAYER_ID aLayer,
            SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aError = ARC_HIGH_DEF ) const;

    ///> @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Tests if a point is inside the bounding polygon of the module
     *
     * The other hit test methods are just checking the bounding box, which
     * can be quite inaccurate for rotated or oddly-shaped footprints.
     *
     * @param aPosition is the point to test
     * @return true if aPosition is inside the bounding polygon
     */
    bool HitTestAccurate( const wxPoint& aPosition, int aAccuracy = 0 ) const;

    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    /**
     * Function GetReference
     * @return const wxString& - the reference designator text.
     */
    const wxString GetReference() const
    {
        return m_Reference->GetText();
    }

    /**
     * Function SetReference
     * @param aReference A reference to a wxString object containing the reference designator
     *                   text.
     */
    void SetReference( const wxString& aReference )
    {
        m_Reference->SetText( aReference );
    }

    /**
     * Function IncrementReference
     * Bumps the current reference by aDelta.
     */
    void IncrementReference( int aDelta );

    /**
     * Function GetValue
     * @return const wxString& - the value text.
     */
    const wxString GetValue() const
    {
        return m_Value->GetText();
    }

    /**
     * Function SetValue
     * @param aValue A reference to a wxString object containing the value text.
     */
    void SetValue( const wxString& aValue )
    {
        m_Value->SetText( aValue );
    }

    /// read/write accessors:
    TEXTE_MODULE& Value()       { return *m_Value; }
    TEXTE_MODULE& Reference()   { return *m_Reference; }

    /// The const versions to keep the compiler happy.
    TEXTE_MODULE& Value() const { return *m_Value; }
    TEXTE_MODULE& Reference() const { return *m_Reference; }

    /**
     * Function FindPadByName
     * returns a D_PAD* with a matching name.  Note that names may not be
     * unique, depending on how the foot print was created.
     * @param aPadName the pad name to find
     * @return D_PAD* - The first matching name is returned, or NULL if not found.
     */
    D_PAD* FindPadByName( const wxString& aPadName ) const;

    /**
     * Function GetPad
     * get a pad at \a aPosition on \a aLayerMask in the footprint.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found otherwise NULL.
     */
    D_PAD* GetPad( const wxPoint& aPosition, LSET aLayerMask = LSET::AllLayersMask() );

    D_PAD* GetTopLeftPad();

    /**
     * Gets the first pad in the list or NULL if none
     * @return first pad or null pointer
     */
    D_PAD* GetFirstPad() const
    {
        return m_pads.empty() ? nullptr : m_pads.front();
    }

    /**
     * GetPadCount
     * returns the number of pads.
     *
     * @param aIncludeNPTH includes non-plated through holes when true.  Does not include
     *                     non-plated through holes when false.
     * @return the number of pads according to \a aIncludeNPTH.
     */
    unsigned GetPadCount( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH_T(INCLUDE_NPTH) ) const;

    /**
     * GetUniquePadCount
     * returns the number of unique pads.
     * A complex pad can be built with many pads having the same pad name
     * to create a complex shape or fragmented solder paste areas.
     *
     * GetUniquePadCount calculate the count of not blank pad names
     *
     * @param aIncludeNPTH includes non-plated through holes when true.  Does not include
     *                     non-plated through holes when false.
     * @return the number of unique pads according to \a aIncludeNPTH.
     */
    unsigned GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH = INCLUDE_NPTH_T(INCLUDE_NPTH) ) const;

    /**
     * Function GetNextPadName
     * returns the next available pad name in the module
     *
     * @param aFillSequenceGaps true if the numbering should "fill in" gaps in the sequence,
     *                          else return the highest value + 1
     * @return the next available pad name
     */
    wxString GetNextPadName( bool aFillSequenceGaps ) const;

    double GetArea( int aPadding = 0 ) const;

    timestamp_t GetLink() const { return m_Link; }
    void SetLink( timestamp_t aLink )            { m_Link = aLink; }

    int GetPlacementCost180() const { return m_CntRot180; }
    void SetPlacementCost180( int aCost )   { m_CntRot180 = aCost; }

    int GetPlacementCost90() const { return m_CntRot90; }
    void SetPlacementCost90( int aCost )    { m_CntRot90 = aCost; }

    /**
     * Function Duplicate
     * Duplicate a given item within the module, without adding to the board
     * @return the new item, or NULL if the item could not be duplicated
     */
    BOARD_ITEM* Duplicate( const BOARD_ITEM* aItem,
            bool aIncrementPadNumbers,
            bool aAddToModule = false );

    /**
     * Function Add3DModel
     * adds \a a3DModel definition to the end of the 3D model list.
     *
     * @param a3DModel A pointer to a #MODULE_3D_SETTINGS to add to the list.
     */
    void Add3DModel( MODULE_3D_SETTINGS* a3DModel );

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    wxString GetClass() const override
    {
        return wxT( "MODULE" );
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    /**
     * Function RunOnChildren
     *
     * Invokes a function on all BOARD_ITEMs that belong to the module (pads, drawings, texts).
     * Note that this function should not add or remove items to the module
     * @param aFunction is the function to be invoked.
     */
    void RunOnChildren( const std::function<void (BOARD_ITEM*)>& aFunction );

    /**
     * Returns a set of all layers that this module has drawings on
     * similar to ViewGetLayers()
     *
     * @param aLayers is an array to store layer ids
     * @param aCount is the number of layers stored in the array
     * @param aIncludePads controls whether to also include pad layers
     */
    void GetAllDrawingLayers( int aLayers[], int& aCount, bool aIncludePads = true ) const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual unsigned int ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    virtual const BOX2I ViewBBox() const override;

    /**
     * static function IsLibNameValid
     * Test for validity of a name of a footprint to be used in a footprint library
     * ( no spaces, dir separators ... )
     * @param aName = the name in library to validate
     * @return true if the given name is valid
     */
    static bool IsLibNameValid( const wxString& aName );

    /**
     * static function StringLibNameInvalidChars
     * Test for validity of the name in a library of the footprint
     * ( no spaces, dir separators ... )
     * @param aUserReadable = false to get the list of invalid chars
     *        true to get a readable form (i.e ' ' = 'space' '\\t'= 'tab')
     * @return a constant std::string giving the list of invalid chars in lib name
     */
    static const wxChar* StringLibNameInvalidChars( bool aUserReadable );

    /**
     * Function SetInitialComments
     * takes ownership of caller's heap allocated aInitialComments block.  The comments
     * are single line strings already containing the s-expression comments with optional
     * leading whitespace and then a '#' character followed by optional single line text
     * (text with no line endings, not even one).
     * This block of single line comments will be output upfront of any generated
     * s-expression text in the PCBIO::Format() function.
     * <p>
     * Note that a block of single line comments constitutes a multiline block of single
     * line comments.  That is, the block is made of consecutive single line comments.
     * @param aInitialComments is a heap allocated wxArrayString or NULL, which the caller
     *                         gives up ownership of over to this MODULE.
     */
    void SetInitialComments( wxArrayString* aInitialComments )
    {
        delete m_initial_comments;
        m_initial_comments = aInitialComments;
    }

    /**
     * Function CoverageRatio
     * Calculates the ratio of total area of the footprint pads and graphical items
     * to the area of the footprint. Used by selection tool heuristics.
     * @return the ratio
     */
    double CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const;

    /// Return the initial comments block or NULL if none, without transfer of ownership.
    const wxArrayString* GetInitialComments() const { return m_initial_comments; }

    /** Used in DRC to test the courtyard area (a complex polygon)
     * @return the courtyard polygon
     */
    SHAPE_POLY_SET& GetPolyCourtyardFront() { return m_poly_courtyard_front; }
    SHAPE_POLY_SET& GetPolyCourtyardBack() { return m_poly_courtyard_back; }

    /** Used in DRC to build the courtyard area (a complex polygon)
     * from graphic items put on the courtyard
     * @return true if OK, or no courtyard defined,
     * false only if the polygon cannot be built due to amalformed courtyard shape
     * The polygon cannot be built if segments/arcs on courtyard layers
     * cannot be grouped in a polygon.
     */
    bool BuildPolyCourtyard();

    virtual void SwapData( BOARD_ITEM* aImage ) override;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:

    /// BOARD_ITEMs for drawings on the board, owned by pointer.
    DRAWINGS                m_drawings;

    /// D_PAD items, owned by pointer
    PADS                    m_pads;

    std::list<MODULE_3D_SETTINGS> m_3D_Drawings;  ///< Linked list of 3D models.
    double m_Orient;                    ///< Orientation in tenths of a degree, 900=90.0 degrees.
    wxPoint m_Pos;                      ///< Position of module on the board in internal units.
    TEXTE_MODULE* m_Reference;          ///< Component reference designator value (U34, R18..)
    TEXTE_MODULE* m_Value;              ///< Component value (74LS00, 22K..)
    LIB_ID m_fpid;                      ///< The #LIB_ID of the MODULE.
    int m_Attributs;                    ///< Flag bits ( see Mod_Attribut )
    int m_ModuleStatus;                 ///< For autoplace: flags (LOCKED, AUTOPLACED)
    EDA_RECT m_BoundaryBox;             ///< Bounding box : coordinates on board, real orientation.

    // The final margin is the sum of these 2 values
    int m_ThermalWidth;
    int m_ThermalGap;
    wxString m_Doc;             ///< File name and path for documentation file.
    wxString m_KeyWord;         ///< Search keywords to find module in library.
    wxString m_Path;
    ZoneConnection m_ZoneConnection;
    timestamp_t m_LastEditTime;
    int m_arflag;           ///< Use to trace ratsnest and auto routing.
    timestamp_t m_Link;     ///< Temporary logical link used during editing
    int m_CntRot90;         ///< Horizontal automatic placement cost ( 0..10 ).
    int m_CntRot180;        ///< Vertical automatic placement cost ( 0..10 ).

    // Local tolerances. When zero, this means the corresponding netclass value
    // is used. Usually theses local tolerances zero, in deference to the
    // corresponding netclass values.
    int m_LocalClearance;
    int m_LocalSolderMaskMargin;            ///< Solder mask margin
    int m_LocalSolderPasteMargin;           ///< Solder paste margin absolute value
    double m_LocalSolderPasteMarginRatio;   ///< Solder mask margin ratio
                                            ///< value of pad size

    wxArrayString* m_initial_comments;      ///< leading s-expression comments in the module,
                                            ///< lazily allocated only if needed for speed

    /// Used in DRC to test the courtyard area (a polygon which can be not basic
    /// Note also a footprint can have courtyards on bot board sides
    SHAPE_POLY_SET m_poly_courtyard_front;
    SHAPE_POLY_SET m_poly_courtyard_back;
};

#endif     // MODULE_H_
