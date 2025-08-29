/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_PANEL_SCH_SELECTION_FILTER_H
#define KICAD_PANEL_SCH_SELECTION_FILTER_H

#include <widgets/panel_sch_selection_filter_base.h>
#include <wx/timer.h>
#include <map>

class SCH_BASE_FRAME;
class SCH_SELECTION_TOOL;
struct SCH_SELECTION_FILTER_OPTIONS;

// Forward declare the event type
class SCH_SELECTION_FILTER_EVENT;
wxDECLARE_EVENT( EVT_SCH_SELECTION_FILTER_FLASH, SCH_SELECTION_FILTER_EVENT );

class SCH_SELECTION_FILTER_EVENT : public wxCommandEvent
{
public:
    SCH_SELECTION_FILTER_EVENT( const SCH_SELECTION_FILTER_OPTIONS& aOptions = SCH_SELECTION_FILTER_OPTIONS(), int id = 0 ) :
            wxCommandEvent( EVT_SCH_SELECTION_FILTER_FLASH, id ), m_options( aOptions ) {}

    wxEvent* Clone() const override
    {
        return new SCH_SELECTION_FILTER_EVENT( *this );
    }

    SCH_SELECTION_FILTER_OPTIONS m_options;
};


class PANEL_SCH_SELECTION_FILTER : public PANEL_SCH_SELECTION_FILTER_BASE
{
public:
    PANEL_SCH_SELECTION_FILTER( wxWindow* aParent );

    ~PANEL_SCH_SELECTION_FILTER();

    void SetCheckboxesFromFilter( SCH_SELECTION_FILTER_OPTIONS& aOptions );

protected:
    void OnFilterChanged( wxCommandEvent& aEvent ) override;
    void OnLanguageChanged( wxCommandEvent& aEvent );

private:
    bool setFilterFromCheckboxes( SCH_SELECTION_FILTER_OPTIONS& aOptions );

    void onRightClick( wxMouseEvent& aEvent );

    void onPopupSelection( wxCommandEvent& aEvent );

    void flashCheckbox( wxCheckBox* aBox );
    void onFlashTimer( wxTimerEvent& aEvent );
    void OnFlashEvent( SCH_SELECTION_FILTER_EVENT& aEvent );
    void onPanelPaint( wxPaintEvent& aEvent );

private:
    SCH_BASE_FRAME*                     m_frame;
    SCH_SELECTION_TOOL*                 m_tool;
    wxCheckBox*                         m_onlyCheckbox;
    std::map<wxCheckBox*, int>          m_flashCounters;
    wxTimer                             m_flashTimer;
    int                                 m_flashSteps;
    wxColour                            m_defaultBg;
};

#endif //KICAD_PANEL_SCH_SELECTION_FILTER_H
