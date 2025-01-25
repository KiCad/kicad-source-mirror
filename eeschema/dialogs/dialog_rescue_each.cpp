/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <class_draw_panel_gal.h>
#include <libraries/legacy_symbol_library.h>
#include <dialog_rescue_each_base.h>
#include <eeschema_settings.h>
#include <invoke_sch_dialog.h>
#include <kiface_base.h>
#include <project_rescue.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <set>
#include <symbol_preview_widget.h>

#include <wx/msgdlg.h>
#include <wx/dcclient.h>


class DIALOG_RESCUE_EACH: public DIALOG_RESCUE_EACH_BASE
{
public:
    /**
     * This dialog asks the user which rescuable, cached parts he wants to rescue.
     *
     * Any rejects will be pruned from aCandidates.
     *
     * @param aParent - the SCH_EDIT_FRAME calling this
     * @param aRescuer - the active RESCUER instance
     * @param aCurrentSheet the current sheet in the schematic editor frame
     * @param aGalBackEndType the current GAL type used to render symbols
     * @param aAskShowAgain - if true, a "Never Show Again" button will be included
     */
    DIALOG_RESCUE_EACH( wxWindow* aParent, RESCUER& aRescuer, SCH_SHEET_PATH* aCurrentSheet,
                        EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType, bool aAskShowAgain );
    ~DIALOG_RESCUE_EACH() = default;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void PopulateConflictList();
    void PopulateInstanceList();
    void OnConflictSelect( wxDataViewEvent& aEvent ) override;
    void OnNeverShowClick( wxCommandEvent& aEvent ) override;
    void OnCancelClick( wxCommandEvent& aEvent ) override;

    // Display the 2 items (old in cache and new in library) corresponding to the
    // selected conflict in m_ListOfConflicts
    void displayItemsInConflict();

private:
    SYMBOL_PREVIEW_WIDGET* m_previewNewWidget;
    SYMBOL_PREVIEW_WIDGET* m_previewOldWidget;
    RESCUER*               m_rescuer;
    SCH_SHEET_PATH*        m_currentSheet;
    bool                   m_askShowAgain;
};


DIALOG_RESCUE_EACH::DIALOG_RESCUE_EACH( wxWindow* aParent, RESCUER& aRescuer, SCH_SHEET_PATH* aCurrentSheet,
                                        EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType, bool aAskShowAgain ) :
        DIALOG_RESCUE_EACH_BASE( aParent ),
        m_rescuer( &aRescuer ),
        m_currentSheet( aCurrentSheet ),
        m_askShowAgain( aAskShowAgain )
{
    wxASSERT( aCurrentSheet );

    m_previewOldWidget = new SYMBOL_PREVIEW_WIDGET( m_previewOldPanel, &Kiway(), false, aGalBackEndType );
	m_SizerOldPanel->Add( m_previewOldWidget, 1, wxEXPAND | wxALL, 5 );

    m_previewNewWidget = new SYMBOL_PREVIEW_WIDGET( m_previewNewPanel,  &Kiway(), false, aGalBackEndType );
	m_SizerNewPanel->Add( m_previewNewWidget, 1, wxEXPAND | wxALL, 5 );

    // Set the info message, customized to include the proper suffix.
    wxString info = _( "This schematic was made using older symbol libraries which may break the "
                       "schematic.  Some symbols may need to be linked to a different symbol name.  "
                       "Some symbols may need to be \"rescued\" (copied and renamed) into a new library.\n\n"
                       "The following changes are recommended to update the project." );
    m_htmlPrompt->AppendToPage( info );

    // wxDataViewListCtrl seems to do a poor job of laying itself out so help it along here.
    wxString header = _( "Accept" );
    wxFont font = m_ListOfConflicts->GetFont();

    font.MakeBold();

    wxClientDC dc( this );

    dc.SetFont( font );

    int width = dc.GetTextExtent( header ).GetWidth() * 1.25;

    m_ListOfConflicts->AppendToggleColumn( header, wxDATAVIEW_CELL_ACTIVATABLE, width, wxALIGN_CENTER );

    header = _( "Symbol Name" );
    width = dc.GetTextExtent( header ).GetWidth() * 2;
    m_ListOfConflicts->AppendTextColumn( header, wxDATAVIEW_CELL_INERT, width );

    header = _( "Action Taken" );
    width = dc.GetTextExtent( header ).GetWidth() * 10;
    m_ListOfConflicts->AppendTextColumn( header, wxDATAVIEW_CELL_INERT, width );

    header = _( "Reference" );
    width = dc.GetTextExtent( header ).GetWidth() * 2;
    m_ListOfInstances->AppendTextColumn( header, wxDATAVIEW_CELL_INERT, width );

    header = _( "Value" );
    width = dc.GetTextExtent( header ).GetWidth() * 10;
    m_ListOfInstances->AppendTextColumn( header, wxDATAVIEW_CELL_INERT, width );

    m_previewOldWidget->SetLayoutDirection( wxLayout_LeftToRight );
    m_previewNewWidget->SetLayoutDirection( wxLayout_LeftToRight );

    // Make sure the HTML window is large enough. Some fun size juggling and
    // fudge factors here but it does seem to work pretty reliably.
    wxSize info_size = m_htmlPrompt->GetTextExtent( info );
    wxSize prompt_size = m_htmlPrompt->GetSize();
    wxSize font_size = m_htmlPrompt->GetTextExtent( "X" );
    int    approx_info_height = ( 2 * info_size.x / prompt_size.x ) * font_size.y;
    m_htmlPrompt->SetSizeHints( 2 * prompt_size.x / 3, approx_info_height );

    SetupStandardButtons();
    m_stdButtonsOK->SetLabel( _( "Rescue Symbols" ) );
    m_stdButtonsCancel->SetLabel( _( "Skip Symbol Rescue" ) );
	m_stdButtons->Layout();
    GetSizer()->SetSizeHints( this );

    Layout();
    Center();
}


bool DIALOG_RESCUE_EACH::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    PopulateConflictList();
    PopulateInstanceList();

    if( !m_askShowAgain )
        m_btnNeverShowAgain->Hide();

    return true;
}


void DIALOG_RESCUE_EACH::PopulateConflictList()
{
    wxVector<wxVariant> data;

    for( RESCUE_CANDIDATE& each_candidate : m_rescuer->m_all_candidates )
    {
        data.clear();
        data.push_back( wxVariant( true ) );
        data.push_back( each_candidate.GetRequestedName() );
        data.push_back( each_candidate.GetActionDescription() );

        m_ListOfConflicts->AppendItem( data );
    }

    if( !m_rescuer->m_all_candidates.empty() )
    {
        // Select the first choice
        m_ListOfConflicts->SelectRow( 0 );

        // Ensure this choice is displayed:
        displayItemsInConflict();
    }
}


void DIALOG_RESCUE_EACH::PopulateInstanceList()
{
    m_ListOfInstances->DeleteAllItems();

    int row = m_ListOfConflicts->GetSelectedRow();

    if( row == wxNOT_FOUND )
        row = 0;

    RESCUE_CANDIDATE& selected_part = m_rescuer->m_all_candidates[row];

    wxVector<wxVariant> data;
    int count = 0;

    for( SCH_SYMBOL* eachSymbol : *m_rescuer->GetSymbols() )
    {
        if( eachSymbol->GetLibId().Format() != UTF8( selected_part.GetRequestedName() ) )
            continue;

        SCH_FIELD* valueField = eachSymbol->GetField( FIELD_T::VALUE );

        data.clear();
        data.push_back( eachSymbol->GetRef( m_currentSheet ) );
        data.push_back( valueField ? valueField->GetText() : wxString( wxT( "" ) ) );
        m_ListOfInstances->AppendItem( data );
        count++;
    }

    wxString msg = wxString::Format( _( "Instances of this symbol (%d items):" ), count );
    m_titleInstances->SetLabelText( msg );
}


void DIALOG_RESCUE_EACH::displayItemsInConflict()
{
    int row = m_ListOfConflicts->GetSelectedRow();

    if( row < 0 )
    {
        m_previewOldWidget->DisplayPart( nullptr, 0, 0 );
        m_previewNewWidget->DisplayPart( nullptr, 0, 0 );
    }
    else
    {
        RESCUE_CANDIDATE& selected_part = m_rescuer->m_all_candidates[row];

        m_previewOldWidget->DisplayPart( selected_part.GetCacheCandidate(), selected_part.GetUnit(),
                                         selected_part.GetBodyStyle() );
        m_previewNewWidget->DisplayPart( selected_part.GetLibCandidate(), selected_part.GetUnit(),
                                         selected_part.GetBodyStyle() );
    }
}


void DIALOG_RESCUE_EACH::OnConflictSelect( wxDataViewEvent& aEvent )
{
    // wxformbuilder connects this event to the _dialog_, not the data view.
    // Make sure the correct item triggered it, otherwise we trigger recursively
    // and get a stack overflow.
    if( aEvent.GetEventObject() != m_ListOfConflicts )
        return;

    PopulateInstanceList();
    displayItemsInConflict();
}


bool DIALOG_RESCUE_EACH::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    for( size_t index = 0; index < m_rescuer->GetCandidateCount(); ++index )
    {
        wxVariant val;
        m_ListOfConflicts->GetValue( val, index, 0 );
        bool rescue_part = val.GetBool();

        if( rescue_part )
            m_rescuer->m_chosen_candidates.push_back( &m_rescuer->m_all_candidates[index] );
    }

    return true;
}


void DIALOG_RESCUE_EACH::OnNeverShowClick( wxCommandEvent& aEvent )
{
    wxMessageDialog dlg( GetParent(),
                         _(  "Stop showing this tool?\n"
                             "No changes will be made.\n\n"
                             "This setting can be changed from the \"Preferences\" dialog,\n"
                             "and the tool can be activated manually from the \"Tools\" menu." ),
                         _( "Rescue Symbols" ), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
    int resp = dlg.ShowModal ();

    if( resp == wxID_YES )
    {
        auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

        if( cfg )
            cfg->m_RescueNeverShow = true;

        m_rescuer->m_chosen_candidates.clear();
        Close();
    }
}


void DIALOG_RESCUE_EACH::OnCancelClick( wxCommandEvent& aEvent )
{
    m_rescuer->m_chosen_candidates.clear();
    DIALOG_RESCUE_EACH_BASE::OnCancelClick( aEvent );
}


int InvokeDialogRescueEach( wxWindow* aParent, RESCUER& aRescuer, SCH_SHEET_PATH* aCurrentSheet,
                            EDA_DRAW_PANEL_GAL::GAL_TYPE aGalBackEndType, bool aAskShowAgain )
{
    DIALOG_RESCUE_EACH dlg( aParent, aRescuer, aCurrentSheet, aGalBackEndType, aAskShowAgain );
    return dlg.ShowQuasiModal();
}
