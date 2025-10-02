/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#ifndef PCB_DIMENSION_H
#define PCB_DIMENSION_H


#include <board_item.h>
#include <pcb_text.h>
#include <geometry/shape.h>
#include <geometry/circle.h>

class LINE_READER;
class MSG_PANEL_ITEM;


/// How to display the units in a dimension's text
enum class DIM_UNITS_FORMAT
{
    NO_SUFFIX,      // 1234.0
    BARE_SUFFIX,    // 1234.0 mm
    PAREN_SUFFIX    // 1234.0 (mm)
};

enum class DIM_PRECISION
{
    X = 0,          // 0
    X_X,            // 0.0
    X_XX,           // 0.00
    X_XXX,          // 0.000
    X_XXXX,         // 0.0000
    X_XXXXX,        // 0.00000
    V_VV,           // 0.00 / 0 / 0.0
    V_VVV,          // 0.000 / 0 / 0.00
    V_VVVV,         // 0.0000 / 0.0 / 0.000
    V_VVVVV         // 0.00000 / 0.00 / 0.0000
};

/// Where to place the text on a dimension
enum class DIM_TEXT_POSITION
{
    OUTSIDE,    ///< Text appears outside the dimension line (default)
    INLINE,     ///< Text appears in line with the dimension line
    MANUAL      ///< Text placement is manually set by the user
};

/**
 * Used for storing the units selection in the file because EDA_UNITS alone doesn't cut it
 */
enum class DIM_UNITS_MODE
{
    INCH,       // Do not use IN: it conflicts with a Windows header
    MILS,
    MM,
    AUTOMATIC
};

/**
* Used for dimension's arrow.
*/
enum class DIM_ARROW_DIRECTION
{
    INWARD,  ///< >-----<
    OUTWARD, ///< <----->
};

/**
 * Frame to show around dimension text
 */
enum class DIM_TEXT_BORDER
{
    NONE,
    RECTANGLE,
    CIRCLE,
    ROUNDRECT
};

/**
 * Abstract dimension API
 *
 * Some notes about dimension nomenclature:
 *
 * - "feature points" are the points being measured by the dimension.  For an example, the start
 *   and end points of a line to be measured.  These are the first points picked when drawing a
 *   new dimension.  Dimensions can have one or more feature points: linear dimensions (the only
 *   type supported in KiCad 5 and earlier) have two feature points; leader dimensions have one;
 *   and ordinate dimensions can have in theory an unlimited number of feature points.
 *
 * - "feature lines" are lines that coincide with feature points.  Not all dimension types have
 *   feature lines.  The actual start and end of feature lines is calculated from dimension style
 *   properties (offset from feature point to start of feature line, height of crossbar, and height
 *   of feature line past crossbar, for example in linear dimensions)
 *
 * - "crossbar" refers to the perpendicular line (usually with arrows at each end) between feature
 *   lines on linear dimensions
 */
class PCB_DIMENSION_BASE : public PCB_TEXT
{
public:
    PCB_DIMENSION_BASE( BOARD_ITEM* aParent, KICAD_T aType = PCB_DIMENSION_T );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    /**
     * The dimension's origin is the first feature point for the dimension.  Every dimension has
     * one or more feature points, so every dimension has at least an origin.
     * @return the origin point of this dimension
     */
    virtual const VECTOR2I& GetStart() const { return m_start; }
    virtual void            SetStart( const VECTOR2I& aPoint ) { m_start = aPoint; }

    virtual const VECTOR2I& GetEnd() const { return m_end; }
    virtual void            SetEnd( const VECTOR2I& aPoint ) { m_end = aPoint; }

    VECTOR2I GetPosition() const override { return m_start; }
    void     SetPosition( const VECTOR2I& aPos ) override { m_start = aPos; }

    bool GetOverrideTextEnabled() const { return m_overrideTextEnabled; }
    void SetOverrideTextEnabled( bool aOverride ) { m_overrideTextEnabled = aOverride; }

    wxString GetOverrideText() const { return m_valueString; }
    void SetOverrideText( const wxString& aValue ) { m_valueString = aValue; }

    void ChangeOverrideText( const wxString& aValue )
    {
        SetOverrideTextEnabled( true );
        SetOverrideText( aValue );
        Update();
    }

    int GetMeasuredValue() const { return m_measuredValue; }

    // KiCad normally calculates the measured value but some importers need to set it.
    void SetMeasuredValue( int aValue ) { m_measuredValue = aValue; }

    /**
     * @return the dimension value, rendered with precision / zero suppression but no units, etc
     */
    wxString GetValueText() const;

    /**
     * Update the dimension's cached text and geometry.
     *
     * Call this whenever you change something in the geometry
     * definition, or the text (which can affect geometry, e.g. by
     * a knockout of a crossbar line or similar)
     */
    void Update()
    {
        // Calls updateText internally
        updateGeometry();
    }

    void UpdateUnits()
    {
        SetUnitsMode( GetUnitsMode() );
        Update();
    }

    wxString GetPrefix() const { return m_prefix; }
    void SetPrefix( const wxString& aPrefix );

    void ChangePrefix( const wxString& aPrefix )
    {
        SetPrefix( aPrefix );
        Update();
    }

    wxString GetSuffix() const { return m_suffix; }
    void SetSuffix( const wxString& aSuffix );

    void ChangeSuffix( const wxString& aSuffix )
    {
        SetSuffix( aSuffix );
        Update();
    }

    DIM_ARROW_DIRECTION GetArrowDirection() const { return m_arrowDirection; }
    void                SetArrowDirection( const DIM_ARROW_DIRECTION& aDirection )
    {
        m_arrowDirection = aDirection;
    }

    void ChangeArrowDirection( const DIM_ARROW_DIRECTION& aDirection )
    {
        SetArrowDirection( aDirection );
        updateText();
        updateGeometry();
    }

    EDA_UNITS GetUnits() const { return m_units; }
    void SetUnits( EDA_UNITS aUnits );

    DIM_UNITS_MODE GetUnitsMode() const;
    void SetUnitsMode( DIM_UNITS_MODE aMode );

    void ChangeUnitsMode( DIM_UNITS_MODE aMode )
    {
        SetUnitsMode( aMode );
        Update();
    }

    void SetAutoUnits( bool aAuto = true ) { m_autoUnits = aAuto; }

    DIM_UNITS_FORMAT GetUnitsFormat() const { return m_unitsFormat; }
    void SetUnitsFormat( const DIM_UNITS_FORMAT aFormat ) { m_unitsFormat = aFormat; }

    void ChangeUnitsFormat( const DIM_UNITS_FORMAT aFormat )
    {
        SetUnitsFormat( aFormat );
        Update();
    }

    DIM_PRECISION GetPrecision() const { return m_precision; }
    void SetPrecision( DIM_PRECISION aPrecision ) { m_precision = aPrecision; }

    void ChangePrecision( DIM_PRECISION aPrecision )
    {
        SetPrecision( aPrecision );
        Update();
    }

    bool GetSuppressZeroes() const { return m_suppressZeroes; }
    void SetSuppressZeroes( bool aSuppress ) { m_suppressZeroes = aSuppress; }

    void ChangeSuppressZeroes( bool aSuppress )
    {
        SetSuppressZeroes( aSuppress );
        Update();
    }

    bool GetKeepTextAligned() const { return m_keepTextAligned; }
    void SetKeepTextAligned( bool aKeepAligned ) { m_keepTextAligned = aKeepAligned; }

    double GetTextAngleDegreesProp() const { return GetTextAngleDegrees(); }
    void ChangeTextAngleDegrees( double aDegrees );
    void ChangeKeepTextAligned( bool aKeepAligned );

    void SetTextPositionMode( DIM_TEXT_POSITION aMode ) { m_textPosition = aMode; }
    DIM_TEXT_POSITION GetTextPositionMode() const { return m_textPosition; }

    int GetArrowLength() const { return m_arrowLength; }
    void SetArrowLength( int aLength ) { m_arrowLength = aLength; }

    void SetExtensionOffset( int aOffset ) { m_extensionOffset = aOffset; }
    int GetExtensionOffset() const { return m_extensionOffset; }

    int GetLineThickness() const        { return m_lineThickness; }
    void SetLineThickness( int aWidth ) { m_lineThickness = aWidth; }

    void StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide ) override;

    /**
     * @return a list of line segments that make up this dimension (for drawing, plotting, etc).
     */
    const std::vector<std::shared_ptr<SHAPE>>& GetShapes() const { return m_shapes; }

    // BOARD_ITEM overrides

    void Move( const VECTOR2I& offset ) override;
    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /**
     * Mirror the dimension relative to a given horizontal axis.
     *
     * The text is not mirrored.  Only its position (and angle) is mirrored.  The layer is not
     * changed.
     *
     * @param axis_pos is the vertical axis position to mirror around.
     */
    virtual void Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    const BOX2I GetBoundingBox() const override;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    const BOX2I ViewBBox() const override;

    void ClearRenderCache() override;

    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                  int aError, ERROR_LOC aErrorLoc,
                                  bool aIgnoreLineWidth = false ) const override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_DIMENSION_BASE& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:

    /**
     * Update the cached geometry of the dimension after changing any of its properties.
     */
    virtual void updateGeometry() = 0;

    /**
     * Update the text field value from the current geometry (called by updateGeometry normally).
     *
     * If you change the text, you should call updateGeometry which will call this,
     * and also handle any text-dependent geoemtry handling (like a knockout)
     */
    virtual void updateText();

    template<typename ShapeType>
    void addShape( const ShapeType& aShape );

    /**
     * Draws an arrow and updates the shape container.
     * example arrow 0Deg tail:4  (---->)
     *
     * @param startPoint arrow point.
     * @param anAngle arrow angle.
     * @param aLength arrow tail length.
     */
    void drawAnArrow( VECTOR2I aStartPoint, EDA_ANGLE anAngle, int aLength );

    // Value format
    bool                    m_overrideTextEnabled;   ///< Manually specify the displayed measurement value
    wxString                m_valueString;     ///< Displayed value when m_overrideValue = true
    wxString                m_prefix;          ///< String prepended to the value
    wxString                m_suffix;          ///< String appended to the value
    EDA_UNITS               m_units;           ///< 0 = inches, 1 = mm
    bool                    m_autoUnits;       ///< If true, follow the currently selected UI units
    DIM_UNITS_FORMAT        m_unitsFormat;     ///< How to render the units suffix
    DIM_ARROW_DIRECTION     m_arrowDirection;  ///< direction of dimension arrow.
    DIM_PRECISION           m_precision;       ///< Number of digits to display after decimal
    bool                    m_suppressZeroes;  ///< Suppress trailing zeroes

    // Geometry
    int                     m_lineThickness;    ///< Thickness used for all graphics in the dimension
    int                     m_arrowLength;      ///< Length of arrow shapes
    int                     m_extensionOffset;  ///< Distance from feature points to extension line start
    DIM_TEXT_POSITION       m_textPosition;     ///< How to position the text
    bool                    m_keepTextAligned;  ///< Calculate text orientation to match dimension

    // Internal
    int                     m_measuredValue;    ///< value of PCB dimensions
    VECTOR2I                m_start;
    VECTOR2I                m_end;

    ///< Internal cache of drawn shapes
    std::vector<std::shared_ptr<SHAPE>> m_shapes;

    bool       m_inClearRenderCache;      ///< re-entrancy guard

    // a flag to protect against reentrance
    bool m_busy;
};


/**
 * For better understanding of the points that make a dimension:
 *
 * Note: historically KiCad called extension lines "feature lines", and also note that what we
 * call the "crossbar line" here is more commonly called the "dimension line"
 *
 *              Start (feature point 1)         End (feature point 2)
 *                |                              |
 *                |   <-- extension lines -->    |
 *                |                              |
 *                |  m_arrowG2F      m_arrowD2F  |
 *                | /                          \ |
 * Crossbar start |/_______crossbar line________\| Crossbar end
 *                |\           m_text           /|
 *                | \                          / |
 *                |  m_arrowG1F      m_arrowD1F  |
 *                |                              |
 *                m_featureLineGF  m_featureLineDF
 */

/**
 * An aligned dimension measures the distance between two feature points.  It has a crossbar
 * (dimension line) that stays parallel with the vector between the feature points.
 *
 * The height (distance from features to crossbar) can be set directly, or set by manipulating the
 * crossbar start or end point (with the point editor).
 */
class PCB_DIM_ALIGNED : public PCB_DIMENSION_BASE
{
public:
    PCB_DIM_ALIGNED( BOARD_ITEM* aParent, KICAD_T aType = PCB_DIM_ALIGNED_T );

    // Do not create a copy constructor & operator=.
    // The ones generated by the compiler are adequate.

    ~PCB_DIM_ALIGNED() = default;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_DIM_ALIGNED_T;
    }

    EDA_ITEM* Clone() const override;

    void Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection ) override;

    BITMAPS GetMenuImage() const override;

    const VECTOR2I& GetCrossbarStart() const { return m_crossBarStart; }

    const VECTOR2I& GetCrossbarEnd() const { return m_crossBarEnd; }

    /**
     * Set the distance from the feature points to the crossbar line.
     *
     * @param aHeight is the new height.
     */
    void SetHeight( int aHeight ) { m_height = aHeight; }
    int GetHeight() const {  return m_height; }

    void ChangeHeight( int aHeight )
    {
        SetHeight( aHeight );
        Update();
    }

    /**
     * Update the stored height basing on points coordinates.
     *
     * @param aCrossbarStart is the start point of the crossbar.
     */
    void UpdateHeight( const VECTOR2I& aCrossbarStart, const VECTOR2I& aCrossbarEnd );

    void SetExtensionHeight( int aHeight ) { m_extensionHeight = aHeight; }
    int GetExtensionHeight() const { return m_extensionHeight; }

    void ChangeExtensionHeight( int aHeight )
    {
        SetExtensionHeight( aHeight );
        Update();
    }

    /**
     * Return the angle of the crossbar.
     *
     * @return Angle of the crossbar line expressed in radians.
     */
    double GetAngle() const
    {
        VECTOR2I delta( m_end - m_start );

        return atan2( (double)delta.y, (double)delta.x );
    }

    wxString GetClass() const override
    {
        return wxT( "PCB_DIM_ALIGNED" );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

    void updateGeometry() override;

    void updateText() override;

    // Geometry
    int          m_height;           ///< Perpendicular distance from features to crossbar
    int          m_extensionHeight;  ///< Length of extension lines past the crossbar

    VECTOR2I m_crossBarStart; ///< Crossbar start control point
    VECTOR2I m_crossBarEnd;   ///< Crossbar end control point

};


/**
 * An orthogonal dimension is like an aligned dimension, but the extension lines are locked to the
 * X or Y axes, and the measurement is only taken in the X or Y direction.
 */
class PCB_DIM_ORTHOGONAL : public PCB_DIM_ALIGNED
{
public:
    enum class DIR
    {
        HORIZONTAL, // Aligned with x-axis
        VERTICAL    // Aligned with y-axis
    };

    PCB_DIM_ORTHOGONAL( BOARD_ITEM* aParent );

    ~PCB_DIM_ORTHOGONAL() = default;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_DIM_ORTHOGONAL_T;
    }

    EDA_ITEM* Clone() const override;

    void Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection ) override;

    BITMAPS GetMenuImage() const override;

    /**
     * Set the orientation of the dimension line (so, perpendicular to the feature lines).
     *
     * @param aOrientation is the orientation the dimension should take.
     */
    void SetOrientation( DIR aOrientation ) { m_orientation = aOrientation; }
    DIR GetOrientation() const { return m_orientation; }

    wxString GetClass() const override
    {
        return wxT( "PCB_DIM_ORTHOGONAL" );
    }
    void     Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

protected:
    void swapData( BOARD_ITEM* aImage ) override;

    void updateGeometry() override;

    void updateText() override;

private:
    // Geometry
    DIR      m_orientation;     ///< What axis to lock the dimension line to.

};


/**
 * A radial dimension indicates either the radius or diameter of an arc or circle.
 *
 * A guide to the geometry of a circle dimension:
 *
 *     |
 *   --a--
 *     |
 *
 *
 *          b_
 *          |\
 *            \
 *             c---d TEXT
 *
 * Point a (the center of the arc or circle) is m_start, point b (a point on the arc or circle)
 * is m_end, point c is m_leaderLength away from b on the a-b vector, and point d is the end of
 * the "text line". The c-d line is drawn from c to the text center, and clipped on the text
 * bounding box.
 */
class PCB_DIM_RADIAL : public PCB_DIMENSION_BASE
{
public:
    PCB_DIM_RADIAL( BOARD_ITEM* aParent );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_DIM_RADIAL_T;
    }

    EDA_ITEM* Clone() const override;

    void SetLeaderLength( int aLength ) { m_leaderLength = aLength; }
    int GetLeaderLength() const { return m_leaderLength; }

    void ChangeLeaderLength( int aLength )
    {
        SetLeaderLength( aLength );
        Update();
    }

    // Returns the point (c).
    VECTOR2I GetKnee() const;

    BITMAPS GetMenuImage() const override;

    wxString GetClass() const override
    {
        return wxT( "PCB_DIM_RADIAL" );
    }

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

    void updateText() override;
    void updateGeometry() override;

private:
    int  m_leaderLength;
};


/**
 * A leader is a dimension-like object pointing to a specific point.
 *
 * A guide to the geometry of a leader:
 *
 *     a_
 *     |\
 *       \
 *        b---c TEXT
 *
 * Point (a) is m_start, point (b) is m_end, point (c) is the end of the "text line"
 * The b-c line is drawn from b to the text center, and clipped on the text bounding box.
 */
class PCB_DIM_LEADER : public PCB_DIMENSION_BASE
{
public:
    PCB_DIM_LEADER( BOARD_ITEM* aParent );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_DIM_LEADER_T;
    }

    EDA_ITEM* Clone() const override;

    BITMAPS GetMenuImage() const override;

    wxString GetClass() const override
    {
        return wxT( "PCB_DIM_LEADER" );
    }

    void SetTextBorder( DIM_TEXT_BORDER aBorder ) { m_textBorder = aBorder; }
    DIM_TEXT_BORDER GetTextBorder() const { return m_textBorder; }

    void ChangeTextBorder( DIM_TEXT_BORDER aBorder )
    {
        SetTextBorder( aBorder );
        Update();
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

    void updateText() override;
    void updateGeometry() override;

private:
    DIM_TEXT_BORDER m_textBorder;
};


/**
 * Mark the center of a circle or arc with a cross shape.
 *
 * The size and orientation of the cross is adjustable.
 * m_start always marks the center being measured; m_end marks the end of one leg of the cross.
 */
class PCB_DIM_CENTER : public PCB_DIMENSION_BASE
{
public:
    PCB_DIM_CENTER( BOARD_ITEM* aParent );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_DIM_CENTER_T;
    }

    EDA_ITEM* Clone() const override;

    BITMAPS GetMenuImage() const override;

    wxString GetClass() const override
    {
        return wxT( "PCB_DIM_CENTER" );
    }

    const BOX2I GetBoundingBox() const override;

    const BOX2I ViewBBox() const override;

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

    void updateText() override;
    void updateGeometry() override;
};

#endif    // DIMENSION_H
