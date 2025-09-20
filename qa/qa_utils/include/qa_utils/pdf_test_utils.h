/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef QA_UTILS_PDF_TEST_UTILS_H
#define QA_UTILS_PDF_TEST_UTILS_H

#include <memory>
#include <string>

#include <wx/string.h>

#include <gal/color4d.h>
#include <render_settings.h>
#include <font/text_attributes.h>
#include <font/stroke_font.h>

/**
 * Make a temporary file path with .pdf extension using a given prefix.
 */
wxString MakeTempPdfPath( const wxString& aPrefix );

/**
 * Minimal concrete render settings suitable for plotters in tests.
 */
class SIMPLE_RENDER_SETTINGS : public KIGFX::RENDER_SETTINGS
{
public:
    SIMPLE_RENDER_SETTINGS();

    KIGFX::COLOR4D        GetColor( const KIGFX::VIEW_ITEM*, int ) const override;
    const KIGFX::COLOR4D& GetBackgroundColor() const override { return m_background; }
    void                  SetBackgroundColor( const KIGFX::COLOR4D& aColor ) override { m_background = aColor; }
    const KIGFX::COLOR4D& GetGridColor() override { return m_grid; }
    const KIGFX::COLOR4D& GetCursorColor() override { return m_cursor; }

private:
    KIGFX::COLOR4D        m_background;
    KIGFX::COLOR4D        m_grid;
    KIGFX::COLOR4D        m_cursor;
};

/**
 * Build a commonly used set of text attributes for plotting text in tests.
 *
 * @param aSizeIu       Size in internal units (both X and Y)
 * @param aStrokeWidth  Stroke width in internal units (0 for outline fonts)
 * @param aBold         Bold flag
 * @param aItalic       Italic flag
 */
TEXT_ATTRIBUTES BuildTextAttributes( int aSizeIu = 3000, int aStrokeWidth = 300,
                                     bool aBold = false, bool aItalic = false );

/**
 * Load the default stroke font and return a unique_ptr for RAII deletion.
 */
std::unique_ptr<KIFONT::STROKE_FONT> LoadStrokeFontUnique();

/**
 * Read a PDF file and append best-effort decompressed contents of any Flate streams to the
 * returned buffer to make text searches easier.
 *
 * @return true on success; false if file cannot be opened or read.
 */
bool ReadPdfWithDecompressedStreams( const wxString& aPdfPath, std::string& aOutBuffer );

/**
 * Count occurrences of a substring in a string (overlapping allowed).
 */
int CountOccurrences( const std::string& aHaystack, const std::string& aNeedle );

/**
 * Convenience contains check.
 */
inline bool PdfContains( const std::string& aBuffer, const char* aNeedle )
{
    return aBuffer.find( aNeedle ) != std::string::npos;
}

/**
 * Rasterize a PDF page to PNG using pdftoppm if available and count non-near-white pixels.
 *
 * @param aPdfPath          Input PDF path.
 * @param aDpi              Rasterization DPI.
 * @param aNearWhiteThresh  Threshold for each RGB channel to consider white (e.g., 240).
 * @param aOutDarkPixels    Output: number of pixels darker than threshold.
 * @return true if rasterization succeeded and dark pixel count is valid. False if tool not
 *         available or conversion failed.
 */
bool RasterizePdfCountDark( const wxString& aPdfPath, int aDpi, int aNearWhiteThresh,
                            long& aOutDarkPixels );

/**
 * Remove a file unless the given environment variable is set (defaults to KICAD_KEEP_TEST_PDF).
 */
void MaybeRemoveFile( const wxString& aPath, const wxString& aEnvVar = wxT( "KICAD_KEEP_TEST_PDF" ) );

#endif // QA_UTILS_PDF_TEST_UTILS_H
