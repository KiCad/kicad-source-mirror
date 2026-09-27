/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015  Cirilo Bernardo
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

#include <dialog_export_idf_base.h>
#include <widgets/unit_binder.h>

class PCB_EDIT_FRAME;
class JOB_EXPORT_PCB_IDF;

class DIALOG_EXPORT_IDF3 : public DIALOG_EXPORT_IDF3_BASE
{
public:
    DIALOG_EXPORT_IDF3( PCB_EDIT_FRAME* aEditFrame );
    DIALOG_EXPORT_IDF3( JOB_EXPORT_PCB_IDF* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );

    ~DIALOG_EXPORT_IDF3() override = default;

    void ApplyJobSettings( const JOB_EXPORT_PCB_IDF& aSettings );
    void GetJobSettings( JOB_EXPORT_PCB_IDF& aSettingsOut ) const;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    void OnOKButton( wxCommandEvent& event ) override;

private:
    bool checkFilenames();
    void doExport();
    void setupDialog();
    void onRadioButtonsChanged( wxCommandEvent& event );

    void ApplySettings( const IDF_EXPORT_SETTINGS& aSettings );
    IDF_EXPORT_SETTINGS GetSettings() const;

    UNIT_BINDER     m_xPos;
    UNIT_BINDER     m_yPos;

    PCB_EDIT_FRAME*     m_parent = nullptr;
    JOB_EXPORT_PCB_IDF* m_job = nullptr;
};
