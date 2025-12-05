/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "reference_image.h"

#include <wx/debug.h>

#include <bitmap_base.h>
#include <geometry/geometry_utils.h>


REFERENCE_IMAGE::REFERENCE_IMAGE( const EDA_IU_SCALE& aIuScale ) :
        m_iuScale( aIuScale ), m_pos( 0, 0 ), m_transformOriginOffset( 0, 0 ),
        m_bitmapBase( std::make_unique<BITMAP_BASE>() )
{
    updatePixelSizeInIU();
}


REFERENCE_IMAGE::REFERENCE_IMAGE( const REFERENCE_IMAGE& aOther ) :
        m_iuScale( aOther.m_iuScale ), m_pos( aOther.m_pos ),
        m_transformOriginOffset( aOther.m_transformOriginOffset ),
        m_bitmapBase( std::make_unique<BITMAP_BASE>( *aOther.m_bitmapBase ) )
{
    updatePixelSizeInIU();
}


REFERENCE_IMAGE::~REFERENCE_IMAGE()
{
}


void REFERENCE_IMAGE::updatePixelSizeInIU()
{
    const double pixelSizeIu = (double) m_iuScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI();
    m_bitmapBase->SetPixelSizeIu( pixelSizeIu );
}


REFERENCE_IMAGE& REFERENCE_IMAGE::operator=( const REFERENCE_IMAGE& aOther )
{
    wxASSERT( m_iuScale.IU_PER_MILS == aOther.m_iuScale.IU_PER_MILS );

    if( &aOther != this )
    {
        if( aOther.m_bitmapBase )
        {
            m_bitmapBase = std::make_unique<BITMAP_BASE>( *aOther.m_bitmapBase );
        }
        m_pos = aOther.m_pos;
        m_transformOriginOffset = aOther.m_transformOriginOffset;
        updatePixelSizeInIU();
    }

    return *this;
}


bool REFERENCE_IMAGE::operator==( const REFERENCE_IMAGE& aOther ) const
{
    if( m_pos != aOther.m_pos )
        return false;

    if( m_transformOriginOffset != aOther.m_transformOriginOffset )
        return false;

    if( m_bitmapBase->GetSize() != aOther.m_bitmapBase->GetSize() )
        return false;

    if( m_bitmapBase->GetPPI() != aOther.m_bitmapBase->GetPPI() )
        return false;

    if( m_bitmapBase->GetScale() != aOther.m_bitmapBase->GetScale() )
        return false;

    if( m_bitmapBase->GetImageID() != aOther.m_bitmapBase->GetImageID() )
        return false;

    if( m_bitmapBase->GetImageData() != aOther.m_bitmapBase->GetImageData() )
        return false;

    return true;
}


double REFERENCE_IMAGE::Similarity( const REFERENCE_IMAGE& aOther ) const
{
    double similarity = 1.0;

    if( m_pos != aOther.m_pos )
        similarity *= 0.9;

    if( m_bitmapBase->GetSize() != aOther.m_bitmapBase->GetSize() )
        similarity *= 0.9;

    if( m_bitmapBase->GetPPI() != aOther.m_bitmapBase->GetPPI() )
        similarity *= 0.9;

    if( m_bitmapBase->GetScale() != aOther.m_bitmapBase->GetScale() )
        similarity *= 0.9;

    if( m_bitmapBase->GetImageID() != aOther.m_bitmapBase->GetImageID() )
        similarity *= 0.9;

    if( m_bitmapBase->GetImageData() != aOther.m_bitmapBase->GetImageData() )
        similarity *= 0.9;

    return similarity;
}


BOX2I REFERENCE_IMAGE::GetBoundingBox() const
{
    return BOX2I::ByCenter( m_pos, m_bitmapBase->GetSize() );
}


VECTOR2I REFERENCE_IMAGE::GetPosition() const
{
    return m_pos;
}


void REFERENCE_IMAGE::SetPosition( const VECTOR2I& aPos )
{
    const BOX2D newBox = BOX2D::ByCenter( aPos, m_bitmapBase->GetSize() );

    if( !IsBOX2Safe( newBox ) )
        return;

    m_pos = aPos;
}


VECTOR2I REFERENCE_IMAGE::GetTransformOriginOffset() const
{
    return m_transformOriginOffset;
}


void REFERENCE_IMAGE::SetTransformOriginOffset( const VECTOR2I& aCenter )
{
    m_transformOriginOffset = aCenter;
}


VECTOR2I REFERENCE_IMAGE::GetSize() const
{
    return m_bitmapBase->GetSize();
}


void REFERENCE_IMAGE::SetWidth( int aWidth )
{
    if( aWidth <= 0 )
        return;

    const double ratio = aWidth / (double) m_bitmapBase->GetSize().x;
    scaleBy( ratio );
}


void REFERENCE_IMAGE::SetHeight( int aHeight )
{
    if( aHeight <= 0 )
        return;

    const double ratio = aHeight / (double) m_bitmapBase->GetSize().y;
    scaleBy( ratio );
}


double REFERENCE_IMAGE::GetImageScale() const
{
    return m_bitmapBase->GetScale();
}


void REFERENCE_IMAGE::SetImageScale( double aScale )
{
    if( aScale <= 0 )
        return;

    const double ratio = aScale / m_bitmapBase->GetScale();
    scaleBy( ratio );
}


void REFERENCE_IMAGE::scaleBy( double aRatio )
{
    if( aRatio <= 0 )
        return;

    const VECTOR2D currentOrigin = m_pos + m_transformOriginOffset;
    const VECTOR2D newOffset = m_transformOriginOffset * aRatio;
    const VECTOR2D newCenter = currentOrigin - newOffset;
    const VECTOR2D newSize = m_bitmapBase->GetSize() * aRatio;

    // The span of the image is limited to the size of the coordinate system
    if( !IsVec2SafeXY( newSize ) )
        return;

    const BOX2D newBox = BOX2D::ByCenter( newCenter, newSize );

    // Any overflow, just reject the call
    if( !IsBOX2Safe( newBox ) )
        return;

    m_bitmapBase->SetScale( m_bitmapBase->GetScale() * aRatio );
    SetTransformOriginOffset( KiROUND( newOffset ) );

    // Don't need to recheck the box, we just did that
    m_pos = KiROUND( newCenter );
}


void REFERENCE_IMAGE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    VECTOR2I newPos = m_pos;
    MIRROR( newPos, aCentre, aFlipDirection );

    const BOX2D newBox = BOX2D::ByCenter( newPos, m_bitmapBase->GetSize() );

    if( !IsBOX2Safe( newBox ) )
        return;

    m_pos = newPos;
    m_bitmapBase->Mirror( aFlipDirection );
}


void REFERENCE_IMAGE::Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE norm( aAngle.AsDegrees(), DEGREES_T );

    RotatePoint( m_pos, aCenter, aAngle );

    norm.Normalize();

    // each call to m_bitmapBase->Rotate() rotates 90 degrees
    for( double ang = 45.0; ang < norm.AsDegrees(); ang += 90.0 )
        m_bitmapBase->Rotate( true );
}


bool REFERENCE_IMAGE::ReadImageFile( const wxString& aFullFilename )
{
    if( m_bitmapBase->ReadImageFile( aFullFilename ) )
    {
        updatePixelSizeInIU();
        return true;
    }

    return false;
}


bool REFERENCE_IMAGE::ReadImageFile( wxMemoryBuffer& aBuffer )
{
    if( m_bitmapBase->ReadImageFile( aBuffer ) )
    {
        updatePixelSizeInIU();
        return true;
    }

    return false;
}


bool REFERENCE_IMAGE::SetImage( const wxImage& aImage )
{
    if( m_bitmapBase->SetImage( aImage ) )
    {
        updatePixelSizeInIU();
        return true;
    }

    return false;
}


const BITMAP_BASE& REFERENCE_IMAGE::GetImage() const
{
    // This cannot be null after construction
    return *m_bitmapBase;
}


BITMAP_BASE& REFERENCE_IMAGE::MutableImage() const
{
    return *m_bitmapBase;
}


void REFERENCE_IMAGE::SwapData( REFERENCE_IMAGE& aOther )
{
    std::swap( m_pos, aOther.m_pos );
    std::swap( m_transformOriginOffset, aOther.m_transformOriginOffset );
    std::swap( m_bitmapBase, aOther.m_bitmapBase );
}
