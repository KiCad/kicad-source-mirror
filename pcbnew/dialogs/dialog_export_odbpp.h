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

#pragma once

#include "dialog_export_odbpp_base.h"

class PCB_EDIT_FRAME;
class JOB_EXPORT_PCB_ODB;
class REPORTER;
class PROGRESS_REPORTER;
class BOARD;

class DIALOG_EXPORT_ODBPP : public DIALOG_EXPORT_ODBPP_BASE
{
public:
    DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent );
    DIALOG_EXPORT_ODBPP( JOB_EXPORT_PCB_ODB* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );

    wxString GetOutputPath() const { return m_outputFileName->GetValue(); }

    wxString GetUnitsString() const
    {
        if( m_choiceUnits->GetSelection() == 0 )
            return wxT( "mm" );
        else
            return wxT( "inch" );
    }

    int GetPrecision() const { return m_precision->GetValue(); }

    int GetCompressFormat() const { return m_choiceCompress->GetSelection(); }

    // Runs the actual generation process; shared between GUI and CLI system
    static void GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard,
                                    PCB_EDIT_FRAME* aParentFrame = nullptr,
                                    PROGRESS_REPORTER* aProgressReporter = nullptr,
                                    REPORTER* aErrorReporter = nullptr );

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onFormatChoice( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    void OnFmtChoiceOptionChanged();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    PCB_EDIT_FRAME*     m_parent;
    JOB_EXPORT_PCB_ODB* m_job;
};
