/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef DIALOG_SHAPE_PROPERTIES_H
#define DIALOG_SHAPE_PROPERTIES_H

#include <memory>

class SCH_SHAPE;
class SCH_BASE_FRAME;


#include <dialog_shape_properties_base.h>
#include <widgets/unit_binder.h>
#include <line_ending.h>


class DIALOG_SHAPE_PROPERTIES : public DIALOG_SHAPE_PROPERTIES_BASE
{
public:
    DIALOG_SHAPE_PROPERTIES( SCH_BASE_FRAME* aParent, SCH_SHAPE* aShape );
    ~DIALOG_SHAPE_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool GetApplyToAllConversions() { return m_checkApplyToAllBodyStyles->IsChecked(); }
    bool GetApplyToAllUnits()       { return m_checkApplyToAllUnits->IsChecked(); }

private:
    void onBorderChecked( wxCommandEvent& aEvent) override;
    void onBorderSwatch( wxCommandEvent& aEvent );
    void onFillChoice( wxCommandEvent& event ) override;
    void onFillRadioButton(wxCommandEvent &aEvent) override;
    void onCustomColorSwatch( wxCommandEvent& aEvent );

private:
    SCH_BASE_FRAME* m_frame;
    SCH_SHAPE*      m_shape;
    UNIT_BINDER     m_borderWidth;

    wxBoxSizer*       m_endingsSizer;
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
    wxStaticText*     m_startStrokeWidthLabel;
    wxTextCtrl*       m_startStrokeWidthCtrl;
    wxStaticText*     m_startStrokeWidthUnits;
    wxStaticText*     m_endStrokeWidthLabel;
    wxTextCtrl*       m_endStrokeWidthCtrl;
    wxStaticText*     m_endStrokeWidthUnits;
    wxStaticText*     m_endingsHelpLabel;

    std::unique_ptr<UNIT_BINDER> m_startLength;
    std::unique_ptr<UNIT_BINDER> m_startWidth;
    std::unique_ptr<UNIT_BINDER> m_startStrokeWidth;
    std::unique_ptr<UNIT_BINDER> m_endLength;
    std::unique_ptr<UNIT_BINDER> m_endWidth;
    std::unique_ptr<UNIT_BINDER> m_endStrokeWidth;

    void createLineEndingControls( SCH_BASE_FRAME* aParent );
};

#endif // DIALOG_SHAPE_PROPERTIES_H
