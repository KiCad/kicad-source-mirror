/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_edit_label.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     18/12/2008 15:46:26
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "wx/valgen.h"

#include "common.h"
#include "class_drawpanel.h"
#include "program.h"
#include "general.h"
#include "dialog_edit_label.h"


int DialogLabelEditor::ShowModally(  WinEDA_SchematicFrame* parent, SCH_TEXT * CurrentText )
{
    int ret;

    DialogLabelEditor* dialog = new DialogLabelEditor( parent, CurrentText );

    // doing any post construction resizing is better done here than in
    // OnInitDialog() since it tends to flash/redraw the dialog less.
    dialog->init();

    ret = dialog->ShowModal();
    dialog->Destroy();
    return ret;
}



DialogLabelEditor::DialogLabelEditor( WinEDA_SchematicFrame* parent, SCH_TEXT* CurrentText ) :
    DialogLabelEditor_Base( parent )
{
    m_Parent = parent;
    m_CurrentText = CurrentText;
    init();
}


void DialogLabelEditor::init()
{
    wxString msg;
    
    if( m_CurrentText->m_MultilineAllowed )
    {
        m_TextLabel = m_textCtrlMultiline;
        m_TextLabelSingleline->Show(false);
    }
    else
    {
        m_TextLabel = m_TextLabelSingleline;
       m_textCtrlMultiline->Show(false);
    }

    m_TextLabel->SetValue( m_CurrentText->m_Text );
    m_TextLabel->SetFocus();

    switch( m_CurrentText->Type() )
    {
    case TYPE_SCH_GLOBALLABEL:
        SetTitle( _( "Global Label Properties" ) );
        break;

    case TYPE_SCH_HIERLABEL:
        SetTitle( _( "Hierarchal Label Properties" ) );
        m_TextShape->SetLabel( _("Hlabel Shape") );
        break;

    case TYPE_SCH_LABEL:
        SetTitle( _( "Label Properties" ) );
        break;

    default:
        SetTitle( _( "Text Properties" ) );
      	m_TextLabel->Disconnect(wxEVT_COMMAND_TEXT_ENTER , wxCommandEventHandler ( DialogLabelEditor::onEnterKey ), NULL, this);
        break;
    }

    unsigned MINTEXTWIDTH  = 30;    // M's are big characters, a few establish a lot of width

    if( m_CurrentText->m_Text.Length() < MINTEXTWIDTH )
    {
        wxString textWidth;
        textWidth.Append( 'M', MINTEXTWIDTH );
        EnsureTextCtrlWidth( m_TextLabel, &textWidth );
    }
    else
        EnsureTextCtrlWidth( m_TextLabel );

    // Set validators
    m_TextOrient->SetSelection( m_CurrentText->GetSchematicTextOrientation() );
    m_TextShape->SetSelection( m_CurrentText->m_Shape );

    int style = 0;
    if( m_CurrentText->m_Italic )
        style = 1;
    if( m_CurrentText->m_Bold )
        style += 2;

    m_TextStyle->SetSelection( style );

    msg = m_SizeTitle->GetLabel() + ReturnUnitSymbol();
    m_SizeTitle->SetLabel( msg );

    msg = ReturnStringFromValue( g_UnitMetric, m_CurrentText->m_Size.x, m_Parent->m_InternalUnits );
    m_TextSize->SetValue( msg );

    if( m_CurrentText->Type() != TYPE_SCH_GLOBALLABEL
     && m_CurrentText->Type() != TYPE_SCH_HIERLABEL )
    {
        m_TextShape->Show( false );
    }

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}

/*!
 * wxTE_PROCESS_ENTER  event handler for m_TextLabel
 */

void DialogLabelEditor::onEnterKey( wxCommandEvent& event )
{
    TextPropertiesAccept( event );
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DialogLabelEditor::OnButtonOKClick( wxCommandEvent& event )
{
    TextPropertiesAccept( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DialogLabelEditor::OnButtonCANCEL_Click( wxCommandEvent& event )
{
    m_Parent->DrawPanel->MouseToCursorSchema();
    EndModal( -1 );
}

