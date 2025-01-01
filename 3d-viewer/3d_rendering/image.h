/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file image.h
 * @brief one 8bit-channel image definition.
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <wx/string.h>

/// Image operation type
enum class IMAGE_OP
{
    RAW,
    ADD,
    SUB,
    DIF,
    MUL,
    AND,
    OR,
    XOR,
    BLEND50,
    MIN,
    MAX
};


/// Image wrap type enumeration
enum class IMAGE_WRAP
{
    ZERO,  ///< Coords that wraps are not evaluated
    CLAMP, ///< Coords are clamped to image size
    WRAP   ///< Coords are wrapped around
};


/// Filter type enumeration
enum class IMAGE_FILTER
{
    HIPASS,
    GAUSSIAN_BLUR,
    GAUSSIAN_BLUR2,
    INVERT_BLUR,
    CARTOON,
    EMBOSS,
    SHARPEN,
    MELT,
    SOBEL_GX,
    SOBEL_GY,
    BLUR_3X3,
};

/// 5x5 Filter struct parameters
struct S_FILTER
{
    signed char    kernel[5][5];
    unsigned int   div;
    unsigned char  offset;
};


/**
 * Manage an 8-bit channel image.
 */
class IMAGE
{
public:
    /**
     * Construct a IMAGE based on image size.
     *
     * @param aXsize x size
     * @param aYsize y size
     */
    IMAGE( unsigned int aXsize, unsigned int aYsize );

    /**
     * Construct a IMAGE based from an existing image.
     *
     * It will make a copy the \a aSrcImage.
     *
     * @param aSrcImage
     */
    IMAGE( const IMAGE& aSrcImage );

    ~IMAGE();

    /**
     * Set a value in a pixel position, position is clamped in accordance with the
     * current clamp settings.
     *
     * @param aX x position
     * @param aY y position
     * @param aValue value to set the pixel
     */
    void Setpixel( int aX, int aY, unsigned char aValue );

    /**
     * Get the pixel value from pixel position, position is clamped in accord with the
     * current clamp settings.
     *
     * @param aX x position
     * @param aY y position
     * @return unsigned char - pixel value
     */
    unsigned char Getpixel( int aX, int aY ) const;

    /**
     * Draw a horizontal line.
     *
     * @param aXStart x start position
     * @param aXEnd x end position
     * @param aY y position
     * @param aValue  value to add
     */
    void Hline( int aXStart, int aXEnd, int aY, unsigned char aValue );

    void CircleFilled( int aCx, int aCy, int aRadius, unsigned char aValue );

    /**
     * Perform a copy operation based on \a aOperation type.
     *
     * The available image operations.
     *  - IMAGE_OP::RAW        this <- aImgA
     *  - IMAGE_OP::ADD        this <- CLAMP(aImgA + aImgB)
     *  - IMAGE_OP::SUB        this <- CLAMP(aImgA - aImgB)
     *  - IMAGE_OP::DIF        this <- abs(aImgA - aImgB)
     *  - IMAGE_OP::MUL        this <- aImgA * aImgB
     *  - IMAGE_OP::AND        this <- aImgA & aImgB
     *  - IMAGE_OP::OR         this <- aImgA | aImgB
     *  - IMAGE_OP::XOR        this <- aImgA ^ aImgB
     *  - IMAGE_OP::BLEND50    this <- (aImgA + aImgB) / 2
     *  - IMAGE_OP::MIN        this <- (aImgA < aImgB) ? aImgA : aImgB
     *  - IMAGE_OP::MAX        this <- (aImgA > aImgB) ? aImgA : aImgB
     *
     * @param aImgA an image input.
     * @param aImgB an image input.
     * @param aOperation operation to perform
     */
    void CopyFull( const IMAGE* aImgA, const IMAGE* aImgB, IMAGE_OP aOperation );

    /**
     * Invert the values of this image <- (255 - this)
     */
    void Invert();

    /**
     * Apply a filter to the input image and store it in the image class.
     *
     * @param aInImg input image
     * @param aFilterType filter type to apply
     */
    void EfxFilter( IMAGE* aInImg, IMAGE_FILTER aFilterType );

    /**
     * Apply a filter to the input image and store it in the image class.
     * skip the circle center defined by radius
     *
     * @param aInImg input image
     * @param aFilterType filter type to apply
     * @param aRadius center circle that the effect will not be applied
     */
    void EfxFilter_SkipCenter( IMAGE* aInImg, IMAGE_FILTER aFilterType, unsigned int aRadius );

    /**
     * Save image buffer to a PNG file into the working folder.
     *
     * Each RGB channel will have the 8bit-channel from the image.
     *
     * @param aFileName file name (without extension)
     */
    void SaveAsPNG( const wxString& aFileName ) const;

    /**
     * Set the current channel from a float normalized (0.0 - 1.0) buffer.
     *
     * this <- CLAMP(NormalizedFloat * 255)
     *
     * @param aNormalizedFloatArray a float array with the same size of the image
     */
    void SetPixelsFromNormalizedFloat( const float* aNormalizedFloatArray );

    /**
     * Get the image buffer pointer.
     *
     * @return unsigned char* the pointer of the buffer 8bit channel.
     */
    unsigned char* GetBuffer() const;

    unsigned int GetWidth() const { return m_width; }
    unsigned int GetHeight() const { return m_height; }

private:
    /**
     * Calculate the coordinates points in accord with the current clamping settings.
     *
     * @param aXo X coordinate to be converted (output).
     * @param aXo Y coordinate to be converted (output).
     * @return bool - true if the coordinates are inside the image, false otherwise.
     */
    bool wrapCoords( int* aXo, int* aYo ) const;

    void plot8CircleLines( int aCx, int aCy, int aX, int aY, unsigned char aValue );

    unsigned char*  m_pixels;           ///< buffer to store the image 8bit-channel
    unsigned int    m_width;            ///< width of the image
    unsigned int    m_height;           ///< height of the image
    unsigned int    m_wxh;              ///< width * height precalc value
    IMAGE_WRAP      m_wraping;          ///< current wrapping type
};

#endif   // IMAGE_H
