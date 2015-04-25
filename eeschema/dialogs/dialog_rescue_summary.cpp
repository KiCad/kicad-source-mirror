/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 Kicad Developers, see change_log.txt for contributors.
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

#include <schframe.h>
#include <invoke_sch_dialog.h>
#include <dialog_rescue_summary_base.h>
#include <kiface_i.h>
#include <lib_cache_rescue.h>
#include <sch_component.h>
#include <vector>
#include <boost/foreach.hpp>

class DIALOG_RESCUE_SUMMARY: public DIALOG_RESCUE_SUMMARY_BASE
{
public:
    DIALOG_RESCUE_SUMMARY( SCH_EDIT_FRAME* aParent, std::vector<RESCUE_LOG>& aRescueLog );

private:
    SCH_EDIT_FRAME* m_Parent;
    wxConfigBase*   m_Config;
    std::vector<RESCUE_LOG>* m_RescueLog;

    bool TransferDataToWindow();
    void OnOkClick( wxCommandEvent& event );
};


DIALOG_RESCUE_SUMMARY::DIALOG_RESCUE_SUMMARY( SCH_EDIT_FRAME* aParent, std::vector<RESCUE_LOG>& aRescueLog )
    : DIALOG_RESCUE_SUMMARY_BASE( aParent ), m_Parent( aParent), m_RescueLog( &aRescueLog )
{ }


bool DIALOG_RESCUE_SUMMARY::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_Config = Kiface().KifaceSettings();
    m_ListOfChanges->AppendTextColumn( wxT( "Reference" ) );
    m_ListOfChanges->AppendTextColumn( wxT( "Old Symbol" ), wxDATAVIEW_CELL_INERT, /*width*/ 100);
    m_ListOfChanges->AppendTextColumn( wxT( "New Symbol" ), wxDATAVIEW_CELL_INERT, /*width*/ 100);

    wxVector<wxVariant> data;
    BOOST_FOREACH( RESCUE_LOG& each_log_item, *m_RescueLog )
    {
        data.clear();
        data.push_back( each_log_item.component->GetRef( & m_Parent->GetCurrentSheet() ) );
        data.push_back( each_log_item.old_name );
        data.push_back( each_log_item.new_name );
        m_ListOfChanges->AppendItem( data );
    }

    GetSizer()->Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();

    return true;
}


int InvokeDialogRescueSummary( SCH_EDIT_FRAME* aCaller, std::vector<RESCUE_LOG>& aRescueLog )
{
    DIALOG_RESCUE_SUMMARY dlg( aCaller, aRescueLog );
    return dlg.ShowModal();
}
