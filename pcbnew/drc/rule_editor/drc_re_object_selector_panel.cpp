#include "drc_re_object_selector_panel.h"

#include <board.h>
#include <widgets/net_selector.h>
#include <widgets/netclass_selector.h>
#include <widgets/area_selector.h>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/stc/stc.h>
#include <wx/regex.h>

DRC_RE_OBJECT_SELECTOR_PANEL::DRC_RE_OBJECT_SELECTOR_PANEL( wxWindow* parent, BOARD* board, const wxString& label ) :
        wxPanel( parent ),
        m_customQueryCtrl( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_label = new wxStaticText( this, wxID_ANY, label );
    mainSizer->Add( m_label, 0, wxALL, 5 );

    m_rowSizer = new wxBoxSizer( wxHORIZONTAL );

    wxArrayString choices;
    choices.Add( _( "Any" ) );
    choices.Add( _( "Net" ) );
    choices.Add( _( "Netclass" ) );
    choices.Add( _( "Within Area" ) );
    choices.Add( _( "Custom Query" ) );

    m_choice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices );
    m_choice->SetSelection( 0 );
    m_rowSizer->Add( m_choice, 0, wxALL, 5 );

    m_netSelector = new NET_SELECTOR( this, wxID_ANY );
    m_netSelector->SetNetInfo( &board->GetNetInfo() );
    m_rowSizer->Add( m_netSelector, 1, wxALL | wxEXPAND, 5 );
    m_netSelector->Hide();

    m_netclassSelector = new NETCLASS_SELECTOR( this, wxID_ANY );
    m_netclassSelector->SetBoard( board );
    m_rowSizer->Add( m_netclassSelector, 1, wxALL | wxEXPAND, 5 );
    m_netclassSelector->Hide();

    m_areaSelector = new AREA_SELECTOR( this, wxID_ANY );
    m_areaSelector->SetBoard( board );
    m_rowSizer->Add( m_areaSelector, 1, wxALL | wxEXPAND, 5 );
    m_areaSelector->Hide();

    mainSizer->Add( m_rowSizer, 0, wxEXPAND, 0 );

    SetSizer( mainSizer );

    m_choice->Bind( wxEVT_CHOICE, &DRC_RE_OBJECT_SELECTOR_PANEL::onChoice, this );
}

void DRC_RE_OBJECT_SELECTOR_PANEL::SetLabelText( const wxString& text )
{
    m_label->SetLabelText( text );
}

void DRC_RE_OBJECT_SELECTOR_PANEL::SetCustomQueryCtrl( wxStyledTextCtrl* ctrl )
{
    // Just store the pointer. The control is managed by the parent panel's sizer, not ours.
    // Do not reparent or add to sizer since the control may already be in another sizer.
    m_customQueryCtrl = ctrl;
}

void DRC_RE_OBJECT_SELECTOR_PANEL::onChoice( const wxCommandEvent& aEvent )
{
    m_netSelector->Hide();
    m_netclassSelector->Hide();
    m_areaSelector->Hide();

    if( m_customQueryCtrl )
        m_customQueryCtrl->Hide();

    switch( m_choice->GetSelection() )
    {
    case 1: // Net
        m_netSelector->Show();
        break;
    case 2: // Netclass
        m_netclassSelector->Show();
        break;
    case 3: // Within Area
        m_areaSelector->Show();
        break;
    case 4: // Custom Query
        if( m_customQueryCtrl )
            m_customQueryCtrl->Show();
        break;
    default: break;
    }

    Layout();

    if( m_choiceChangeCallback )
        m_choiceChangeCallback();
}


bool DRC_RE_OBJECT_SELECTOR_PANEL::HasCustomQuerySelected() const
{
    return m_choice->GetSelection() == 4;
}


void DRC_RE_OBJECT_SELECTOR_PANEL::ParseCondition( const wxString& aExpr, const wxString& aPrefix )
{
    wxString expr = aExpr;

    if( expr.IsEmpty() )
    {
        m_choice->SetSelection( 0 );
        onChoice( wxCommandEvent() );

        if( m_customQueryCtrl )
            m_customQueryCtrl->SetValue( wxEmptyString );

        return;
    }

    wxString prefix = aPrefix;
    if( !prefix.IsEmpty() && expr.StartsWith( prefix + "." ) )
        expr = expr.Mid( prefix.Length() + 1 );

    wxRegEx netRe( wxT( "^NetName\\s*==\\s*'([^']*)'" ) );
    wxRegEx netclassRe1( wxT( "^hasNetclass\\('([^']*)'\\)" ) );
    wxRegEx netclassRe2( wxT( "^NetClass\\s*==\\s*'([^']*)'" ) );
    wxRegEx areaRe( wxT( "^(?:enclosedByArea|intersectsArea)\\('([^']*)'\\)" ) );

    if( netRe.Matches( expr ) )
    {
        m_choice->SetSelection( 1 );
        onChoice( wxCommandEvent() );
        m_netSelector->SetSelectedNet( netRe.GetMatch( expr, 1 ) );
        return;
    }
    else if( netclassRe1.Matches( expr ) )
    {
        m_choice->SetSelection( 2 );
        onChoice( wxCommandEvent() );
        m_netclassSelector->SetSelectedNetclass( netclassRe1.GetMatch( expr, 1 ) );
        return;
    }
    else if( netclassRe2.Matches( expr ) )
    {
        m_choice->SetSelection( 2 );
        onChoice( wxCommandEvent() );
        m_netclassSelector->SetSelectedNetclass( netclassRe2.GetMatch( expr, 1 ) );
        return;
    }
    else if( areaRe.Matches( expr ) )
    {
        m_choice->SetSelection( 3 );
        onChoice( wxCommandEvent() );
        m_areaSelector->SetSelectedArea( areaRe.GetMatch( expr, 1 ) );
        return;
    }

    m_choice->SetSelection( 4 );
    onChoice( wxCommandEvent() );
    if( m_customQueryCtrl )
        m_customQueryCtrl->SetValue( expr );
}


wxString DRC_RE_OBJECT_SELECTOR_PANEL::BuildCondition( const wxString& aPrefix ) const
{
    wxString prefix = aPrefix;

    switch( m_choice->GetSelection() )
    {
    case 1: // Net
        if( m_netSelector->GetSelectedNetname().IsEmpty() )
            return wxEmptyString;
        return prefix + wxString::Format( ".NetName == '%s'", m_netSelector->GetSelectedNetname() );

    case 2: // Netclass
        if( m_netclassSelector->GetSelectedNetclass().IsEmpty() )
            return wxEmptyString;
        return prefix + wxString::Format( ".hasNetclass('%s')", m_netclassSelector->GetSelectedNetclass() );

    case 3: // Within Area
        if( m_areaSelector->GetSelectedArea().IsEmpty() )
            return wxEmptyString;
        return prefix + wxString::Format( ".enclosedByArea('%s')", m_areaSelector->GetSelectedArea() );

    case 4: // Custom
        if( m_customQueryCtrl )
            return m_customQueryCtrl->GetText();
        else
            return wxEmptyString;

    default: return wxEmptyString;
    }
}
