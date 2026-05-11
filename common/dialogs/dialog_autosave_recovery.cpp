/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialogs/dialog_autosave_recovery.h>

#include <wx/datetime.h>
#include <wx/filename.h>


static wxString formatStamp( const wxDateTime& aDt )
{
    if( !aDt.IsValid() )
        return _( "(missing)" );

    return aDt.Format( wxS( "%Y-%m-%d %H:%M:%S" ) );
}


DIALOG_AUTOSAVE_RECOVERY::DIALOG_AUTOSAVE_RECOVERY( wxWindow*                                         aParent,
                                                    const std::vector<std::pair<wxString, wxString>>& aStale ) :
        DIALOG_AUTOSAVE_RECOVERY_BASE( aParent ),
        m_choice( AUTOSAVE_RECOVERY_CHOICE::CANCEL ),
        m_stale( aStale )
{
    m_fileList->AppendColumn( _( "File" ), wxLIST_FORMAT_LEFT, 260 );
    m_fileList->AppendColumn( _( "Saved version" ), wxLIST_FORMAT_LEFT, 150 );
    m_fileList->AppendColumn( _( "Auto-saved copy" ), wxLIST_FORMAT_LEFT, 150 );

    m_fileList->EnableCheckBoxes( true );

    for( const auto& [autosavePath, srcPath] : m_stale )
    {
        wxFileName srcFn( srcPath );
        wxFileName autosaveFn( autosavePath );

        wxDateTime srcTime;
        wxDateTime autosaveTime;

        if( srcFn.FileExists() )
            srcTime = srcFn.GetModificationTime();

        if( autosaveFn.FileExists() )
            autosaveTime = autosaveFn.GetModificationTime();

        long row = m_fileList->InsertItem( m_fileList->GetItemCount(), srcFn.GetFullName() );
        m_fileList->SetItem( row, 1, formatStamp( srcTime ) );
        m_fileList->SetItem( row, 2, formatStamp( autosaveTime ) );
        m_fileList->CheckItem( row, true );
    }

    m_fileList->Bind( wxEVT_LIST_ITEM_CHECKED, &DIALOG_AUTOSAVE_RECOVERY::onItemCheckChanged, this );
    m_fileList->Bind( wxEVT_LIST_ITEM_UNCHECKED, &DIALOG_AUTOSAVE_RECOVERY::onItemCheckChanged, this );

    updateActionButtons();
}


std::vector<std::pair<wxString, wxString>> DIALOG_AUTOSAVE_RECOVERY::GetSelectedStale() const
{
    std::vector<std::pair<wxString, wxString>> selected;

    for( long i = 0; i < m_fileList->GetItemCount(); ++i )
    {
        if( m_fileList->IsItemChecked( i ) )
            selected.push_back( m_stale[static_cast<size_t>( i )] );
    }

    return selected;
}


void DIALOG_AUTOSAVE_RECOVERY::onItemCheckChanged( wxListEvent& aEvent )
{
    updateActionButtons();
    aEvent.Skip();
}


void DIALOG_AUTOSAVE_RECOVERY::updateActionButtons()
{
    bool anyChecked = false;

    for( long i = 0; i < m_fileList->GetItemCount(); ++i )
    {
        if( m_fileList->IsItemChecked( i ) )
        {
            anyChecked = true;
            break;
        }
    }

    m_btnRestore->Enable( anyChecked );
    m_btnKeepCurrent->Enable( anyChecked );
    m_btnKeepBoth->Enable( anyChecked );
}


void DIALOG_AUTOSAVE_RECOVERY::OnRestore( wxCommandEvent& aEvent )
{
    m_choice = AUTOSAVE_RECOVERY_CHOICE::RESTORE;
    EndModal( wxID_OK );
}


void DIALOG_AUTOSAVE_RECOVERY::OnKeepCurrent( wxCommandEvent& aEvent )
{
    m_choice = AUTOSAVE_RECOVERY_CHOICE::KEEP_CURRENT;
    EndModal( wxID_OK );
}


void DIALOG_AUTOSAVE_RECOVERY::OnKeepBoth( wxCommandEvent& aEvent )
{
    m_choice = AUTOSAVE_RECOVERY_CHOICE::KEEP_BOTH;
    EndModal( wxID_OK );
}


void DIALOG_AUTOSAVE_RECOVERY::OnCancel( wxCommandEvent& aEvent )
{
    m_choice = AUTOSAVE_RECOVERY_CHOICE::CANCEL;
    EndModal( wxID_CANCEL );
}
