/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 Kicad Developers, see AUTHORS.txt for contributors.
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
#ifndef BITMOP2CMP_GUI_H_
#define BITMOP2CMP_GUI_H_

#include "bitmap2component.h"

#include "bitmap2cmp_gui_base.h"
#include <eda_units.h>
#include <potracelib.h>


class IMAGE_SIZE
{
public:
    IMAGE_SIZE();

    // Set the unit used for m_outputSize, and convert the old m_outputSize value
    // to the value in new unit
    void SetUnit( EDA_UNITS aUnit );

    // Accessors:
    void SetOriginalDPI( int aDPI )
    {
        m_originalDPI = aDPI;
    }

    void SetOriginalSizePixels( int aPixels )
    {
        m_originalSizePixels = aPixels;
    }

    double GetOutputSize()
    {
        return m_outputSize;
    }

    void SetOutputSize( double aSize, EDA_UNITS aUnit )
    {
        m_unit = aUnit;
        m_outputSize = aSize;
    }

    int  GetOriginalSizePixels()
    {
        return m_originalSizePixels;
    }

    // Set the m_outputSize value from the m_originalSizePixels and the selected unit
    void SetOutputSizeFromInitialImageSize();

    /** @return the pixels per inch value to build the output image.
     * It is used by potrace to build the polygonal image
     */
    int GetOutputDPI();

private:
    EDA_UNITS m_unit;                 // The units for m_outputSize (mm, inch, dpi)
    double    m_outputSize;           // The size in m_unit of the output image, depending on
                                      // the user settings. Set to the initial image size
    int       m_originalDPI;          // The image DPI if specified in file, or 0 if unknown
    int       m_originalSizePixels;   // The original image size read from file, in pixels
};

class BM2CMP_FRAME : public BM2CMP_FRAME_BASE
{
public:
    BM2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~BM2CMP_FRAME();

    // overload KIWAY_PLAYER virtual
    bool OpenProjectFiles( const std::vector<wxString>& aFilenames, int aCtl = 0 ) override;

private:
    // Event handlers
    void OnPaintInit( wxPaintEvent& event ) override;
    void OnPaintGreyscale( wxPaintEvent& event ) override;
    void OnPaintBW( wxPaintEvent& event ) override;
    void OnLoadFile( wxCommandEvent& event ) override;
    void OnExportToFile( wxCommandEvent& event ) override;
    void OnExportToClipboard( wxCommandEvent& event ) override;

    ///< @return the EDA_UNITS from the m_PixelUnit choice
    EDA_UNITS getUnitFromSelection();

    // return a string giving the output size, according to the selected unit
    wxString FormatOutputSize( double aSize );

    /**
     * Generate a schematic library which contains one component:
     * the logo
     */
    void exportEeschemaFormat();

    /**
     * Generate a footprint in S expr format
     */
    void exportPcbnewFormat();

    /**
     * Generate a postscript file
     */
    void exportPostScriptFormat();

    /**
     * Generate a file suitable to be copied into a page layout
     * description file (.kicad_wks file
     */
    void OnExportLogo();

    void Binarize( double aThreshold ); // aThreshold = 0.0 (black level) to 1.0 (white level)
    void OnNegativeClicked( wxCommandEvent& event ) override;
    void OnThresholdChange( wxScrollEvent& event ) override;

    void OnSizeChangeX( wxCommandEvent& event ) override;
    void OnSizeChangeY( wxCommandEvent& event ) override;
    void OnSizeUnitChange( wxCommandEvent& event ) override;

    void ToggleAspectRatioLock( wxCommandEvent& event ) override;


    void NegateGreyscaleImage();
    /**
     * generate a export data of the current bitmap.
     * @param aOutput is a string buffer to fill with data
     * @param aFormat is the format to generate
     */
    void ExportToBuffer( std::string& aOutput, OUTPUT_FMT_ID aFormat );

    void updateImageInfo();
    void OnFormatChange( wxCommandEvent& event ) override;
    void exportBitmap( OUTPUT_FMT_ID aFormat );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    wxWindow* GetToolCanvas() const override;

private:
    wxImage    m_Pict_Image;
    wxBitmap   m_Pict_Bitmap;
    wxImage    m_Greyscale_Image;
    wxBitmap   m_Greyscale_Bitmap;
    wxImage    m_NB_Image;
    wxBitmap   m_BN_Bitmap;
    IMAGE_SIZE m_outputSizeX;
    IMAGE_SIZE m_outputSizeY;
    bool       m_Negative;
    wxString   m_BitmapFileName;
    wxString   m_ConvertedFileName;
    bool       m_exportToClipboard;
    bool       m_AspectRatioLocked;
    double     m_AspectRatio;
};
#endif// BITMOP2CMP_GUI_H_
