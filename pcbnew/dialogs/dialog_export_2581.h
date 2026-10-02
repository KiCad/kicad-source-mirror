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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IPC2581_EXPORT_DIALOG_H
#define IPC2581_EXPORT_DIALOG_H
#include "dialog_export_2581_base.h"
#include "panel_fab_export_content.h"

class PCB_EDIT_FRAME;
class JOB_EXPORT_PCB_IPC2581;

class DIALOG_EXPORT_2581 : public DIALOG_EXPORT_2581_BASE
{
public:
    DIALOG_EXPORT_2581( PCB_EDIT_FRAME* aParent );
    DIALOG_EXPORT_2581( JOB_EXPORT_PCB_IPC2581* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );

    wxString GetOutputPath() const
    {
        return m_outputFileName->GetValue();
    }

    wxString GetUnitsString() const
    {
        if( m_choiceUnits->GetSelection() == 0 )
            return wxT( "mm" );
        else
            return wxT( "inch" );
    }

    wxString GetPrecision() const
    {
        return wxString::Format( "%d", m_precision->GetValue() );
    }

    char GetVersion() const
    {
        return m_versionChoice->GetSelection() == 0 ? 'B' : 'C';
    }

    bool GetCompress() const
    {
        return m_cbCompress->GetValue();
    }

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onCompressCheck( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    /// Keep the dialog selections  A job keeps them on the job
    void saveToProject();

    void init();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME*         m_parent;
    JOB_EXPORT_PCB_IPC2581* m_job;
};

#endif // IPC2581_EXPORT_DIALOG_H
