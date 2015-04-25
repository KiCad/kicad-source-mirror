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
#include <sch_component.h>
#include <invoke_sch_dialog.h>
#include <dialog_rescue_each_base.h>
#include <kiface_i.h>
#include <class_library.h>
#include <class_libentry.h>
#include <set>
#include <vector>
#include <boost/foreach.hpp>
#include <lib_cache_rescue.h>

class DIALOG_RESCUE_EACH: public DIALOG_RESCUE_EACH_BASE
{
public:
    /**
     * Constructor
     * This dialog asks the user which rescuable, cached parts he wants to rescue.
     * Any rejects will be pruned from aCandidates.
     * @param aCaller - the SCH_EDIT_FRAME calling this
     * @param aCandidates - the list of RESCUE_CANDIDATES
     * @param aComponents - a vector of all the components in the schematic
     * @param aAskShowAgain - if true, a "Never Show Again" button will be included
     */
    DIALOG_RESCUE_EACH( SCH_EDIT_FRAME* aParent, std::vector<RESCUE_CANDIDATE>& aCandidates,
            std::vector<SCH_COMPONENT*>& aComponents, bool aAskShowAgain );

    ~DIALOG_RESCUE_EACH();

private:
    SCH_EDIT_FRAME* m_Parent;
    wxConfigBase*   m_Config;
    std::vector<RESCUE_CANDIDATE>* m_Candidates;
    std::vector<SCH_COMPONENT*>* m_Components;
    bool            m_AskShowAgain;

    bool m_insideUpdateEvent;

    bool TransferDataToWindow();
    bool TransferDataFromWindow();
    void PopulateConflictList();
    void PopulateInstanceList();
    void OnConflictSelect( wxDataViewEvent& event );
    void OnNeverShowClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnHandleCachePreviewRepaint( wxPaintEvent& aRepaintEvent );
    void OnHandleLibraryPreviewRepaint( wxPaintEvent& aRepaintEvent );
    void OnDialogResize( wxSizeEvent& aSizeEvent );
    void renderPreview( LIB_PART* aComponent, int aUnit, wxPanel* panel );
};


DIALOG_RESCUE_EACH::DIALOG_RESCUE_EACH( SCH_EDIT_FRAME* aParent, std::vector<RESCUE_CANDIDATE>& aCandidates,
        std::vector<SCH_COMPONENT*>& aComponents, bool aAskShowAgain )

    : DIALOG_RESCUE_EACH_BASE( aParent ),
      m_Parent( aParent ),
      m_Candidates( &aCandidates ),
      m_Components( &aComponents ),
      m_AskShowAgain( aAskShowAgain ),
      m_insideUpdateEvent( false )
{
}


DIALOG_RESCUE_EACH::~DIALOG_RESCUE_EACH()
{
}


bool DIALOG_RESCUE_EACH::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_Config = Kiface().KifaceSettings();
    m_ListOfConflicts->AppendToggleColumn( wxT("Rescue") );
    m_ListOfConflicts->AppendTextColumn( wxT("Symbol Name") );
    m_ListOfInstances->AppendTextColumn( wxT("Reference") );
    m_ListOfInstances->AppendTextColumn( wxT("Value") );
    PopulateConflictList();
    PopulateInstanceList();

    if( !m_AskShowAgain )
        m_btnNeverShowAgain->Hide();

    GetSizer()->Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();

    return true;
}


void DIALOG_RESCUE_EACH::PopulateConflictList()
{
    wxVector<wxVariant> data;
    BOOST_FOREACH( RESCUE_CANDIDATE& each_candidate, *m_Candidates )
    {
        data.clear();
        data.push_back( wxVariant( true ) );
        data.push_back( each_candidate.requested_name );
        m_ListOfConflicts->AppendItem( data );
    }
}


void DIALOG_RESCUE_EACH::PopulateInstanceList()
{
    m_ListOfInstances->DeleteAllItems();

    int row = m_ListOfConflicts->GetSelectedRow();
    if( row == wxNOT_FOUND )
        row = 0;

    RESCUE_CANDIDATE& selected_part = (*m_Candidates)[row];

    wxVector<wxVariant> data;
    BOOST_FOREACH( SCH_COMPONENT* each_component, *m_Components )
    {
        if( each_component->GetPartName() != selected_part.requested_name )
            continue;

        SCH_FIELD* valueField = each_component->GetField( 1 );

        data.clear();
        data.push_back( each_component->GetRef( & m_Parent->GetCurrentSheet() ) );
        data.push_back( valueField ? valueField->GetText() : wxT("") );
        m_ListOfInstances->AppendItem( data );

    }
}


void DIALOG_RESCUE_EACH::OnHandleCachePreviewRepaint( wxPaintEvent& aRepaintEvent )
{
    int row = m_ListOfConflicts->GetSelectedRow();
    if( row == wxNOT_FOUND )
        row = 0;

    RESCUE_CANDIDATE& selected_part = (*m_Candidates)[row];

    renderPreview( selected_part.cache_candidate, 0, m_componentViewOld );
}


void DIALOG_RESCUE_EACH::OnHandleLibraryPreviewRepaint( wxPaintEvent& aRepaintEvent )
{
    int row = m_ListOfConflicts->GetSelectedRow();
    if( row == wxNOT_FOUND )
        row = 0;

    RESCUE_CANDIDATE& selected_part = (*m_Candidates)[row];

    renderPreview( selected_part.lib_candidate, 0, m_componentViewNew );
}


void DIALOG_RESCUE_EACH::OnDialogResize( wxSizeEvent& aSizeEvent )
{
    // Placeholer - I was previously doing some extra reflow here.
    DIALOG_RESCUE_EACH_BASE::OnDialogResize( aSizeEvent );
}


// Render the preview in our m_componentView. If this gets more complicated, we should
// probably have a derived class from wxPanel; but this keeps things local.
void DIALOG_RESCUE_EACH::renderPreview( LIB_PART* aComponent, int aUnit, wxPanel* aPanel )
{
    wxPaintDC dc( aPanel );
    EDA_COLOR_T bgcolor = m_Parent->GetDrawBgColor();

    dc.SetBackground( bgcolor == BLACK ? *wxBLACK_BRUSH : *wxWHITE_BRUSH );
    dc.Clear();

    if( aComponent == NULL )
        return;

    if( aUnit <= 0 )
        aUnit = 1;

    const wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Find joint bounding box for everything we are about to draw.
    EDA_RECT bBox = aComponent->GetBoundingBox( aUnit, /* deMorganConvert */ 0 );
    const double xscale = (double) dc_size.x / bBox.GetWidth();
    const double yscale = (double) dc_size.y / bBox.GetHeight();
    const double scale  = std::min( xscale, yscale ) * 0.85;

    dc.SetUserScale( scale, scale );

    wxPoint offset =  bBox.Centre();
    NEGATE( offset.x );
    NEGATE( offset.y );

    // Avoid rendering when either dimension is zero
    int width, height;

    dc.GetSize( &width, &height );
    if( !width || !height )
        return;

    aComponent->Draw( NULL, &dc, offset, aUnit, /* deMorganConvert */ 0, GR_COPY,
                      UNSPECIFIED_COLOR, DefaultTransform, true, true, false );
}


void DIALOG_RESCUE_EACH::OnConflictSelect( wxDataViewEvent& aEvent )
{
    // wxformbuilder connects this event to the _dialog_, not the data view.
    // Make sure the correct item triggered it, otherwise we trigger recursively
    // and get a stack overflow.
    if( aEvent.GetEventObject() != m_ListOfConflicts ) return;

    PopulateInstanceList();

    int row = m_ListOfConflicts->GetSelectedRow();
    if( row == wxNOT_FOUND )
        row = 0;

    RESCUE_CANDIDATE& selected_part = (*m_Candidates)[row];

    renderPreview( selected_part.cache_candidate, 0, m_componentViewOld );
    renderPreview( selected_part.lib_candidate, 0, m_componentViewNew );
}


bool DIALOG_RESCUE_EACH::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::vector<RESCUE_CANDIDATE>::iterator it = m_Candidates->begin();
    for( size_t index = 0; it != m_Candidates->end(); ++index )
    {
        wxVariant val;
        m_ListOfConflicts->GetValue( val, index, 0 );
        bool rescue_part = val.GetBool();

        if( !rescue_part )
            m_Candidates->erase( it );
        else
            ++it;
    }

    return true;
}


void DIALOG_RESCUE_EACH::OnNeverShowClick( wxCommandEvent& aEvent )
{
    wxMessageDialog dlg( m_Parent, wxT( "Stop showing this tool? No changes will be made.\n\n"
                "This setting can be changed from the Component Libraries settings, and the "
                "tool can be activated manually from the Tools menu." ),
            wxT( "Rescue Components" ), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
    int resp = dlg.ShowModal ();

    if( resp == wxID_YES )
    {
        m_Config->Write( wxT("RescueNeverShow"), true );
        m_Candidates->clear();
        Close();
    }
}


void DIALOG_RESCUE_EACH::OnCancelClick( wxCommandEvent& aEvent )
{
    m_Candidates->clear();
    DIALOG_RESCUE_EACH_BASE::OnCancelClick( aEvent );
}


int InvokeDialogRescueEach( SCH_EDIT_FRAME* aCaller, std::vector<RESCUE_CANDIDATE>& aCandidates,
        std::vector<SCH_COMPONENT*>& aComponents, bool aAskShowAgain )
{
    DIALOG_RESCUE_EACH dlg( aCaller, aCandidates, aComponents, aAskShowAgain );
    return dlg.ShowModal();
}
