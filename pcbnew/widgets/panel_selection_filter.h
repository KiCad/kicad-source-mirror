/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_PANEL_SELECTION_FILTER_H
#define KICAD_PANEL_SELECTION_FILTER_H

#include <widgets/panel_selection_filter_base.h>
#include <wx/timer.h>
#include <map>


class PCB_SELECTION_TOOL;
struct PCB_SELECTION_FILTER_OPTIONS;

// Forward declare the event type
class PCB_SELECTION_FILTER_EVENT;
wxDECLARE_EVENT( EVT_PCB_SELECTION_FILTER_FLASH, PCB_SELECTION_FILTER_EVENT );

class PCB_SELECTION_FILTER_EVENT : public wxCommandEvent
{
public:
    PCB_SELECTION_FILTER_EVENT( const PCB_SELECTION_FILTER_OPTIONS& aOptions = PCB_SELECTION_FILTER_OPTIONS(), int id = 0 ) :
            wxCommandEvent( EVT_PCB_SELECTION_FILTER_FLASH, id ), m_options( aOptions ) {}

    wxEvent* Clone() const override
    {
        return new PCB_SELECTION_FILTER_EVENT( *this );
    }

    PCB_SELECTION_FILTER_OPTIONS m_options;
};


class PANEL_SELECTION_FILTER : public PANEL_SELECTION_FILTER_BASE
{
public:
    PANEL_SELECTION_FILTER( wxWindow* aParent );

    ~PANEL_SELECTION_FILTER();

    void SetCheckboxesFromFilter( PCB_SELECTION_FILTER_OPTIONS& aOptions );

protected:
    void OnFilterChanged( wxCommandEvent& aEvent ) override;
    void OnLanguageChanged( wxCommandEvent& aEvent );

private:
    bool setFilterFromCheckboxes( PCB_SELECTION_FILTER_OPTIONS& aOptions );

    void onRightClick( wxMouseEvent& aEvent );

    void onPopupSelection( wxCommandEvent& aEvent );

    void flashCheckbox( wxCheckBox* aBox );
    void onFlashTimer( wxTimerEvent& aEvent );
    void OnFlashEvent( PCB_SELECTION_FILTER_EVENT& aEvent );
    void onPanelPaint( wxPaintEvent& aEvent );

private:
    PCB_BASE_EDIT_FRAME*                     m_frame;
    PCB_SELECTION_TOOL*                      m_tool;
    wxCheckBox*                              m_onlyCheckbox;
    std::map<wxCheckBox*, int>               m_flashCounters;
    wxTimer                                  m_flashTimer;
    int                                      m_flashSteps;
    wxColour                                 m_defaultBg;
};


#endif // KICAD_PANEL_SELECTION_FILTER_H
