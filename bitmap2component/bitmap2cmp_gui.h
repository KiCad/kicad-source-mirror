/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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
#include <map>
#include <potracelib.h>

enum UNIT
{
    MM = 0,
    INCH,
    MILS,
    DPI
};

class IMAGE_SIZE
{
public:
    IMAGE_SIZE();

    void Set( float aValue, UNIT aUnit );
    void SetUnit( UNIT aUnit )
    {
        m_unit = aUnit;
    }


    void SetInputResolution( int aResolution );
    int  GetInputResolution()
    {
        return m_originalResolution;
    }

    float GetValue();

    int GetOutputDPI();

private:
    UNIT  m_unit;
    float m_value;
    int   m_originalDPI;
    int   m_originalResolution;
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

    /**
     * Generate a schematic library which contains one component:
     * the logo
     */
    void exportEeschemaFormat();

    /**
     * Generate a module in S expr format
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

private:
    wxImage  m_Pict_Image;
    wxBitmap m_Pict_Bitmap;
    wxImage  m_Greyscale_Image;
    wxBitmap m_Greyscale_Bitmap;
    wxImage  m_NB_Image;
    wxBitmap m_BN_Bitmap;
    IMAGE_SIZE                    m_outputSizeX;
    IMAGE_SIZE                    m_outputSizeY;
    bool                          m_Negative;
    wxString                      m_BitmapFileName;
    wxString                      m_ConvertedFileName;
    wxSize                        m_frameSize;
    wxPoint                       m_framePos;
    std::unique_ptr<wxConfigBase> m_config;
    bool                          m_exportToClipboard;
    bool                          m_AspectRatioLocked;
    float                         m_AspectRatio;
    std::map<UNIT, wxString>      m_unitMap = { { MM, _("mm") }, { INCH, _("Inch") }, { MILS, _("Mils") },
        { DPI, _("DPI") } };
};
#endif// BITMOP2CMP_GUI_H_
