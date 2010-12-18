/**********************************/
/* dialog_lib_edit_text.cpp */
/**********************************/

/* Code for editing component library text items, not fields. */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "general.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "lib_text.h"

#include "dialog_lib_edit_text.h"


DIALOG_LIB_EDIT_TEXT::DIALOG_LIB_EDIT_TEXT( LIB_EDIT_FRAME* aParent, LIB_TEXT* aText ) :
    DIALOG_LIB_EDIT_TEXT_BASE( aParent )
{
    m_Parent = aParent;
    m_GraphicText = aText;
    InitDialog();
}


void DIALOG_LIB_EDIT_TEXT::InitDialog( )
{
    wxString msg;

    SetFocus();

    if ( m_GraphicText )
    {
        msg = ReturnStringFromValue( g_UserUnit, m_GraphicText->m_Size.x,
                                     m_Parent->m_InternalUnits );
        m_TextSize->SetValue( msg );
        m_TextValue->SetValue( m_GraphicText->m_Text );

        if ( m_GraphicText->GetUnit() == 0 )
            m_CommonUnit->SetValue( TRUE );
        if ( m_GraphicText->GetConvert() == 0 )
            m_CommonConvert->SetValue( TRUE );
        if ( m_GraphicText->m_Orient == TEXT_ORIENT_VERT )
            m_Orient->SetValue( TRUE );

        int shape = 0;
        if ( m_GraphicText->m_Italic )
            shape = 1;
        if ( m_GraphicText->m_Bold )
            shape |= 2;

        m_TextShapeOpt->SetSelection( shape );

        switch ( m_GraphicText->m_HJustify )
        {
            case GR_TEXT_HJUSTIFY_LEFT:
                m_TextHJustificationOpt->SetSelection( 0 );
                break;

            case GR_TEXT_HJUSTIFY_CENTER:
                m_TextHJustificationOpt->SetSelection( 1 );
                break;

            case GR_TEXT_HJUSTIFY_RIGHT:
                m_TextHJustificationOpt->SetSelection( 2 );
                break;

        }

        switch ( m_GraphicText->m_VJustify )
        {
        case GR_TEXT_VJUSTIFY_BOTTOM:
            m_TextVJustificationOpt->SetSelection( 0 );
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            m_TextVJustificationOpt->SetSelection( 1 );
            break;

        case GR_TEXT_VJUSTIFY_TOP:
            m_TextVJustificationOpt->SetSelection( 2 );
            break;
        }
    }
    else
    {
        msg = ReturnStringFromValue( g_UserUnit, m_Parent->m_textSize,
                                     m_Parent->m_InternalUnits );
        m_TextSize->SetValue( msg );

        if ( ! m_Parent->m_drawSpecificUnit )
            m_CommonUnit->SetValue( TRUE );
        if ( ! m_Parent->m_drawSpecificConvert )
            m_CommonConvert->SetValue( TRUE );
        if ( m_Parent->m_textOrientation == TEXT_ORIENT_VERT )
            m_Orient->SetValue( TRUE );
    }

    msg = m_TextSizeText->GetLabel() + ReturnUnitSymbol();
    m_TextSizeText->SetLabel( msg );

    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }

    m_sdbSizer1OK->SetDefault();
}


void DIALOG_LIB_EDIT_TEXT::OnCancelClick( wxCommandEvent& event )
{
    EndModal(wxID_CANCEL);
}


/* Updates the different parameters for the component being edited */
void DIALOG_LIB_EDIT_TEXT::OnOkClick( wxCommandEvent& event )
{
    wxString Line;

    Line = m_TextValue->GetValue();
    m_Parent->m_textOrientation = m_Orient->GetValue() ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
    wxString msg = m_TextSize->GetValue();
    m_Parent->m_textSize = ReturnValueFromString( g_UserUnit, msg, m_Parent->m_InternalUnits );
    m_Parent->m_drawSpecificConvert = m_CommonConvert->GetValue() ? false : true;
    m_Parent->m_drawSpecificUnit = m_CommonUnit->GetValue() ? false : true;

    if( m_GraphicText )
    {
        if( ! Line.IsEmpty() )
            m_GraphicText->SetText( Line );
        else
            m_GraphicText->SetText( wxT( "[null]" ) );

        m_GraphicText->m_Size.x = m_GraphicText->m_Size.y = m_Parent->m_textSize;
        m_GraphicText->m_Orient = m_Parent->m_textOrientation;

        if( m_Parent->m_drawSpecificUnit )
            m_GraphicText->SetUnit( m_Parent->GetUnit() );
        else
            m_GraphicText->SetUnit( 0 );

        if( m_Parent->m_drawSpecificConvert )
            m_GraphicText->SetConvert( m_Parent->GetConvert() );
        else
            m_GraphicText->SetConvert( 0 );

        if( ( m_TextShapeOpt->GetSelection() & 1 ) != 0 )
            m_GraphicText->m_Italic = true;
        else
            m_GraphicText->m_Italic = false;

        if( ( m_TextShapeOpt->GetSelection() & 2 ) != 0 )
            m_GraphicText->m_Bold = true;
        else
            m_GraphicText->m_Bold = false;

        switch( m_TextHJustificationOpt->GetSelection() )
        {
        case 0:
            m_GraphicText->m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
            break;

        case 1:
            m_GraphicText->m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
            break;

        case 2:
            m_GraphicText->m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
            break;
        }

        switch( m_TextVJustificationOpt->GetSelection() )
        {
        case 0:
            m_GraphicText->m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
            break;

        case 1:
            m_GraphicText->m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
            break;

        case 2:
            m_GraphicText->m_VJustify = GR_TEXT_VJUSTIFY_TOP;
            break;
        }
    }

    if( m_Parent->GetDrawItem() )
        m_Parent->GetDrawItem()->DisplayInfo( m_Parent );

    EndModal(wxID_OK);
}
