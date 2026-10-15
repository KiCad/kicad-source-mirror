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

#ifndef BITMAP_IMPORT_PLUGIN_H
#define BITMAP_IMPORT_PLUGIN_H

#include "graphics_import_plugin.h"

#include <vector>
#include <wx/image.h>


/**
 * Import a raster image by tracing its dark areas into filled polygons.
 *
 * The image is reduced to black and white at a threshold, then traced. Geometry is
 * reported in millimetres, taken from the image resolution.
 */
class BITMAP_IMPORT_PLUGIN : public GRAPHICS_IMPORT_PLUGIN
{
public:
    /// Used when the caller offers the user no threshold setting.
    static constexpr double DEFAULT_THRESHOLD = 0.5;

    /// Resolution assumed for images that do not carry one.
    static constexpr int DEFAULT_DPI = 300;

    const wxString GetName() const override { return "Bitmap image"; }

    const std::vector<std::string> GetFileExtensions() const override
    {
        static std::vector<std::string> exts = { "png", "jpg", "jpeg", "bmp", "gif" };
        return exts;
    }

    const wxString& GetMessages() const override { return m_messages; }

    void ReportMsg( const wxString& aMessage ) override;

    bool Load( const wxString& aFileName ) override;
    bool LoadFromMemory( const wxMemoryBuffer& aMemBuffer ) override;
    bool Import() override;

    double GetImageWidth() const override;
    double GetImageHeight() const override;
    BOX2D  GetImageBBox() const override;

    /// The loaded image, so a caller showing a preview does not have to read the file again.
    const wxImage& GetImage() const { return m_image; }

    /// Brightness cut deciding which pixels are traced, from 0 to 1.
    void SetThreshold( double aThreshold ) { m_threshold = aThreshold; }

    /// Invert the image before the threshold is applied.
    void SetInvert( bool aInvert ) { m_invert = aInvert; }

private:
    double pixelSizeXmm() const;
    double pixelSizeYmm() const;

    wxImage  m_image;
    double   m_threshold = DEFAULT_THRESHOLD;
    bool     m_invert = false;
    wxString m_messages;
};

#endif // BITMAP_IMPORT_PLUGIN_H
