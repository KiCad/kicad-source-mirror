/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_MEANDER_H
#define __PNS_MEANDER_H

#include <math/vector2d.h>
#include <core/minoptmax.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

class NETCLASS;

namespace PNS {

class MEANDER_PLACER_BASE;
class MEANDERED_LINE;

///< Shapes of available meanders.
enum MEANDER_TYPE {
        MT_SINGLE,          // _|^|_, single-sided
        MT_START,           // _|^|
        MT_FINISH,          // |^|_
        MT_TURN,            // |^| or |_|
        MT_CHECK_START,     // try fitting a start type, but don't produce a line
        MT_CHECK_FINISH,    // try fitting a finish type, but don't produce a line
        MT_CORNER,          // line corner
        MT_ARC,             // arc corner
        MT_EMPTY            // no meander (straight line)
};

///< Meander corner shape.
enum MEANDER_STYLE {
    MEANDER_STYLE_ROUND = 1,          // rounded (90 degree arc)
    MEANDER_STYLE_CHAMFER             // chamfered (45 degree segment)
};

///< Initial side the meander is placed on.
enum MEANDER_SIDE
{
    MEANDER_SIDE_LEFT = -1,
    MEANDER_SIDE_DEFAULT = 0,
    MEANDER_SIDE_RIGHT = 1
};

/**
 * Dimensions for the meandering algorithm.
 */
class MEANDER_SETTINGS
{
public:
    static const long long int DEFAULT_LENGTH_TOLERANCE;
    static const long long int LENGTH_UNCONSTRAINED;

    static const long long int DEFAULT_DELAY_TOLERANCE;
    static const long long int DELAY_UNCONSTRAINED;

    static const int SKEW_UNCONSTRAINED;

    MEANDER_SETTINGS();

    void SetTargetLength( long long int aOpt );
    void SetTargetLength( const MINOPTMAX<int>& aConstraint );

    void SetTargetLengthDelay( long long int aOpt );
    void SetTargetLengthDelay( const MINOPTMAX<int>& aConstraint );

    void SetTargetSkew( int aOpt );
    void SetTargetSkew( const MINOPTMAX<int>& aConstraint );

    void SetTargetSkewDelay( int aOpt );
    void SetTargetSkewDelay( const MINOPTMAX<int>& aConstraint );

    ///< Minimum meandering amplitude.
    int m_minAmplitude;

    ///< Maximum meandering amplitude.
    int m_maxAmplitude;

    ///< Meandering period/spacing (see dialog picture for explanation).
    int m_spacing;

    ///< Amplitude/spacing adjustment step.
    int m_step;

    ///< Length PadToDie.
    int m_lenPadToDie;

    ///< Desired length of the tuned line/diff pair (this is in nm, so allow more than board width).
    MINOPTMAX<long long int> m_targetLength;

    ///< Desired propagation delay of the tuned line
    MINOPTMAX<long long int> m_targetLengthDelay;

    ///< Target skew value for diff pair de-skewing.
    MINOPTMAX<int>           m_targetSkew;

    ///< Target skew value for diff pair de-skewing.
    MINOPTMAX<int> m_targetSkewDelay;

    bool                     m_overrideCustomRules;

    ///< Type of corners for the meandered line.
    MEANDER_STYLE m_cornerStyle;

    ///< Rounding percentage (0 - 100).
    int m_cornerRadiusPercentage;

    ///< Place meanders on one side.
    bool m_singleSided;

    ///< Initial side when placing meanders at segment
    MEANDER_SIDE m_initialSide;

    ///< Allowable tuning error.
    int m_lengthTolerance;

    ///< Keep vertices between pre, tuned and post parts of the line.
    bool m_keepEndpoints;

    ///< Calculate tuning in the time domain
    bool m_isTimeDomain;

    ///< The net class this meander pattern belongs to
    NETCLASS* m_netClass;
};

/**
 * The geometry of a single meander.
 */
class MEANDER_SHAPE
{
public:
    /**
     * @param aPlacer the meander placer instance.
     * @param aWidth width of the meandered line.
     * @param aIsDual when true, the shape contains two meandered
     *                lines at a given offset (diff pairs).
     */
    MEANDER_SHAPE( MEANDER_PLACER_BASE* aPlacer, int aWidth, bool aIsDual = false ) :
        m_placer( aPlacer ),
        m_dual( aIsDual ),
        m_width( aWidth ),
        m_baselineOffset( 0 )
    {
        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_type = MT_SINGLE;
        m_amplitude = 0;
        m_targetBaseLen = 0;
        m_side = false;
        m_baseIndex = 0;
        m_currentTarget = nullptr;
        m_meanCornerRadius = 0;
    }

    /**
     * Set the type of the meander.
     */
    void SetType( MEANDER_TYPE aType )
    {
        m_type = aType;
    }

    /**
     * @return the type of the meander.
     */
    MEANDER_TYPE Type() const
    {
        return m_type;
    }

    /**
     * Set an auxiliary index of the segment being meandered in its original LINE.
     */
    void SetBaseIndex( int aIndex )
    {
        m_baseIndex = aIndex;
    }

    /**
     * @return auxiliary index of the segment being meandered in its original LINE.
     */
    int BaseIndex() const
    {
        return m_baseIndex;
    }

    /**
     * @return the amplitude of the meander shape.
     */
    int Amplitude() const
    {
        return m_amplitude;
    }

    /**
     * Create a dummy meander shape representing a line corner. Used to define
     * the starts/ends of meandered segments.
     *
     * @param aP1 corner point of the 1st line.
     * @param aP2 corner point of the 2nd line (if m_dual == true).
     */
    void MakeCorner( const VECTOR2I& aP1, const VECTOR2I& aP2 = VECTOR2I( 0, 0 ) );

    /**
     * Create a dummy meander shape representing an arc corner. Allows representing existing
     * arc tracks so they can be reconstructed after length tuning.
     *
     * @param aArc1 Arc shape on the 1st line.
     * @param aArc2 Arc shape on the 2nd line (if m_dual == true).
     */
    void MakeArc( const SHAPE_ARC& aArc1, const SHAPE_ARC& aArc2 = SHAPE_ARC() );

    /**
     * Change the amplitude of the meander shape to aAmpl and recalculates the resulting
     * line chain.
     *
     * @param aAmpl new amplitude.
     */
    void Resize( int aAmpl );

    /**
     * Recalculate the line chain representing the meander's shape.
     */
    void Recalculate();

    /**
     * @return true if the shape represents 2 parallel lines (diff pair).
     */
    bool IsDual() const
    {
        return m_dual;
    }

    /**
     * @return true if the meander is to the right of its base segment.
     */
    bool Side() const
    {
        return m_side;
    }

    /**
     * @return end vertex of the base segment of the meander shape.
     */
    VECTOR2I End() const
    {
        return m_clippedBaseSeg.B;
    }

    /**
     * @return the line chain representing the shape of the meander.
     */
    const SHAPE_LINE_CHAIN& CLine( int aShape ) const
    {
        return m_shapes[aShape];
    }

    /**
     * Replace the meander with straight bypass line(s), effectively clearing it.
     */
    void MakeEmpty();

    /**
     * Attempt to fit a meander of a given type onto a segment, avoiding collisions with other
     * board features.
     *
     * @param aType type of meander shape.
     * @param aSeg base segment for meandering.
     * @param aP start point of the meander.
     * @param aSide side of aSeg to put the meander on (true = right).
     * @return true on success.
     */
    bool Fit( MEANDER_TYPE aType, const SEG& aSeg, const VECTOR2I& aP, bool aSide );

    /**
     * Return the base segment the meander was fitted to.
     *
     * @return the base segment.
     */
    const SEG& BaseSegment() const
    {
        return m_clippedBaseSeg;
    }

    /**
     * @return length of the base segment for the meander (i.e.the minimum tuned length).
     */
    int BaselineLength() const;

    /**
     * @return the length of the fitted line chain.
     */
    long long int CurrentLength() const;

    /**
     * @return the minumum tunable length according to settings.
     */
    long long int MinTunableLength() const;

    /**
     * @return the minumum possible amplitude according to settings.
     */
    int MinAmplitude() const;

    /**
     * @return the current meandering settings.
     */
    const MEANDER_SETTINGS& Settings() const;

    /**
     * @return width of the meandered line.
     */
    int Width() const
    {
        return m_width;
    }

    /**
     * Set the parallel offset between the base segment and the meandered line. Used for
     * dual meanders (diff pair) only.
     *
     * @param aOffset the offset.
     */
    void SetBaselineOffset( int aOffset )
    {
        m_baselineOffset = aOffset;
    }

    /**
     * Sets the target length of the baseline. When resizing, the meander will try to
     * fit the baseline length into the specified value.
     *
     * @param aLength the minimum baseline length.
     */
    void SetTargetBaselineLength( int aLength ) { m_targetBaseLen = aLength; }

private:
    friend class MEANDERED_LINE;

    ///< Start turtle drawing
    void start( SHAPE_LINE_CHAIN* aTarget, const VECTOR2D& aWhere, const VECTOR2D& aDir );

    ///< Move turtle forward by \a aLength.
    void forward( int aLength );

    ///< Turn the turtle by \a aAngle
    void turn( const EDA_ANGLE& aAngle );

    ///< Tell the turtle to draw a mitered corner of given radius and turn direction.
    void miter( int aRadius, bool aSide );

    ///< Tell the turtle to draw an U-like shape.
    void uShape( int aSides, int aCorner, int aTop );

    ///< Generate a 90-degree circular arc.
    SHAPE_LINE_CHAIN makeMiterShape( const VECTOR2D& aP, const VECTOR2D& aDir, bool aSide );

    ///< Produce a meander shape of given type.
    SHAPE_LINE_CHAIN genMeanderShape( const VECTOR2D& aP, const VECTOR2D& aDir, bool aSide,
                                      MEANDER_TYPE aType, int aBaselineOffset = 0 );

    ///< Recalculate the clipped baseline after the parameters of the meander have been changed.
    void updateBaseSegment();

    ///< Return sanitized corner radius value.
    int cornerRadius() const;

    ///< Return sanitized spacing value.
    int spacing() const;

    ///< The type of meander.
    MEANDER_TYPE m_type;

    ///< The placer that placed this meander.
    MEANDER_PLACER_BASE* m_placer;

    ///< Dual or single line.
    bool m_dual;

    ///< Width of the line.
    int m_width;

    ///< Amplitude of the meander.
    int m_amplitude;

    ///< Offset wrs the base segment (dual only).
    int m_baselineOffset;

    ///< Average radius of meander corners (for correction of DP meanders).
    int m_meanCornerRadius;

    ///< Minimum length of the base segment to target when resizing.
    int m_targetBaseLen;

    ///< First point of the meandered line.
    VECTOR2I m_p0;

    ///< Base segment (unclipped).
    SEG m_baseSeg;

    ///< Base segment (clipped).
    SEG m_clippedBaseSeg;

    ///< Side (true = right).
    bool m_side;

    ///< The actual shapes (0 used for single, both for dual).
    SHAPE_LINE_CHAIN m_shapes[2];

    ///< Index of the meandered segment in the base line.
    int m_baseIndex;

    ///< The current turtle direction.
    VECTOR2D m_currentDir;

    ///< The current turtle position.
    VECTOR2D m_currentPos;

    ///< The line the turtle is drawing on.
    SHAPE_LINE_CHAIN* m_currentTarget;
};


/**
 * Represent a set of meanders fitted over a single or two lines.
 */
class MEANDERED_LINE
{
public:
    MEANDERED_LINE()
    {
        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_placer = nullptr;
        m_dual = false;
        m_width = 0;
        m_baselineOffset = 0;
    }

    /**
     * @param aPlacer the meander placer instance
     * @param aIsDual when true, the meanders are generated for two coupled lines
     */
    MEANDERED_LINE( MEANDER_PLACER_BASE* aPlacer, bool aIsDual = false ) :
        m_placer( aPlacer ),
        m_dual( aIsDual )
    {
        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_width = 0;
        m_baselineOffset = 0;
    }

    ~MEANDERED_LINE()
    {
        Clear();
    }

    /**
     * Create a dummy meander shape representing a line corner.  Used to define the starts/ends
     * of meandered segments.
     *
     * @param aA corner point of the 1st line.
     * @param aB corner point of the 2nd line (if m_dual == true).
     */
    void AddCorner( const VECTOR2I& aA, const VECTOR2I& aB = VECTOR2I( 0, 0 ) );

    /**
     * Create a dummy meander shape representing an arc corner.  Allows representing existing
     * arc tracks so they can be reconstructed after length tuning.
     *
     * @param aArc1 Arc shape on the 1st line.
     * @param aArc2 Arc shape on the 2nd line (if m_dual == true).
     */
    void AddArc( const SHAPE_ARC& aArc1, const SHAPE_ARC& aArc2 = SHAPE_ARC() );

    /**
     * Create a dummy meander shape representing an arc corner.  Allows representing existing
     * arc tracks so they can be reconstructed after length tuning.
     *
     * @param aArc1 Arc shape on the 1st line.
     * @param aPt2 corner point of the 2nd line (if m_dual == true).
     */
    void AddArcAndPt( const SHAPE_ARC& aArc1, const VECTOR2I& aPt2 );

    /**
     * Create a dummy meander shape representing an arc corner.  Allows representing existing
     * arc tracks so they can be reconstructed after length tuning.
     *
     * @param aPt1 corner point of the 1st line.
     * @param aArc2 Arc shape on the 2nd line (if m_dual == true).
     */
    void AddPtAndArc( const VECTOR2I& aPt1, const SHAPE_ARC& aArc2 );

    /**
     * Add a new meander shape to the meandered line.
     *
     * @param aShape the meander shape to add
     */
    void AddMeander( MEANDER_SHAPE* aShape );

    /**
     * Clear the line geometry, removing all corners and meanders.
     */
    void Clear();

    /**
     * Set the line width.
     */
    void SetWidth( int aWidth )
    {
        m_width = aWidth;
    }

    /**
     * Fit maximum amplitude meanders on a given segment and adds to the current line.
     *
     * @param aSeg the base segment to meander.
     * @param aSide Side to start meandering the segment. True=left, False=Right
     * @param aBaseIndex index of the base segment in the original line.
     */
    void MeanderSegment( const SEG& aSeg, bool aSide, int aBaseIndex = 0 );

    /// @copydoc MEANDER_SHAPE::SetBaselineOffset()
    void SetBaselineOffset( int aOffset )
    {
        m_baselineOffset = aOffset;
    }

    /**
     * @return set of meander shapes for this line.
     */
    std::vector<MEANDER_SHAPE*>& Meanders()
    {
        return m_meanders;
    }

    /**
     * Check if the given shape is intersecting with any other meander in the current line.
     *
     * @param aShape the shape to check.
     * @param aClearance clearance value.
     * @return true, if the meander shape is not colliding.
     */
    bool CheckSelfIntersections( MEANDER_SHAPE* aShape, int aClearance );

    /**
     * @return the current meandering settings.
     */
    const MEANDER_SETTINGS& Settings() const;

    // Move assignment operator
    MEANDERED_LINE& operator=( MEANDERED_LINE&& aOther ) noexcept
    {
        if (this != &aOther)
        {
            m_last = aOther.m_last;

            m_placer = aOther.m_placer;
            m_meanders = std::move( aOther.m_meanders );

            m_dual = aOther.m_dual;
            m_width = aOther.m_width;
            m_baselineOffset = aOther.m_baselineOffset;
        }

        return *this;
    }

private:
    VECTOR2I m_last;

    MEANDER_PLACER_BASE* m_placer;
    std::vector<MEANDER_SHAPE*> m_meanders;

    bool m_dual;
    int m_width;
    int m_baselineOffset;
};

}

#endif    // __PNS_MEANDER_H
