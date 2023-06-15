/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <kiway_player.h>
#include <bitmap2cmp_frame.h>

#include <eda_units.h>
#include <potracelib.h>

class BITMAP2CMP_PANEL;

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


class BITMAP2CMP_FRAME : public KIWAY_PLAYER
{
public:
    BITMAP2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~BITMAP2CMP_FRAME();

    // overload KIWAY_PLAYER virtual
    bool OpenProjectFiles( const std::vector<wxString>& aFilenames, int aCtl = 0 ) override;

    void OnExit( wxCommandEvent& event );
    void OnLoadFile( wxCommandEvent& event );

    /**
     * Generate a schematic library which contains one component:
     * the logo
     */
    void ExportEeschemaFormat();

    /**
     * Generate a footprint in S expr format
     */
    void ExportPcbnewFormat();

    /**
     * Generate a postscript file
     */
    void ExportPostScriptFormat();

    /**
     * Generate a file suitable to be copied into a drawing sheet (.kicad_wks) file
     */
    void ExportLogo();

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    wxWindow* GetToolCanvas() const override;

DECLARE_EVENT_TABLE()

protected:
    void doReCreateMenuBar() override;

private:
    BITMAP2CMP_PANEL* m_panel;
    wxStatusBar*      m_statusBar;

    wxString          m_bitmapFileName;
    wxString          m_convertedFileName;
};
#endif// BITMOP2CMP_GUI_H_
