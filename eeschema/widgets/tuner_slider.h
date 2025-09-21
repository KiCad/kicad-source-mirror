/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef TUNER_SLIDER_H
#define TUNER_SLIDER_H

#include "tuner_slider_base.h"

#include <sim/spice_value.h>
#include <sim/spice_generator.h>

#include <wx/timer.h>

class SIMULATOR_FRAME_UI;
class SCH_SYMBOL;

/**
 * Custom widget to handle quick component values modification and simulation on the fly.
 */
class TUNER_SLIDER : public TUNER_SLIDER_BASE
{
public:
    enum class RUN_MODE
    {
        SINGLE,
        MULTI
    };

    TUNER_SLIDER( SIMULATOR_FRAME_UI* aPanel, wxWindow* aParent, const SCH_SHEET_PATH& aSheetPath,
                  SCH_SYMBOL* aSymbol );

    wxString GetSymbolRef() const
    {
        return m_ref;
    }

    RUN_MODE GetRunMode() const
    {
        return m_runMode;
    }

    int GetStepCount() const;

    const SPICE_VALUE& GetMin() const
    {
        return m_min;
    }

    const SPICE_VALUE& GetMax() const
    {
        return m_max;
    }

    const SPICE_VALUE& GetValue() const
    {
        return m_value;
    }

    KIID GetSymbol( SCH_SHEET_PATH* aSheetPath ) const
    {
        *aSheetPath = m_sheetPath;
        return m_symbol;
    }

    bool SetValue( const SPICE_VALUE& aVal );
    bool SetMin( const SPICE_VALUE& aVal );
    bool SetMax( const SPICE_VALUE& aVal );

    void ShowChangedLanguage();

private:
    void updateComponentValue();
    void updateSlider();
    void updateValueText();

    void updateModeControls();

    void updateMax();
    void updateValue();
    void updateMin();

    void onESeries( wxCommandEvent& event ) override;
    void onRunModeChanged( wxCommandEvent& event ) override;
    void onClose( wxCommandEvent& event ) override;
    void onSave( wxCommandEvent& event ) override;
    void onSliderScroll( wxScrollEvent& event ) override;
    void onSliderChanged( wxScrollEvent& event ) override;

    void onMaxKillFocus( wxFocusEvent& event ) override;
    void onValueKillFocus( wxFocusEvent& event ) override;
    void onMinKillFocus( wxFocusEvent& event ) override;

    void onMaxTextEnter( wxCommandEvent& event ) override;
    void onValueTextEnter( wxCommandEvent& event ) override;
    void onMinTextEnter( wxCommandEvent& event ) override;
    void onStepsChanged( wxSpinEvent& event ) override;
    void onStepsTextEnter( wxCommandEvent& event ) override;

private:
    KIID                m_symbol;
    SCH_SHEET_PATH      m_sheetPath;
    wxString            m_ref;

    SPICE_VALUE         m_min;
    SPICE_VALUE         m_max;
    SPICE_VALUE         m_value;

    RUN_MODE            m_runMode;
    SIMULATOR_FRAME_UI* m_frame;
};

#endif /* TUNER_SLIDER_H */
