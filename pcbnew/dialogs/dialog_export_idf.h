/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015  Cirilo Bernardo
 * Copyright (C) 2013-2023 KiCad Developers, see change_log.txt for contributors.
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

#pragma once

#include <dialog_export_idf_base.h>

class PCB_EDIT_FRAME;

class DIALOG_EXPORT_IDF3 : public DIALOG_EXPORT_IDF3_BASE
{
private:
    bool   m_idfThouOpt; // remember last preference for units in THOU
    bool   m_AutoAdjust; // remember last Reference Point AutoAdjust setting
    int    m_RefUnits;   // remember last units for Reference Point
    double m_XRef;       // remember last X Reference Point
    double m_YRef;       // remember last Y Reference Point

    PCB_EDIT_FRAME* m_editFrame;

public:
    DIALOG_EXPORT_IDF3( PCB_EDIT_FRAME* aEditFrame );

    ~DIALOG_EXPORT_IDF3();

    bool GetThouOption() { return m_rbUnitSelection->GetSelection() == 1; }

    wxFilePickerCtrl* FilePicker() { return m_filePickerIDF; }

    int GetRefUnitsChoice() { return m_IDF_RefUnitChoice->GetSelection(); }

    double GetXRef() { return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_IDF_Xref->GetValue() ); }

    double GetYRef() { return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_IDF_Yref->GetValue() ); }

    bool GetNoUnspecifiedOption() { return m_cbRemoveUnspecified->GetValue(); }

    bool GetNoDNPOption() { return m_cbRemoveDNP->GetValue(); }

    bool GetAutoAdjustOffset() { return m_cbAutoAdjustOffset->GetValue(); }

    void OnAutoAdjustOffset( wxCommandEvent& event );

    bool TransferDataFromWindow() override;
};