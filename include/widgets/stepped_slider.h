/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STEPPED_SLIDER_H
#define STEPPED_SLIDER_H

#include <wx/slider.h>

/**
 * Customized wxSlider with forced stepping.
 */
class STEPPED_SLIDER : public wxSlider
{
public:
    STEPPED_SLIDER(
            wxWindow* aParent,
            wxWindowID aId,
            int aValue,
            int aMinValue,
            int aMaxValue,
            const wxPoint& aPos = wxDefaultPosition,
            const wxSize& aSize = wxDefaultSize,
            long aStyle = wxSL_HORIZONTAL,
            const wxValidator& aValidator = wxDefaultValidator,
            const wxString& aName = wxSliderNameStr );

    virtual ~STEPPED_SLIDER();

    /**
     * Set the step size.
     */
    void SetStep( int aSize );

    /**
     * Get the step size.
     */
    int GetStep() const;

protected:
    DECLARE_EVENT_TABLE()

private:
    int m_step;

    void OnScroll( wxScrollEvent& aEvent );
};


#endif // STEPPED_SLIDER_H
