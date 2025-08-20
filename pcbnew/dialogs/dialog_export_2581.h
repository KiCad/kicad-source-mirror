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

#ifndef IPC2581_EXPORT_DIALOG_H
#define IPC2581_EXPORT_DIALOG_H
#include "dialog_export_2581_base.h"

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

    wxString GetOEM() const
    {
        if( m_oemRef->GetSelection() == 0 )
            return wxEmptyString;
        else
            return m_oemRef->GetStringSelection();
    }

    bool GetCompress() const
    {
        return m_cbCompress->GetValue();
    }

    wxString GetMPN() const
    {
        if( !m_choiceMPN->IsEnabled() || m_choiceMPN->GetSelection() == 0 )
            return wxEmptyString;
        else
            return m_choiceMPN->GetStringSelection();
    }

    wxString GetMfg() const
    {
        if( !m_choiceMfg->IsEnabled() || m_choiceMfg->GetSelection() == 0 )
            return wxEmptyString;
        else
            return m_choiceMfg->GetStringSelection();
    }

    wxString GetDistPN() const
    {
        if( !m_choiceDistPN->IsEnabled() || m_choiceDistPN->GetSelection() == 0 )
            return wxEmptyString;
        else
            return m_choiceDistPN->GetStringSelection();
    }

    wxString GetDist() const
    {
        if( !m_textDistributor->IsEnabled() || m_textDistributor->GetValue() == _( "N/A" ) )
            return wxEmptyString;
        else
            return m_textDistributor->GetValue();
    }

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onCompressCheck( wxCommandEvent& event ) override;
    void onMfgPNChange( wxCommandEvent& event ) override;
    void onDistPNChange( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    void init();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME* m_parent;
    JOB_EXPORT_PCB_IPC2581* m_job;
};

#endif // IPC2581_EXPORT_DIALOG_H