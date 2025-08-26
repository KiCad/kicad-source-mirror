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
 * Inspired by the Inkscape preferences widget, which is
 * Authors:
 *   Marco Scholten
 *   Bruno Dilly <bruno.dilly@gmail.com>
 *
 * Copyright (C) 2004, 2006, 2007 Authors
 *
 * Released under GNU GPL v2+
 */


#ifndef ZOOM_CORRECTION_CTRL__H_
#define ZOOM_CORRECTION_CTRL__H_

#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>

class ZOOM_CORRECTION_RULER;

enum class ZOOM_CORRECTION_UNITS : int
{
    MM,
    CM,
    INCH
};

/**
 * Control to calibrate screen zoom to match real-world size.
 */
class ZOOM_CORRECTION_CTRL : public wxPanel
{
public:
    ZOOM_CORRECTION_CTRL( wxWindow* aParent, double& aValue );

    void   SetDisplayedValue( double aValue );
    double GetValue() const;
    int    GetUnitsSelection() const;
    bool   TransferDataToWindow() override;
    bool   TransferDataFromWindow() override;

private:
    void sliderChanged( wxCommandEvent& aEvent );
    void unitsChanged( wxCommandEvent& aEvent );
    void spinnerChanged( wxSpinEvent& aEvent );

    double*                m_value;
    ZOOM_CORRECTION_RULER* m_ruler;
    wxSlider*              m_slider;
    wxSpinCtrl*            m_spinner;
    wxChoice*              m_unitsChoice;
};

#endif
