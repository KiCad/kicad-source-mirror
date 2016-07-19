/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file buffers_debug.cpp
 * @brief
 */

#include "buffers_debug.h"
#include <wx/image.h>   // Used for save an image to disk

/**
 * @brief dbg_save_rgb_buffer
 * @param aFileName
 * @param aRGBpixelBuffer: from wxWidget documentation
 * "The data given must have the size (width*height*3).
 *  The data must have been allocated with malloc(), NOT with operator new."
 * @param aXSize
 * @param aYSize
 */
static void dbg_save_rgb_buffer( wxString aFileName,
                                 unsigned char *aRGBpixelBuffer,
                                 unsigned int aXSize,
                                 unsigned int aYSize )
{
    wxImage image( aXSize, aYSize );
    image.SetData( aRGBpixelBuffer );
    image = image.Mirror( false );
    image.SaveFile( aFileName + ".png", wxBITMAP_TYPE_PNG );
    image.Destroy();
}


void DBG_SaveBuffer( wxString aFileName,
                     const unsigned char *aInBuffer,
                     unsigned int aXSize,
                     unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        unsigned char v = aInBuffer[i];

        // Set RGB value with all same values intensities
        pixelbuffer[i * 3 + 0] = v;
        pixelbuffer[i * 3 + 1] = v;
        pixelbuffer[i * 3 + 2] = v;
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( wxString aFileName,
                     const float *aInBuffer,
                     unsigned int aXSize,
                     unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const unsigned char v = (unsigned char)glm::min( (int)(aInBuffer[i] * 255.0f),
                                                         255 );

        // Set RGB value with all same values intensities
        pixelbuffer[i * 3 + 0] = v;
        pixelbuffer[i * 3 + 1] = v;
        pixelbuffer[i * 3 + 2] = v;
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( wxString aFileName,
                     const SFVEC3F *aInBuffer,
                     unsigned int aXSize,
                     unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const SFVEC3F &v = aInBuffer[i];
        const unsigned int ix3 = i * 3;

        // Set RGB value with all same values intensities
        pixelbuffer[ix3 + 0] = (unsigned char)glm::min( (int)(v.r * 255.0f), 255 );
        pixelbuffer[ix3 + 1] = (unsigned char)glm::min( (int)(v.g * 255.0f), 255 );
        pixelbuffer[ix3 + 2] = (unsigned char)glm::min( (int)(v.b * 255.0f), 255 );
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveNormalsBuffer( wxString aFileName,
                            const SFVEC3F *aInNormalsBuffer,
                            unsigned int aXSize,
                            unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const SFVEC3F &v = aInNormalsBuffer[i];
        const unsigned int ix3 = i * 3;

        // Set RGB value with all same values intensities
        pixelbuffer[ix3 + 0] = (unsigned char)glm::min( (int)((v.r + 1.0f) * 127.0f), 255 );
        pixelbuffer[ix3 + 1] = (unsigned char)glm::min( (int)((v.g + 1.0f) * 127.0f), 255 );
        pixelbuffer[ix3 + 2] = (unsigned char)glm::min( (int)((v.b + 1.0f) * 127.0f), 255 );
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}
