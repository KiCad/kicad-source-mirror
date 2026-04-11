/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GERBER_TO_PNG_H
#define GERBER_TO_PNG_H

#include <wx/string.h>
#include <wx/arrstr.h>
#include <gal/color4d.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <memory>

class GERBER_FILE_IMAGE;
class JOB_GERBER_EXPORT_PNG;


/**
 * Determine if a file is an Excellon drill file based on extension.
 */
bool IsExcellonFile( const wxString& aPath );


/**
 * Calculate bounding box for all draw items in a gerber image.
 */
BOX2I CalculateGerberBoundingBox( GERBER_FILE_IMAGE* aImage );


/**
 * Load a Gerber or Excellon file, auto-detecting by extension.
 *
 * @return Loaded image, or nullptr on failure
 */
std::unique_ptr<GERBER_FILE_IMAGE> LoadGerberOrExcellon( const wxString& aPath, wxString* aErrorMsg,
                                                         wxArrayString* aMessages = nullptr );


/**
 * Render options for Gerber to PNG conversion.
 */
struct GERBER_RENDER_OPTIONS
{
    int            dpi = 300;
    int            width = 0;  ///< 0 = calculate from DPI
    int            height = 0; ///< 0 = calculate from DPI
    bool           antialias = true;
    KIGFX::COLOR4D foregroundColor = KIGFX::COLOR4D::BLACK;
    KIGFX::COLOR4D backgroundColor = KIGFX::COLOR4D( 1.0, 1.0, 1.0, 0.0 ); ///< Transparent white

    double originXMm = 0.0;        ///< Viewport origin X in mm
    double originYMm = 0.0;        ///< Viewport origin Y in mm
    double windowWidthMm = 0.0;    ///< Viewport width in mm (> 0 enables viewport mode)
    double windowHeightMm = 0.0;   ///< Viewport height in mm (> 0 enables viewport mode)

    bool HasViewportOverride() const
    {
        return windowWidthMm > 0.0 && windowHeightMm > 0.0;
    }
};


/**
 * Computed plotter viewport parameters from a bounding box and render settings.
 */
struct GERBER_PLOTTER_VIEWPORT
{
    int      width;
    int      height;
    double   plotScale;
    double   iuPerDecimil;
    VECTOR2I offset;
};

static constexpr int MIN_PIXEL_SIZE = 10;

/**
 * Compute pixel dimensions and plotter scale from a bounding box and DPI/size settings.
 *
 * @param aBBox Bounding box in gerber IU
 * @param aDpi DPI for auto-sizing
 * @param aWidth Requested width (0 = auto from DPI)
 * @param aHeight Requested height (0 = auto from DPI)
 */
GERBER_PLOTTER_VIEWPORT CalculatePlotterViewport( const BOX2I& aBBox, int aDpi, int aWidth, int aHeight );


/**
 * Render a Gerber or Excellon file to PNG.
 *
 * Loads the file, converts all draw items to polygons, and renders
 * using PNG_PLOTTER with Cairo.
 *
 * @param aInputPath Path to the Gerber or Excellon file
 * @param aOutputPath Path for the output PNG file
 * @param aOptions Rendering options
 * @param aErrorMsg Optional pointer to receive error messages
 * @param aMessages Optional pointer to receive parse messages
 * @return true on success, false on failure
 */
bool RenderGerberToPng( const wxString& aInputPath, const wxString& aOutputPath, const GERBER_RENDER_OPTIONS& aOptions,
                        wxString* aErrorMsg = nullptr, wxArrayString* aMessages = nullptr );


/**
 * Render a Gerber or Excellon file to PNG using job parameters.
 *
 * @param aInputPath Path to the Gerber or Excellon file
 * @param aOutputPath Path for the output PNG file
 * @param aJob Job containing render parameters
 * @param aErrorMsg Optional pointer to receive error messages
 * @param aMessages Optional pointer to receive parse messages
 * @return true on success, false on failure
 */
bool RenderGerberToPng( const wxString& aInputPath, const wxString& aOutputPath, const JOB_GERBER_EXPORT_PNG& aJob,
                        wxString* aErrorMsg = nullptr, wxArrayString* aMessages = nullptr );


#endif // GERBER_TO_PNG_H
