/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_module_text.cpp
// Author:      jean-pierre Charras
// Licence:		GPL
/////////////////////////////////////////////////////////////////////////////


#include "fctsys.h"
#include "macros.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "drawtxt.h"
#include "confirm.h"
#include "wxBasePcbFrame.h"

#include "class_module.h"
#include "class_text_mod.h"

#include "dialog_edit_module_text_base.h"


extern wxPoint MoveVector;  // Move vector for move edge, imported from edtxtmod.cpp


/*************** **************/
/* class DialogEditModuleText */
/*************** **************/
class DialogEditModuleText : public DialogEditModuleText_base
{
private:
    PCB_BASE_FRAME* m_parent;
    wxDC* m_dc;
    MODULE* m_module;
    TEXTE_MODULE* m_currentText;

public:
    DialogEditModuleText( PCB_BASE_FRAME* aParent, TEXTE_MODULE* aTextMod, wxDC* aDC );
    ~DialogEditModuleText() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
};

/*************************************************************************************/
void PCB_BASE_FRAME::InstallTextModOptionsFrame( TEXTE_MODULE* TextMod, wxDC* DC )
/**************************************************************************************/
{
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    DialogEditModuleText dialog( this, TextMod, DC );
    dialog.ShowModal();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}


DialogEditModuleText::DialogEditModuleText( PCB_BASE_FRAME* aParent,
                                            TEXTE_MODULE* aTextMod, wxDC* aDC ) :
    DialogEditModuleText_base( aParent )

{
    m_parent = aParent;
    m_dc     = aDC;
    m_module = NULL;
    m_currentText = aTextMod;

    if( m_currentText )
        m_module = (MODULE*) m_currentText->GetParent();

    initDlg( );

    m_buttonOK->SetDefault();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    Centre();
}


void DialogEditModuleText::OnCancelClick( wxCommandEvent& event )
{
   EndModal(0);
}


/********************************************************/
void DialogEditModuleText::initDlg( )
/********************************************************/
{
    SetFocus();

    wxString msg;

    if( m_module )
    {
        wxString format = m_ModuleInfoText->GetLabel();
        msg.Printf( format,
            GetChars( m_module->m_Reference->m_Text ),
            GetChars( m_module->m_Value->m_Text ),
            (float) m_module->m_Orient / 10 );
    }

    else
        msg.Empty();

    m_ModuleInfoText->SetLabel( msg );


    if( m_currentText->GetType() == TEXT_is_VALUE )
        m_TextDataTitle->SetLabel( _( "Value:" ) );
    else if( m_currentText->GetType() == TEXT_is_DIVERS )
        m_TextDataTitle->SetLabel( _( "Text:" ) );
    else if( m_currentText->GetType() != TEXT_is_REFERENCE )
        m_TextDataTitle->SetLabel( wxT( "???" ) );

    m_Name->SetValue( m_currentText->m_Text );

    m_Style->SetSelection( m_currentText->m_Italic ? 1 : 0 );

    AddUnitSymbol( *m_SizeXTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlX, m_currentText->m_Size.x,
        m_parent->GetInternalUnits() );

    AddUnitSymbol( *m_SizeYTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlY, m_currentText->m_Size.y,
        m_parent->GetInternalUnits() );

    AddUnitSymbol( *m_PosXTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlX, m_currentText->GetPos0().x,
        m_parent->GetInternalUnits() );

    AddUnitSymbol( *m_PosYTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlY, m_currentText->GetPos0().y,
        m_parent->GetInternalUnits() );

    AddUnitSymbol( *m_WidthTitle );
    PutValueInLocalUnits( *m_TxtWidthCtlr, m_currentText->m_Thickness,
        m_parent->GetInternalUnits() );

    int text_orient = m_currentText->m_Orient;
    NORMALIZE_ANGLE_90(text_orient)
    if( (text_orient != 0) )
        m_Orient->SetSelection( 1 );

    if( !m_currentText->IsVisible() )
        m_Show->SetSelection( 1 );;
}


/*********************************************************************************/
void DialogEditModuleText::OnOkClick( wxCommandEvent& event )
/*********************************************************************************/
{
    wxString msg;

    if ( m_module)
        m_parent->SaveCopyInUndoList( m_module, UR_CHANGED );
    if( m_dc )     //Erase old text on screen
    {
        m_currentText->Draw( m_parent->DrawPanel, m_dc, GR_XOR,
                             (m_currentText->IsMoving()) ? MoveVector : wxPoint( 0, 0 ) );
    }
    m_currentText->m_Text = m_Name->GetValue();

    m_currentText->m_Italic = m_Style->GetSelection() == 1 ? true : false;

    wxPoint tmp;

    msg = m_TxtPosCtrlX->GetValue();
    tmp.x = ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() );

    msg = m_TxtPosCtrlY->GetValue();
    tmp.y = ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() );

    m_currentText->SetPos0( tmp );

    msg = m_TxtSizeCtrlX->GetValue();
    m_currentText->m_Size.x = ReturnValueFromString( g_UserUnit, msg,
        m_parent->GetInternalUnits() );
    msg = m_TxtSizeCtrlY->GetValue();
    m_currentText->m_Size.y = ReturnValueFromString( g_UserUnit, msg,
        m_parent->GetInternalUnits() );

    // Test for a reasonnable size:
    if( m_currentText->m_Size.x< TEXTS_MIN_SIZE )
        m_currentText->m_Size.x = TEXTS_MIN_SIZE;
    if( m_currentText->m_Size.y< TEXTS_MIN_SIZE )
        m_currentText->m_Size.y = TEXTS_MIN_SIZE;

    msg = m_TxtWidthCtlr->GetValue();
    int width = ReturnValueFromString( g_UserUnit, msg, m_parent->GetInternalUnits() );

    // Test for a reasonnable width:
    if( width <= 1 )
        width = 1;
    int maxthickness = Clamp_Text_PenSize(width, m_currentText->m_Size );
    if( width > maxthickness )
    {
        DisplayError(NULL, _("The text thickness is too large for the text size. It will be clamped"));
        width = maxthickness;
    }
    m_currentText->SetThickness( width );

    m_currentText->SetVisible( m_Show->GetSelection() == 0 );

    int text_orient = (m_Orient->GetSelection() == 0) ? 0 : 900;
    m_currentText->m_Orient = text_orient;

    m_currentText->SetDrawCoord();
    if( m_dc )     // Display new text
    {
        m_currentText->Draw( m_parent->DrawPanel, m_dc, GR_XOR,
                             (m_currentText->IsMoving()) ? MoveVector : wxPoint( 0, 0 ) );
    }

    m_parent->OnModify();

    if( m_module )
        m_module->m_LastEdit_Time = time( NULL );

    EndModal(1);
}
