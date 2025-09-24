/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <backannotate.h>
#include <dialog_update_from_pcb.h>
#include <sch_edit_frame.h>
#include <sch_editor_control.h>
#include "widgets/wx_html_report_panel.h"


DIALOG_UPDATE_FROM_PCB::DIALOG_UPDATE_FROM_PCB( SCH_EDIT_FRAME* aParent ) :
        DIALOG_UPDATE_FROM_PCB_BASE( aParent ),
        m_frame( aParent )
{
    m_messagePanel->SetLabel( _( "Changes to Be Applied" ) );
    m_messagePanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    m_messagePanel->SetLazyUpdate( true );
    m_messagePanel->GetSizer()->SetSizeHints( this );

    if( m_cbRelinkFootprints->GetValue() )
    {
        m_cbUpdateReferences->SetValue( false );
        m_cbUpdateReferences->Enable( false );
    }
    else
    {
        m_cbUpdateReferences->Enable( true );
    }

    SetupStandardButtons( { { wxID_OK,     _( "Update Schematic" ) },
                            { wxID_CANCEL, _( "Close" )            } } );

    finishDialogSettings();
}


void DIALOG_UPDATE_FROM_PCB::updateData()
{
    bool successfulRun = false;
    m_messagePanel->Clear();
    BACK_ANNOTATE backAnno( m_frame,
                            m_messagePanel->Reporter(),
                            m_cbRelinkFootprints->GetValue(),
                            m_cbUpdateFootprints->GetValue(),
                            m_cbUpdateValues->GetValue(),
                            m_cbUpdateReferences->GetValue(),
                            m_cbUpdateNetNames->GetValue(),
                            m_cbUpdateAttributes->GetValue(),
                            m_cbUpdateOtherFields->GetValue(),
                            m_cbPreferUnitSwaps->GetValue(),
                            m_cbPreferPinSwaps->GetValue(),
                            true );
    std::string   netlist;

    if( backAnno.FetchNetlistFromPCB( netlist ) )
        successfulRun = backAnno.BackAnnotateSymbols( netlist );

    m_sdbSizerOK->Enable( successfulRun );
    m_messagePanel->Flush( false );
}


bool DIALOG_UPDATE_FROM_PCB::TransferDataToWindow()
{
    updateData();
    return true;
}


void DIALOG_UPDATE_FROM_PCB::OnOptionChanged( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_cbRelinkFootprints )
    {
        if( m_cbRelinkFootprints->GetValue() )
        {
            m_cbUpdateReferences->SetValue( false );
            m_cbUpdateReferences->Enable( false );
        }
        else
        {
            m_cbUpdateReferences->Enable( true );
        }
    }

    updateData();
}


void DIALOG_UPDATE_FROM_PCB::OnUpdateClick( wxCommandEvent& event )
{
    std::string netlist;
    m_messagePanel->Clear();
    BACK_ANNOTATE backAnno( m_frame,
                            m_messagePanel->Reporter(),
                            m_cbRelinkFootprints->GetValue(),
                            m_cbUpdateFootprints->GetValue(),
                            m_cbUpdateValues->GetValue(),
                            m_cbUpdateReferences->GetValue(),
                            m_cbUpdateNetNames->GetValue(),
                            m_cbUpdateAttributes->GetValue(),
                            m_cbUpdateOtherFields->GetValue(),
                            m_cbPreferUnitSwaps->GetValue(),
                            m_cbPreferPinSwaps->GetValue(),
                            false );

    if( backAnno.FetchNetlistFromPCB( netlist ) && backAnno.BackAnnotateSymbols( netlist ) )
    {
        m_frame->GetCurrentSheet().UpdateAllScreenReferences();
        m_frame->SetSheetNumberAndCount();
        m_frame->SyncView();
        m_frame->OnModify();
        m_frame->GetCanvas()->Refresh();

        if( m_cbRelinkFootprints->GetValue() )
            backAnno.PushNewLinksToPCB();

        m_sdbSizerCancel->SetDefault();
        m_sdbSizerOK->Enable( false );
    }

    m_messagePanel->Flush( false );
}
