/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
#ifndef BITMAP2CMP_PANEL_H
#define BITMAP2CMP_PANEL_H

#include <bitmap2cmp_panel_base.h>
#include <eda_units.h>
#include <wx/dnd.h>

class BITMAP2CMP_FRAME;
class BITMAP2CMP_SETTINGS;


class IMAGE_SIZE
{
public:
    IMAGE_SIZE();

    // Set the unit used for m_outputSize, and convert the old m_outputSize value
    // to the value in new unit
    void SetUnit( EDA_UNITS aUnit );

    void SetOriginalDPI( int aDPI ) { m_originalDPI = aDPI; }

    void SetOriginalSizePixels( int aPixels ) { m_originalSizePixels = aPixels; }
    int  GetOriginalSizePixels() { return m_originalSizePixels; }

    double GetOutputSize() { return m_outputSize; }
    void SetOutputSize( double aSize, EDA_UNITS aUnit ) { m_outputSize = aSize; m_unit = aUnit; }

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


class BITMAP2CMP_PANEL : public BITMAP2CMP_PANEL_BASE
{
public:
    BITMAP2CMP_PANEL( BITMAP2CMP_FRAME* aParent );
    ~BITMAP2CMP_PANEL() {}

    bool OpenProjectFiles( const std::vector<wxString>& aFilenames, int aCtl = 0 );

    void OnLoadFile( wxCommandEvent& event ) override;

    void LoadSettings( BITMAP2CMP_SETTINGS* aCfg );
    void SaveSettings( BITMAP2CMP_SETTINGS* aCfg );

    wxWindow* GetCurrentPage();

    IMAGE_SIZE GetOutputSizeX() const { return m_outputSizeX; }
    IMAGE_SIZE GetOutputSizeY() const { return m_outputSizeY; }
    void SetOutputSize( const IMAGE_SIZE& aSizeX, const IMAGE_SIZE& aSizeY );

    /**
     * generate a export data of the current bitmap.
     * @param aOutput is a string buffer to fill with data
     * @param aFormat is the format to generate
     */
    void ExportToBuffer( std::string& aOutput, OUTPUT_FMT_ID aFormat );

private:
    // Event handlers
    void OnPaintInit( wxPaintEvent& event ) override;
    void OnPaintGreyscale( wxPaintEvent& event ) override;
    void OnPaintBW( wxPaintEvent& event ) override;
    void OnExportToFile( wxCommandEvent& event ) override;
    void OnExportToClipboard( wxCommandEvent& event ) override;
    void OnFormatChange( wxCommandEvent& event ) override;
    void OnNegativeClicked( wxCommandEvent& event ) override;
    void OnThresholdChange( wxScrollEvent& event ) override;
    void OnSizeChangeX( wxCommandEvent& event ) override;
    void OnSizeChangeY( wxCommandEvent& event ) override;
    void OnSizeUnitChange( wxCommandEvent& event ) override;
    void ToggleAspectRatioLock( wxCommandEvent& event ) override;

    OUTPUT_FMT_ID getOutputFormat();

    ///< @return the EDA_UNITS from the m_PixelUnit choice
    EDA_UNITS getUnitFromSelection();

    // return a string giving the output size, according to the selected unit
    wxString formatOutputSize( double aSize );

    void binarize( double aThreshold ); // aThreshold = 0.0 (black level) to 1.0 (white level)

    void negateGreyscaleImage();

    void updateImageInfo();

private:
    BITMAP2CMP_FRAME* m_parentFrame;

    wxImage           m_Pict_Image;
    wxBitmap          m_Pict_Bitmap;
    wxImage           m_Greyscale_Image;
    wxBitmap          m_Greyscale_Bitmap;
    wxImage           m_NB_Image;
    wxBitmap          m_BN_Bitmap;
    IMAGE_SIZE        m_outputSizeX;
    IMAGE_SIZE        m_outputSizeY;
    bool              m_negative;
    double            m_aspectRatio;
};


class DROP_FILE : public wxFileDropTarget
{
public:
    DROP_FILE( BITMAP2CMP_PANEL* panel );
    DROP_FILE() = delete;

    bool OnDropFiles( wxCoord x, wxCoord y, const wxArrayString& filenames ) override;

private:
    BITMAP2CMP_PANEL* m_panel;
};

#endif// BITMAP2CMP_PANEL
