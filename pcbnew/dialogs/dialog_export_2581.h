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
#include <optional>
#include <vector>

#include "dialog_export_2581_base.h"
#include "dialog_export_2581_bom.h"
#include <pcb_io/ipc2581/ipc2581_function_mode.h>

class BOARD;
class PCB_EDIT_FRAME;
class PROGRESS_REPORTER;
class REPORTER;
class JOB_EXPORT_PCB_IPC2581;

class DIALOG_EXPORT_2581 : public DIALOG_EXPORT_2581_BASE
{
public:
    DIALOG_EXPORT_2581( PCB_EDIT_FRAME* aParent );
    DIALOG_EXPORT_2581( JOB_EXPORT_PCB_IPC2581* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent );

    // Generate the actual IPC-2581 file; shared between dialog and CLI
    static bool GenerateFile( JOB_EXPORT_PCB_IPC2581& aJob, BOARD* aBoard,
                              PROGRESS_REPORTER* aProgressReporter, REPORTER* aReporter );

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

    IPC2581::MODE GetDataSet() const
    {
        return static_cast<IPC2581::MODE>( m_choiceDataSet->GetSelection() );
    }

    wxString GetNetNamePolicy() const
    {
        return m_choiceNetNames->GetSelection() == 1 ? wxT( "anonymize" ) : wxT( "include" );
    }

    wxString GetRefDesPolicy() const
    {
        return m_choiceRefDes->GetSelection() == 1 ? wxT( "omit" ) : wxT( "include" );
    }

private:
    void onBrowseClicked( wxCommandEvent& event ) override;
    void onCompressCheck( wxCommandEvent& event ) override;
    void onDataSetChange( wxCommandEvent& event ) override;
    void onCustomizeClick( wxCommandEvent& event ) override;
    void onBomFieldsClick( wxCommandEvent& event ) override;
    void onOKClick( wxCommandEvent& event ) override;

    /// Set the includes line and the BOM Fields button from the function mode
    void updateContentSummary();

    IPC2581::SECTION_SET resolvedSections() const;

    static std::vector<std::pair<IPC2581::SECTION, wxString>> sectionLabels();

    /// Keep the dialog selections  A job keeps them on the job
    void saveToProject();

    void init();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCB_EDIT_FRAME*         m_parent;
    JOB_EXPORT_PCB_IPC2581* m_job;
    IPC2581_BOM_FIELDS      m_bomFields;
    /// Set when the user selects the sections  An empty value is then a true selection
    std::optional<wxString> m_sectionKey;
};

#endif // IPC2581_EXPORT_DIALOG_H
