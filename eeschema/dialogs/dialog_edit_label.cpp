/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_label.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     18/12/2008 15:46:26
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "wx/valgen.h"
#include "wxEeschemaStruct.h"

#include "common.h"
#include "class_drawpanel.h"
#include "general.h"
#include "drawtxt.h"
#include "confirm.h"
#include "sch_text.h"

#include "dialog_edit_label.h"


/* Edit the properties of the text (Label, Global label, graphic text).. )
 *  pointed by "aTextStruct"
 */
void SCH_EDIT_FRAME::EditSchematicText( SCH_TEXT* aTextItem )
{
    if( aTextItem == NULL )
        return;

    DialogLabelEditor dialog( this, aTextItem );

    dialog.ShowModal();
}


DialogLabelEditor::DialogLabelEditor( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
    DialogLabelEditor_Base( aParent )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;
    InitDialog();

    GetSizer()->SetSizeHints( this );
    Layout();
    Fit();
    SetMinSize( GetBestSize() );

    Centre();
}


void DialogLabelEditor::InitDialog()
{
    wxString msg;
    bool multiLine = false;

    if( m_CurrentText->m_MultilineAllowed )
    {
        m_textLabel = m_textLabelMultiLine;
        m_textLabelSingleLine->Show(false);
        multiLine = true;
    }
    else
    {
        m_textLabel = m_textLabelSingleLine;
        m_textLabelMultiLine->Show(false);
    }

    m_textLabel->SetValue( m_CurrentText->m_Text );
    m_textLabel->SetFocus();

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:
        SetTitle( _( "Global Label Properties" ) );
        break;

    case SCH_HIERARCHICAL_LABEL_T:
        SetTitle( _( "Hierarchal Label Properties" ) );
        break;

    case SCH_LABEL_T:
        SetTitle( _( "Label Properties" ) );
        break;

    default:
        SetTitle( _( "Text Properties" ) );
        m_textLabel->Disconnect( wxEVT_COMMAND_TEXT_ENTER,
                                 wxCommandEventHandler ( DialogLabelEditor::OnEnterKey ),
                                 NULL, this );
        break;
    }

    int MINTEXTWIDTH = 40;    // M's are big characters, a few establish a lot of width

    int max_len = 0;
    if ( !multiLine )
    {
        max_len =m_CurrentText->m_Text.Length();
    }
    else
    {
        // calculate the length of the biggest line
        // we cannot use the length of the entire text that has no meaning
        int curr_len = MINTEXTWIDTH;
        int imax = m_CurrentText->m_Text.Len();
        for( int count = 0; count < imax; count++ )
        {
            if( m_CurrentText->m_Text[count] == '\n' ||
                m_CurrentText->m_Text[count] == '\r' ) // new line
            {
                curr_len = 0;
            }
            else
            {
                curr_len++;
                if ( max_len < curr_len )
                    max_len = curr_len;
            }
        }
    }
    if( max_len < MINTEXTWIDTH )
        max_len = MINTEXTWIDTH;

    wxString textWidth;
    textWidth.Append( 'M', MINTEXTWIDTH );
    EnsureTextCtrlWidth( m_textLabel, &textWidth );

    // Set validators
    m_TextOrient->SetSelection( m_CurrentText->GetSchematicTextOrientation() );
    m_TextShape->SetSelection( m_CurrentText->m_Shape );

    int style = 0;
    if( m_CurrentText->m_Italic )
        style = 1;
    if( m_CurrentText->m_Bold )
        style += 2;

    m_TextStyle->SetSelection( style );

    wxString units = ReturnUnitSymbol( g_UserUnit, wxT( "(%s)" ) );
    msg = _( "H" ) + units + _( " x W" ) + units;
    m_staticSizeUnits->SetLabel( msg );

    msg = ReturnStringFromValue( g_UserUnit, m_CurrentText->m_Size.x,
                                 m_Parent->m_InternalUnits );
    m_TextSize->SetValue( msg );

    if( m_CurrentText->Type() != SCH_GLOBAL_LABEL_T
     && m_CurrentText->Type() != SCH_HIERARCHICAL_LABEL_T )
    {
        m_TextShape->Show( false );
    }

    m_sdbSizer1OK->SetDefault();
}


/*!
 * wxTE_PROCESS_ENTER  event handler for m_textLabel
 */

void DialogLabelEditor::OnEnterKey( wxCommandEvent& aEvent )
{
    TextPropertiesAccept( aEvent );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DialogLabelEditor::OnOkClick( wxCommandEvent& aEvent )
{
    TextPropertiesAccept( aEvent );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DialogLabelEditor::OnCancelClick( wxCommandEvent& aEvent )
{
    m_Parent->DrawPanel->MouseToCursorSchema();
    EndModal( wxID_CANCEL );
}


void DialogLabelEditor::TextPropertiesAccept( wxCommandEvent& aEvent )
{
    wxString text;
    int      value;

    /* save old text in undo list if not already in edit */
    if( m_CurrentText->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_CurrentText, UR_CHANGED );

    m_Parent->DrawPanel->RefreshDrawingRect( m_CurrentText->GetBoundingBox() );

    text = m_textLabel->GetValue();
    if( !text.IsEmpty() )
        m_CurrentText->m_Text = text;
    else if( (m_CurrentText->m_Flags & IS_NEW) == 0 )
        DisplayError( this, _( "Empty Text!" ) );

    m_CurrentText->SetSchematicTextOrientation( m_TextOrient->GetSelection() );
    text  = m_TextSize->GetValue();
    value = ReturnValueFromString( g_UserUnit, text, m_Parent->m_InternalUnits );
    m_CurrentText->m_Size.x = m_CurrentText->m_Size.y = value;
    if( m_TextShape )
        m_CurrentText->m_Shape = m_TextShape->GetSelection();

    int style = m_TextStyle->GetSelection();
    if( ( style & 1 ) )
        m_CurrentText->m_Italic = 1;
    else
        m_CurrentText->m_Italic = 0;

    if( ( style & 2 ) )
    {
        m_CurrentText->m_Bold  = true;
        m_CurrentText->m_Thickness = GetPenSizeForBold( m_CurrentText->m_Size.x );
    }
    else
    {
        m_CurrentText->m_Bold  = false;
        m_CurrentText->m_Thickness = 0;
    }

    m_Parent->OnModify();

    /* Make the text size as new default size if it is a new text */
    if( (m_CurrentText->m_Flags & IS_NEW) != 0 )
        g_DefaultTextLabelSize = m_CurrentText->m_Size.x;

    m_Parent->DrawPanel->RefreshDrawingRect( m_CurrentText->GetBoundingBox() );
    m_Parent->DrawPanel->MouseToCursorSchema();
    EndModal( wxID_OK );
}
