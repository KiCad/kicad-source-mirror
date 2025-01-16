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

#include <widgets/stepped_slider.h>
#include <wx/event.h>


BEGIN_EVENT_TABLE( STEPPED_SLIDER, wxSlider )
    EVT_SCROLL( STEPPED_SLIDER::OnScroll )
END_EVENT_TABLE()


STEPPED_SLIDER::STEPPED_SLIDER( wxWindow* aParent, wxWindowID aId, int aValue, int aMinValue,
                                int aMaxValue, const wxPoint& aPos, const wxSize& aSize,
                                long aStyle, const wxValidator& aValidator,
                                const wxString& aName ) :
        wxSlider( aParent, aId, aValue, aMinValue, aMaxValue, aPos, aSize,
                  ( aStyle | wxSL_AUTOTICKS | wxSL_MIN_MAX_LABELS ),
                  aValidator,
                  aName ),
        m_step( 1 )
{}


STEPPED_SLIDER::~STEPPED_SLIDER()
{}


void STEPPED_SLIDER::SetStep( int aSize )
{
    wxASSERT( aSize > 0 );
    m_step = ( aSize > 0 ) ? aSize : 1;

    // configure pg up/down to increment by our steps
    SetPageSize( aSize );

    //configure arrows to increment by our steps
    SetLineSize( aSize );

#ifdef __WXMSW__
    ClearTicks();

    if( aSize > 1 )
        SetTickFreq( aSize );
#endif // __WXMSW__
}


int STEPPED_SLIDER::GetStep() const
{
    return m_step;
}


void STEPPED_SLIDER::OnScroll( wxScrollEvent& aEvent )
{
    // On Windows, moving the thumb can generate multiple subevents and wx is blindly
    // calling SetValue for each one before calling this event handler
    // We need to explicitly wait until the "final" sub event or else we end up fighting
    // wx on the value it is setting and extreme glitchlyness will occur
    // Not sure if other platforms have this issue
#ifdef __WXMSW__
    if( aEvent.GetEventType() == wxEVT_SCROLL_CHANGED )
    {
#endif // __WXMSW__

        const int value = GetValue();
        const int rounded = value - value % m_step;
        SetValue( rounded );

#ifdef __WXMSW__
    }
#endif // __WXMSW__

    aEvent.Skip();
}
