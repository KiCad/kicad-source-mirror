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

#ifndef GERBER_DIFF_H
#define GERBER_DIFF_H

#include <geometry/shape_poly_set.h>
#include <math/vector2d.h>
#include <gal/color4d.h>
#include <nlohmann/json_fwd.hpp>
#include <wx/string.h>


/**
 * Result of a Gerber diff calculation containing the geometric differences
 * and area statistics.
 */
struct GERBER_DIFF_RESULT
{
    SHAPE_POLY_SET overlap;   ///< Areas present in both files (file1 AND file2)
    SHAPE_POLY_SET additions; ///< Areas in file2 but not file1 (file2 AND NOT file1)
    SHAPE_POLY_SET removals;  ///< Areas in file1 but not file2 (file1 AND NOT file2)

    double referenceArea;  ///< Total area of file1 in mm^2
    double comparisonArea; ///< Total area of file2 in mm^2
    double overlapArea;    ///< Area of overlap in mm^2
    double additionsArea;  ///< Area of additions in mm^2
    double removalsArea;   ///< Area of removals in mm^2

    double additionsPercent; ///< Additions as percent of reference area
    double removalsPercent;  ///< Removals as percent of reference area
    double netChangePercent; ///< Net change as percent of reference area

    GERBER_DIFF_RESULT() :
            referenceArea( 0.0 ),
            comparisonArea( 0.0 ),
            overlapArea( 0.0 ),
            additionsArea( 0.0 ),
            removalsArea( 0.0 ),
            additionsPercent( 0.0 ),
            removalsPercent( 0.0 ),
            netChangePercent( 0.0 )
    {
    }
};


/**
 * Calculate the alignment offset to make a comparison poly set align with a reference.
 *
 * Uses bounding box origin alignment.
 *
 * @param aReference The reference poly set (will not be modified)
 * @param aComparison The comparison poly set (offset will translate this to align)
 * @return Translation vector to apply to aComparison to align with aReference
 */
VECTOR2I CalculateAlignment( const SHAPE_POLY_SET& aReference, const SHAPE_POLY_SET& aComparison );


/**
 * Calculate the geometric differences between two poly sets.
 *
 * Computes overlap, additions (in comparison but not reference), and removals
 * (in reference but not comparison) using boolean operations.
 *
 * @param aReference The reference poly set
 * @param aComparison The comparison poly set
 * @return GERBER_DIFF_RESULT with geometry and statistics
 */
GERBER_DIFF_RESULT CalculateGerberDiff( const SHAPE_POLY_SET& aReference, const SHAPE_POLY_SET& aComparison );


/**
 * Units for diff result formatting.
 */
enum class DIFF_UNITS
{
    MM,
    INCH,
    MILS
};


/**
 * Format a diff result as human-readable text.
 *
 * @param aResult The diff result to format
 * @param aFile1 Name of the reference file
 * @param aFile2 Name of the comparison file
 * @param aUnits Output units for area values
 * @return Formatted text string
 */
wxString FormatDiffResultText( const GERBER_DIFF_RESULT& aResult, const wxString& aFile1, const wxString& aFile2,
                               DIFF_UNITS aUnits = DIFF_UNITS::MM );


/**
 * Format a diff result as JSON.
 *
 * @param aResult The diff result to format
 * @param aFile1 Name of the reference file
 * @param aFile2 Name of the comparison file
 * @param aUnits Output units for area values
 * @param aMaxDiffPercent Threshold for within_threshold field (0 = any diff fails)
 * @return JSON object
 */
nlohmann::json FormatDiffResultJson( const GERBER_DIFF_RESULT& aResult, const wxString& aFile1, const wxString& aFile2,
                                     DIFF_UNITS aUnits = DIFF_UNITS::MM, double aMaxDiffPercent = 0.0 );


/**
 * Options for diff PNG rendering.
 */
struct DIFF_RENDER_OPTIONS
{
    int  dpi = 300;
    int  width = 0;  ///< 0 = calculate from DPI
    int  height = 0; ///< 0 = calculate from DPI
    bool antialias = true;

    KIGFX::COLOR4D colorOverlap = KIGFX::COLOR4D( 0.5, 0.5, 0.5, 1.0 );    ///< Gray
    KIGFX::COLOR4D colorAdditions = KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 );  ///< Green
    KIGFX::COLOR4D colorRemovals = KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 );   ///< Red
    KIGFX::COLOR4D colorBackground = KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ); ///< White
};


/**
 * Render a diff result to PNG with colored regions.
 *
 * Draws overlap in gray, additions in green, and removals in red.
 *
 * @param aResult The diff result containing geometry
 * @param aOutputPath Path for the output PNG file
 * @param aOptions Rendering options
 * @return true on success, false on failure
 */
bool RenderDiffToPng( const GERBER_DIFF_RESULT& aResult, const wxString& aOutputPath,
                      const DIFF_RENDER_OPTIONS& aOptions );


#endif // GERBER_DIFF_H
