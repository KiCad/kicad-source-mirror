/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#ifndef DIMENSION_H
#define DIMENSION_H


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
    INCHES,
    MILS,
    MILLIMETRES,
    AUTOMATIC
};

/**
 * Frame to show around dimension text
 */
enum class DIM_TEXT_FRAME
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
class DIMENSION_BASE : public BOARD_ITEM
{
public:
    DIMENSION_BASE( BOARD_ITEM* aParent, KICAD_T aType = PCB_DIMENSION_T );

    bool IsType( const KICAD_T aScanTypes[] ) const override
    {
        if( BOARD_ITEM::IsType( aScanTypes ) )
            return true;

        for( const KICAD_T* p = aScanTypes; *p != EOT; ++p )
        {
            if( *p == PCB_LOCATE_GRAPHIC_T )
                return true;
        }

        return false;
    }

    void SetParent( EDA_ITEM* aParent ) override;

    /**
     * The dimension's origin is the first feature point for the dimension.  Every dimension has
     * one or more feature points, so every dimension has at least an origin.
     * @return the origin point of this dimension
     */
    virtual const wxPoint& GetStart() const { return m_start; }
    virtual void SetStart( const wxPoint& aPoint ) { m_start = aPoint; }

    virtual const wxPoint& GetEnd() const { return m_end; }
    virtual void SetEnd( const wxPoint& aPoint ) { m_end = aPoint; }

    wxPoint GetPosition() const override { return m_start; }
    void SetPosition( const wxPoint& aPos ) override { m_start = aPos; }

    bool GetOverrideTextEnabled() const { return m_overrideTextEnabled; }
    void SetOverrideTextEnabled( bool aOverride ) { m_overrideTextEnabled = aOverride; }

    wxString GetOverrideText() const { return m_valueString; }
    void SetOverrideText( const wxString& aValue ) { m_valueString = aValue; }

    int GetMeasuredValue() const { return m_measuredValue; }

    /**
     * @return the dimension value, rendered with precision / zero suppression but no units, etc
     */
    wxString GetValueText() const;

    /**
     * Update the dimension's cached text and geometry.
     */
    void Update()
    {
        updateGeometry();
        updateText();
    }

    wxString GetPrefix() const { return m_prefix; }
    void SetPrefix( const wxString& aPrefix );

    wxString GetSuffix() const { return m_suffix; }
    void SetSuffix( const wxString& aSuffix );

    void GetUnits( EDA_UNITS& aUnits ) const { aUnits = m_units; }
    void SetUnits( EDA_UNITS aUnits );

    DIM_UNITS_MODE GetUnitsMode() const;
    void SetUnitsMode( DIM_UNITS_MODE aMode );

    void SetAutoUnits( bool aAuto = true ) { m_autoUnits = aAuto; }

    DIM_UNITS_FORMAT GetUnitsFormat() const { return m_unitsFormat; }
    void SetUnitsFormat( const DIM_UNITS_FORMAT aFormat ) { m_unitsFormat = aFormat; }

    int GetPrecision() const { return m_precision; }
    void SetPrecision( int aPrecision ) { m_precision = aPrecision; }

    bool GetSuppressZeroes() const { return m_suppressZeroes; }
    void SetSuppressZeroes( bool aSuppress ) { m_suppressZeroes = aSuppress; }

    bool GetKeepTextAligned() const { return m_keepTextAligned; }
    void SetKeepTextAligned( bool aKeepAligned ) { m_keepTextAligned = aKeepAligned; }

    void SetTextPositionMode( DIM_TEXT_POSITION aMode ) { m_textPosition = aMode; }
    DIM_TEXT_POSITION GetTextPositionMode() const { return m_textPosition; }

    int GetArrowLength() const { return m_arrowLength; }
    void SetArrowLength( int aLength ) { m_arrowLength = aLength; }

    void SetExtensionOffset( int aOffset ) { m_extensionOffset = aOffset; }
    int GetExtensionOffset() const { return m_extensionOffset; }

    int GetLineThickness() const        { return m_lineThickness; }
    void SetLineThickness( int aWidth ) { m_lineThickness = aWidth; }

    void SetLayer( PCB_LAYER_ID aLayer ) override;

    void SetTextSize( const wxSize& aTextSize )
    {
        m_text.SetTextSize( aTextSize );
    }

    /**
     * Set the override text - has no effect if m_overrideValue == false.
     *
     * @param aNewText is the text to use as the value.
     */
    void           SetText( const wxString& aNewText );

    /**
     * Retrieve the value text or override text, not including prefix or suffix.
     *
     * @return the value portion of the dimension text (either overridden or not).
     */
    const wxString GetText() const;

    PCB_TEXT& Text() { return m_text; }
    const PCB_TEXT& Text() const { return m_text; }

    /**
     * @return a list of line segments that make up this dimension (for drawing, plotting, etc).
     */
    const std::vector<std::shared_ptr<SHAPE>>& GetShapes() const { return m_shapes; }

    // BOARD_ITEM overrides

    void Move( const wxPoint& offset ) override;
    void Rotate( const wxPoint& aRotCentre, double aAngle ) override;
    void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    /**
     * Mirror the dimension relative to a given horizontal axis.
     *
     * The text is not mirrored.  Only its position (and angle) is mirrored.  The layer is not
     * changed.
     *
     * @param axis_pos is the vertical axis position to mirror around.
     */
    void Mirror( const wxPoint& axis_pos, bool aMirrorLeftRight = false );

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    const EDA_RECT GetBoundingBox() const override;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    const BOX2I ViewBBox() const override;

    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, PCB_LAYER_ID aLayer,
                                               int aClearance, int aError, ERROR_LOC aErrorLoc,
                                               bool aIgnoreLineWidth ) const override;

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
     */
    virtual void updateText();

    template<typename ShapeType>
    void addShape( const ShapeType& aShape );

    /**
     * Find the intersection between a given segment and polygon outline.
     *
     * @param aPoly is the polygon to collide.
     * @param aSeg is the segment to collide.
     * @param aStart if true will start from aSeg.A, otherwise aSeg.B.
     * @return a point on aSeg that collides with aPoly closest to the start, if one exists.
     */
    static OPT_VECTOR2I segPolyIntersection( const SHAPE_POLY_SET& aPoly, const SEG& aSeg,
                                             bool aStart = true );
    static OPT_VECTOR2I segCircleIntersection( CIRCLE& aCircle, SEG& aSeg, bool aStart = true );

    // Value format
    bool              m_overrideTextEnabled;   ///< Manually specify the displayed measurement value
    wxString          m_valueString;     ///< Displayed value when m_overrideValue = true
    wxString          m_prefix;          ///< String prepended to the value
    wxString          m_suffix;          ///< String appended to the value
    EDA_UNITS         m_units;           ///< 0 = inches, 1 = mm
    bool              m_autoUnits;       ///< If true, follow the currently selected UI units
    DIM_UNITS_FORMAT  m_unitsFormat;     ///< How to render the units suffix
    int               m_precision;       ///< Number of digits to display after decimal
    bool              m_suppressZeroes;  ///< Suppress trailing zeroes

    // Geometry
    int               m_lineThickness;    ///< Thickness used for all graphics in the dimension
    int               m_arrowLength;      ///< Length of arrow shapes
    int               m_extensionOffset;  ///< Distance from feature points to extension line start
    DIM_TEXT_POSITION m_textPosition;     ///< How to position the text
    bool              m_keepTextAligned;  ///< Calculate text orientation to match dimension

    // Internal
    PCB_TEXT          m_text;             ///< The actual text object
    int               m_measuredValue;    ///< value of PCB dimensions
    wxPoint           m_start;
    wxPoint           m_end;

    ///< Internal cache of drawn shapes
    std::vector<std::shared_ptr<SHAPE>> m_shapes;

    static constexpr float s_arrowAngle = 27.5;
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
class ALIGNED_DIMENSION : public DIMENSION_BASE
{
public:
    ALIGNED_DIMENSION( BOARD_ITEM* aParent, KICAD_T aType = PCB_DIM_ALIGNED_T );

    // Do not create a copy constructor & operator=.
    // The ones generated by the compiler are adequate.

    ~ALIGNED_DIMENSION() = default;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DIM_ALIGNED_T == aItem->Type();
    }

    EDA_ITEM* Clone() const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

    BITMAPS GetMenuImage() const override;

    const wxPoint& GetCrossbarStart() const { return m_crossBarStart; }

    const wxPoint& GetCrossbarEnd() const { return m_crossBarEnd; }

    /**
     * Set the distance from the feature points to the crossbar line.
     *
     * @param aHeight is the new height.
     */
    void SetHeight( int aHeight ) { m_height = aHeight; }
    int GetHeight() const {  return m_height; }

    /**
     * Update the stored height basing on points coordinates.
     *
     * @param aCrossbarStart is the start point of the crossbar.
     */
    void UpdateHeight( const wxPoint& aCrossbarStart, const wxPoint& aCrossbarEnd );

    void SetExtensionHeight( int aHeight ) { m_extensionHeight = aHeight; }
    int GetExtensionHeight() const { return m_extensionHeight; }

    /**
     * Return the angle of the crossbar.
     *
     * @return Angle of the crossbar line expressed in radians.
     */
    double GetAngle() const
    {
        wxPoint delta( m_end - m_start );

        return atan2( (double)delta.y, (double)delta.x );
    }

    wxString GetClass() const override
    {
        return wxT( "ALIGNED_DIMENSION" );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    void updateGeometry() override;

    void updateText() override;

    // Geometry
    int          m_height;           ///< Perpendicular distance from features to crossbar
    int          m_extensionHeight;  ///< Length of extension lines past the crossbar

    wxPoint      m_crossBarStart;    ///< Crossbar start control point
    wxPoint      m_crossBarEnd;      ///< Crossbar end control point

};


/**
 * An orthogonal dimension is like an aligned dimension, but the extension lines are locked to the
 * X or Y axes, and the measurement is only taken in the X or Y direction.
 */
class ORTHOGONAL_DIMENSION : public ALIGNED_DIMENSION
{
public:
    enum class DIR
    {
        HORIZONTAL, // Aligned with x-axis
        VERTICAL    // Aligned with y-axis
    };

    ORTHOGONAL_DIMENSION( BOARD_ITEM* aParent );

    ~ORTHOGONAL_DIMENSION() = default;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DIM_ORTHOGONAL_T == aItem->Type();
    }

    EDA_ITEM* Clone() const override;

    void SwapData( BOARD_ITEM* aImage ) override;

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
        return wxT( "ORTHOGONAL_DIMENSION" );
    }
    void     Rotate( const wxPoint& aRotCentre, double aAngle ) override;

protected:
    void updateGeometry() override;

    void updateText() override;

private:
    // Geometry
    DIR      m_orientation;     ///< What axis to lock the dimension line to.

};


/**
 * A leader is a dimension-like object pointing to a specific point.
 *
 * A guide to the geometry of a leader:
 *
 *     a
 *        _
 *       |\
 *          \
 *            b---c TEXT
 *
 * Point (a) is m_start, point (b) is m_end, point (c) is the end of the "text line"
 * The b-c line is drawn from b to the text center, and clipped on the text bounding box.
 */
class LEADER : public DIMENSION_BASE
{
public:
    LEADER( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DIM_LEADER_T == aItem->Type();
    }

    EDA_ITEM* Clone() const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

    BITMAPS GetMenuImage() const override;

    wxString GetClass() const override
    {
        return wxT( "LEADER" );
    }

    void SetTextFrame( DIM_TEXT_FRAME aFrame ) { m_textFrame = aFrame; }
    DIM_TEXT_FRAME GetTextFrame() const { return m_textFrame; }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    void updateGeometry() override;

private:
    DIM_TEXT_FRAME m_textFrame;
};


/**
 * Mark the center of a circle or arc with a cross shape.
 *
 * The size and orientation of the cross is adjustable.
 * m_start always marks the center being measured; m_end marks the end of one leg of the cross.
 */
class CENTER_DIMENSION : public DIMENSION_BASE
{
public:
    CENTER_DIMENSION( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DIM_CENTER_T == aItem->Type();
    }

    EDA_ITEM* Clone() const override;

    virtual void SwapData( BOARD_ITEM* aImage ) override;

    BITMAPS GetMenuImage() const override;

    wxString GetClass() const override
    {
        return wxT( "CENTER_DIMENSION" );
    }

    const EDA_RECT GetBoundingBox() const override;

    const BOX2I ViewBBox() const override;

protected:
    void updateGeometry() override;
};

#endif    // DIMENSION_H
