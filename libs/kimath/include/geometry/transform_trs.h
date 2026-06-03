/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TRANSFORM_TRS_H
#define TRANSFORM_TRS_H

#include <math/vector2d.h>
#include <geometry/eda_angle.h>


// Translate / rotate / scale transform for footprint placement.
// Apply order is fixed: scale, then rotate, then translate.
// Independent X/Y scale is allowed. Shear is not representable.
class TRANSFORM_TRS
{
public:
    TRANSFORM_TRS() :
            m_translate( 0, 0 ),
            m_rotate( ANGLE_0 ),
            m_scaleX( 1.0 ),
            m_scaleY( 1.0 )
    {}

    TRANSFORM_TRS( const VECTOR2I& aTranslate, const EDA_ANGLE& aRotate,
                   double aScaleX, double aScaleY ) :
            m_translate( aTranslate ),
            m_rotate( aRotate ),
            m_scaleX( aScaleX ),
            m_scaleY( aScaleY )
    {}

    VECTOR2I Apply( const VECTOR2I& aPoint ) const;
    VECTOR2D Apply( const VECTOR2D& aPoint ) const;

    VECTOR2I InverseApply( const VECTOR2I& aPoint ) const;
    VECTOR2D InverseApply( const VECTOR2D& aPoint ) const;

    TRANSFORM_TRS Invert() const;

    TRANSFORM_TRS Compose( const TRANSFORM_TRS& aOuter ) const;

    TRANSFORM_TRS RescaleAround( const VECTOR2I& aFixedPoint, double aSx, double aSy ) const;

    bool IsIdentity() const;
    bool IsUniformScale() const;

    double ApplyLinearScale( double aLength ) const;

    const VECTOR2I&  GetTranslate() const { return m_translate; }
    const EDA_ANGLE& GetRotate() const { return m_rotate; }
    double           GetScaleX() const { return m_scaleX; }
    double           GetScaleY() const { return m_scaleY; }

    void SetTranslate( const VECTOR2I& aT ) { m_translate = aT; }
    void SetRotate( const EDA_ANGLE& aR ) { m_rotate = aR; }
    void SetScale( double aSx, double aSy ) { m_scaleX = aSx; m_scaleY = aSy; }

    bool operator==( const TRANSFORM_TRS& aOther ) const;
    bool operator!=( const TRANSFORM_TRS& aOther ) const { return !( *this == aOther ); }

private:
    VECTOR2I  m_translate;
    EDA_ANGLE m_rotate;
    double    m_scaleX;
    double    m_scaleY;
};

#endif // TRANSFORM_TRS_H
