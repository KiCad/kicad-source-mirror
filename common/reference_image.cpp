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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "reference_image.h"

#include <cmath>
#include <cstdlib>
#include <string>

#include <wx/debug.h>
#include <wx/mstream.h>

#include <bitmap_base.h>
#include <geometry/geometry_utils.h>
#include <plotters/plotter.h>


static bool compareImages( const BITMAP_BASE& aLeft, const BITMAP_BASE& aRight )
{
    const wxImage* leftImage = aLeft.GetImageData();
    const wxImage* rightImage = aRight.GetImageData();

    if( !leftImage || !rightImage )
        return leftImage == rightImage;

    wxMemoryOutputStream leftStream;
    wxMemoryOutputStream rightStream;

    if( !aLeft.SaveImageData( leftStream ) || !aRight.SaveImageData( rightStream ) )
        return false;

    size_t leftSize = leftStream.GetSize();
    size_t rightSize = rightStream.GetSize();

    if( leftSize != rightSize )
        return false;

    if( leftSize == 0 )
        return true;

    std::string leftData( leftSize, '\0' );
    std::string rightData( rightSize, '\0' );

    leftStream.CopyTo( leftData.data(), leftSize );
    rightStream.CopyTo( rightData.data(), rightSize );

    return leftData == rightData;
}


REFERENCE_IMAGE::REFERENCE_IMAGE( const EDA_IU_SCALE& aIuScale ) :
        m_iuScale( aIuScale ),
        m_pos( 0, 0 ),
        m_transformOriginOffset( 0, 0 ),
        m_orientation( ANGLE_0 ),
        m_opacity( 1.0 ),
        m_bitmapBase( std::make_unique<BITMAP_BASE>() )
{
    updatePixelSizeInIU();
}


REFERENCE_IMAGE::REFERENCE_IMAGE( const REFERENCE_IMAGE& aOther ) :
        m_iuScale( aOther.m_iuScale ),
        m_pos( aOther.m_pos ),
        m_transformOriginOffset( aOther.m_transformOriginOffset ),
        m_orientation( aOther.m_orientation ),
        m_opacity( aOther.m_opacity ),
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
        m_orientation = aOther.m_orientation;
        m_opacity = aOther.m_opacity;
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

    if( m_orientation != aOther.m_orientation )
        return false;

    if( m_opacity != aOther.m_opacity )
        return false;

    if( m_bitmapBase->GetSize() != aOther.m_bitmapBase->GetSize() )
        return false;

    if( m_bitmapBase->GetPPI() != aOther.m_bitmapBase->GetPPI() )
        return false;

    if( m_bitmapBase->GetScale() != aOther.m_bitmapBase->GetScale() )
        return false;

    if( !compareImages( *m_bitmapBase, *aOther.m_bitmapBase ) )
        return false;

    return true;
}


double REFERENCE_IMAGE::Similarity( const REFERENCE_IMAGE& aOther ) const
{
    double similarity = 1.0;

    if( m_pos != aOther.m_pos )
        similarity *= 0.9;

    if( m_orientation != aOther.m_orientation )
        similarity *= 0.9;

    if( m_opacity != aOther.m_opacity )
        similarity *= 0.9;

    if( m_bitmapBase->GetSize() != aOther.m_bitmapBase->GetSize() )
        similarity *= 0.9;

    if( m_bitmapBase->GetPPI() != aOther.m_bitmapBase->GetPPI() )
        similarity *= 0.9;

    if( m_bitmapBase->GetScale() != aOther.m_bitmapBase->GetScale() )
        similarity *= 0.9;

    if( !compareImages( *m_bitmapBase, *aOther.m_bitmapBase ) )
        similarity *= 0.9;

    return similarity;
}


BOX2I REFERENCE_IMAGE::GetBoundingBox() const
{
    return BOX2I::ByCenter( m_pos, boundingSize() );
}


VECTOR2I REFERENCE_IMAGE::boundingSize() const
{
    // Un-rotated size of the image.
    const VECTOR2I  size = m_bitmapBase->GetSize();
    const EDA_ANGLE angle = m_orientation.GetAngle();

    if( angle.IsHorizontal() )
        return size;

    if( angle.IsVertical() )
        return VECTOR2I{ size.y, size.x };

    // The raster is stored unrotated, so its axis-aligned extent grows with the orientation.
    const double angleCos = std::abs( angle.Cos() );
    const double angleSin = std::abs( angle.Sin() );

    const double w = size.x * angleCos + size.y * angleSin;
    const double h = size.x * angleSin + size.y * angleCos;

    return VECTOR2I{ KiROUND( w ), KiROUND( h ) };
}


VECTOR2I REFERENCE_IMAGE::GetPosition() const
{
    return m_pos;
}


void REFERENCE_IMAGE::SetPosition( const VECTOR2I& aPos )
{
    const BOX2D newBox = BOX2D::ByCenter( aPos, boundingSize() );

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
    const VECTOR2D newSize = boundingSize() * aRatio;

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


bool REFERENCE_IMAGE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    VECTOR2I newPos = m_pos;
    MIRROR( newPos, aCentre, aFlipDirection );

    const BOX2D newBox = BOX2D::ByCenter( newPos, boundingSize() );

    if( !IsBOX2Safe( newBox ) )
        return false;

    m_pos = newPos;
    m_bitmapBase->Mirror( aFlipDirection );

    // A reflection reverses the sense of the stored rotation.
    m_orientation = -m_orientation;

    MIRROR( m_transformOriginOffset, VECTOR2I( 0, 0 ), aFlipDirection );
    return true;
}


void REFERENCE_IMAGE::Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aCenter, aAngle );
    RotatePoint( m_transformOriginOffset, aAngle );

    m_orientation += aAngle;
}


EDA_ANGLE REFERENCE_IMAGE::GetOrientation() const
{
    return m_orientation.GetAngle();
}


void REFERENCE_IMAGE::SetOrientation( const EDA_ANGLE& aAngle )
{
    m_orientation = aAngle;
}


void REFERENCE_IMAGE::SetOpacity( double aOpacity )
{
    m_opacity = std::clamp( aOpacity, 0.0, 1.0 );
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


void REFERENCE_IMAGE::PackToBytes( std::string& aOutputBytes ) const
{
    wxMemoryOutputStream imageStream;

    if( m_bitmapBase->GetImageData() && m_bitmapBase->SaveImageData( imageStream ) )
    {
        size_t size = imageStream.GetSize();

        if( size > 0 )
        {
            aOutputBytes.resize( size );
            imageStream.CopyTo( aOutputBytes.data(), size );
        }
    }
}


bool REFERENCE_IMAGE::UnpackFromBytes( const std::string& aInputBytes )
{
    if( aInputBytes.empty() )
        return false;

    wxMemoryBuffer imageBuffer;
    imageBuffer.AppendData( aInputBytes.data(), aInputBytes.size() );

    return ReadImageFile( imageBuffer );
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


void REFERENCE_IMAGE::PlotImage( PLOTTER* aPlotter, const VECTOR2I& aPos, const KIGFX::COLOR4D& aDefaultColor,
                                 int aDefaultPensize ) const
{
    const wxImage* image = m_bitmapBase->GetImageData();

    if( !image || !image->IsOk() )
        return;

    // These 2 lines are useful only for plotters that cannot plot a bitmap and plot a
    // rectangle instead of the actual image.
    aPlotter->SetColor( aDefaultColor );
    aPlotter->SetCurrentLineWidth( aDefaultPensize );

    const double    scaleFactor = m_bitmapBase->GetScalingFactor();
    const EDA_ANGLE angle = m_orientation.GetAngle();
    const bool      faded = m_opacity < 1.0;

    if( angle.IsZero() && !faded )
    {
        aPlotter->PlotImage( *image, aPos, scaleFactor );
        return;
    }

    // Avoid modifying the original image via the shared buffer.
    wxImage turned = image->Copy();

    if( angle == ANGLE_90 )
    {
        turned = turned.Rotate90( false );
    }
    else if( angle == ANGLE_180 )
    {
        turned = turned.Rotate180();
    }
    else if( angle == ANGLE_270 )
    {
        turned = turned.Rotate90( true );
    }
    else if( !angle.IsZero() )
    {
        // Avoid opaque corners after non-cardinal rotation
        if( !turned.HasAlpha() )
            turned.InitAlpha();

        const wxPoint centerPoint( turned.GetWidth() / 2, turned.GetHeight() / 2 );
        turned = turned.Rotate( angle.AsRadians(), centerPoint, true );
    }

    wxCHECK_MSG( turned.IsOk(), /* void */, wxT( "Failed to rotate image" ) );

    // Apply fading if the image opacity is less than 1.0
    if( faded )
    {
        if( !turned.HasAlpha() )
            turned.InitAlpha();

        unsigned char* alpha = turned.GetAlpha();
        const int      pixels = turned.GetWidth() * turned.GetHeight();

        // Post-multiply the alpha channel by the image overall opacity
        for( int ii = 0; ii < pixels; ++ii )
            alpha[ii] = static_cast<unsigned char>( KiROUND( alpha[ii] * m_opacity ) );
    }

    aPlotter->PlotImage( turned, aPos, scaleFactor );
}


void REFERENCE_IMAGE::SwapData( REFERENCE_IMAGE& aOther )
{
    std::swap( m_pos, aOther.m_pos );
    std::swap( m_transformOriginOffset, aOther.m_transformOriginOffset );
    std::swap( m_orientation, aOther.m_orientation );
    std::swap( m_opacity, aOther.m_opacity );
    std::swap( m_bitmapBase, aOther.m_bitmapBase );
}
