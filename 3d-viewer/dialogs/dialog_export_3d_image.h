/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include "dialog_shim.h"
#include <3d_viewer/eda_3d_viewer_frame.h> // for EDA_3D_VIEWER_EXPORT_FORMAT
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>

enum class SIZE_UNITS
{
    PIXELS,
    PERCENT,
    MM,
    INCHES
};

enum class RESOLUTION_UNITS
{
    PIXELS_PER_INCH,
    PIXELS_PER_MM
};

class DIALOG_EXPORT_3D_IMAGE : public DIALOG_SHIM
{
public:
    DIALOG_EXPORT_3D_IMAGE( wxWindow* aParent,
                            const wxSize& aSize );

    wxSize GetSize() const { return wxSize( m_width, m_height ); }
    double GetXResolution() const { return m_xResolution; }
    double GetYResolution() const { return m_yResolution; }

private:
    bool TransferDataFromWindow() override;

    void OnLockToggle( wxCommandEvent& aEvent );
    void OnWidthChange( wxSpinDoubleEvent& aEvent );
    void OnHeightChange( wxSpinDoubleEvent& aEvent );
    void OnXResolutionChange( wxSpinDoubleEvent& aEvent );
    void OnYResolutionChange( wxSpinDoubleEvent& aEvent );
    void OnSizeUnitChange( wxCommandEvent& aEvent );
    void OnResolutionUnitChange( wxCommandEvent& aEvent );

    wxSize GetPixelSize( double aWidth, double aHeight, double aXResolution, double aYResolution, SIZE_UNITS aSizeUnits ) const;
    void UpdatePixelSize();
    void UpdateAspectRatio();
    void ConvertSizeUnits( SIZE_UNITS aFromUnit, SIZE_UNITS aToUnit );
    void ConvertResolutionUnits( RESOLUTION_UNITS aFromUnit, RESOLUTION_UNITS aToUnit );

private:
    EDA_3D_VIEWER_EXPORT_FORMAT m_format;
    wxSize m_originalSize;
    int m_width;
    int m_height;
    double m_xResolution;
    double m_yResolution;
    double m_aspectRatio;
    bool m_lockAspectRatio;

    SIZE_UNITS m_sizeUnits;
    RESOLUTION_UNITS m_resolutionUnits;

    wxSpinCtrlDouble* m_spinWidth;
    wxSpinCtrlDouble* m_spinHeight;
    wxSpinCtrlDouble* m_spinXResolution;
    wxSpinCtrlDouble* m_spinYResolution;
    wxChoice* m_choiceSizeUnits;
    wxChoice* m_choiceResolutionUnits;
    wxBitmapButton* m_lockButton;
    wxStaticText* m_pixelSizeLabel;
};