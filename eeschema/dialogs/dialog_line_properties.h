/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

#include <dialog_line_properties_base.h>
#include <widgets/unit_binder.h>
#include <line_ending.h>


class SCH_EDIT_FRAME;
class SCH_LINE;


class DIALOG_LINE_PROPERTIES : public DIALOG_LINE_PROPERTIES_BASE
{
public:
    DIALOG_LINE_PROPERTIES( SCH_EDIT_FRAME* aParent, std::deque<SCH_LINE*>& aLines );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    void resetDefaults( wxCommandEvent& event ) override;

private:
    void createLineEndingControls( SCH_EDIT_FRAME* aParent );

private:
    SCH_EDIT_FRAME*       m_frame;
    std::deque<SCH_LINE*> m_lines;

    UNIT_BINDER           m_width;

    wxStaticText*     m_startShapeLabel;
    wxBitmapComboBox* m_startShapeChoice;
    wxStaticText*     m_endShapeLabel;
    wxBitmapComboBox* m_endShapeChoice;
    wxStaticText*     m_startLengthLabel;
    wxTextCtrl*       m_startLengthCtrl;
    wxStaticText*     m_startLengthUnits;
    wxStaticText*     m_endLengthLabel;
    wxTextCtrl*       m_endLengthCtrl;
    wxStaticText*     m_endLengthUnits;
    wxStaticText*     m_startWidthLabel;
    wxTextCtrl*       m_startWidthCtrl;
    wxStaticText*     m_startWidthUnits;
    wxStaticText*     m_endWidthLabel;
    wxTextCtrl*       m_endWidthCtrl;
    wxStaticText*     m_endWidthUnits;
    wxStaticText*     m_startThicknessLabel;
    wxTextCtrl*       m_startThicknessCtrl;
    wxStaticText*     m_startThicknessUnits;
    wxStaticText*     m_endThicknessLabel;
    wxTextCtrl*       m_endThicknessCtrl;
    wxStaticText*     m_endThicknessUnits;
    wxStaticText*     m_endingsHelpLabel;

    std::unique_ptr<UNIT_BINDER> m_startLength;
    std::unique_ptr<UNIT_BINDER> m_startWidth;
    std::unique_ptr<UNIT_BINDER> m_startThickness;
    std::unique_ptr<UNIT_BINDER> m_endLength;
    std::unique_ptr<UNIT_BINDER> m_endWidth;
    std::unique_ptr<UNIT_BINDER> m_endThickness;
};
