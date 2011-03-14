/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_module_text.cpp
// Author:      jean-pierre Charras
// Licence:		GPL
/////////////////////////////////////////////////////////////////////////////


#include "fctsys.h"
#include "macros.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "drawtxt.h"
#include "confirm.h"

#include "dialog_edit_module_text_base.h"

extern wxPoint MoveVector;  // Move vector for move edge, imported from edtxtmod.cpp

/*************** **************/
/* class DialogEditModuleText */
/*************** **************/
class DialogEditModuleText : public DialogEditModuleText_base
{
private:
    PCB_BASE_FRAME* m_Parent;
    wxDC* m_DC;
    MODULE* m_Module;
    TEXTE_MODULE* m_CurrentTextMod;

public:
    DialogEditModuleText( PCB_BASE_FRAME* parent, TEXTE_MODULE* TextMod, wxDC* DC );
    ~DialogEditModuleText() {};

private:
    void Init( );
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


DialogEditModuleText::DialogEditModuleText( PCB_BASE_FRAME* parent,
                                            TEXTE_MODULE* TextMod, wxDC* DC ) :
    DialogEditModuleText_base(parent)

{
    m_Parent = parent;
    m_DC     = DC;
    m_Module = NULL;
    m_CurrentTextMod = TextMod;

    if( m_CurrentTextMod )
        m_Module = (MODULE*) m_CurrentTextMod->GetParent();

    Init( );

    m_buttonOK->SetDefault();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DialogEditModuleText::OnCancelClick( wxCommandEvent& event )
{
   EndModal(0);
}


/********************************************************/
void DialogEditModuleText::Init( )
/********************************************************/
{
    SetFocus();

    wxString msg;

    if( m_Module )
    {
        wxString format = m_ModuleInfoText->GetLabel();
        msg.Printf( format,
            GetChars( m_Module->m_Reference->m_Text ),
            GetChars( m_Module->m_Value->m_Text ),
            (float) m_Module->m_Orient / 10 );
    }

    else
        msg.Empty();

    m_ModuleInfoText->SetLabel( msg );


    if( m_CurrentTextMod->m_Type == TEXT_is_VALUE )
        m_TextDataTitle->SetLabel( _( "Value:" ) );
    else if( m_CurrentTextMod->m_Type == TEXT_is_DIVERS )
        m_TextDataTitle->SetLabel( _( "Text:" ) );
    else if( m_CurrentTextMod->m_Type != TEXT_is_REFERENCE )
        m_TextDataTitle->SetLabel( wxT( "???" ) );

    m_Name->SetValue( m_CurrentTextMod->m_Text );

    m_Style->SetSelection( m_CurrentTextMod->m_Italic ? 1 : 0 );

    AddUnitSymbol( *m_SizeXTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlX, m_CurrentTextMod->m_Size.x,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_SizeYTitle );
    PutValueInLocalUnits( *m_TxtSizeCtrlY, m_CurrentTextMod->m_Size.y,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_PosXTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlX, m_CurrentTextMod->m_Pos0.x,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_PosYTitle );
    PutValueInLocalUnits( *m_TxtPosCtrlY, m_CurrentTextMod->m_Pos0.y,
        m_Parent->m_InternalUnits );

    AddUnitSymbol( *m_WidthTitle );
    PutValueInLocalUnits( *m_TxtWidthCtlr, m_CurrentTextMod->m_Thickness,
        m_Parent->m_InternalUnits );

    int text_orient = m_CurrentTextMod->m_Orient;
    NORMALIZE_ANGLE_90(text_orient)
    if( (text_orient != 0) )
        m_Orient->SetSelection( 1 );

    if( m_CurrentTextMod->m_NoShow )
        m_Show->SetSelection( 1 );;

}


/*********************************************************************************/
void DialogEditModuleText::OnOkClick( wxCommandEvent& event )
/*********************************************************************************/
{
    wxString msg;

    if ( m_Module)
        m_Parent->SaveCopyInUndoList( m_Module, UR_CHANGED );
    if( m_DC )     //Erase old text on screen
    {
        m_CurrentTextMod->Draw( m_Parent->DrawPanel, m_DC, GR_XOR,
            (m_CurrentTextMod->m_Flags & IS_MOVED) ? MoveVector : wxPoint( 0, 0 ) );
    }
    m_CurrentTextMod->m_Text = m_Name->GetValue();

    m_CurrentTextMod->m_Italic = m_Style->GetSelection() == 1 ? true : false;


    msg = m_TxtPosCtrlX->GetValue();
    m_CurrentTextMod->m_Pos0.x = ReturnValueFromString( g_UserUnit, msg,
        m_Parent->m_InternalUnits );
    msg = m_TxtPosCtrlY->GetValue();
    m_CurrentTextMod->m_Pos0.y = ReturnValueFromString( g_UserUnit, msg,
        m_Parent->m_InternalUnits );

    msg = m_TxtSizeCtrlX->GetValue();
    m_CurrentTextMod->m_Size.x = ReturnValueFromString( g_UserUnit, msg,
        m_Parent->m_InternalUnits );
    msg = m_TxtSizeCtrlY->GetValue();
    m_CurrentTextMod->m_Size.y = ReturnValueFromString( g_UserUnit, msg,
        m_Parent->m_InternalUnits );

    // Test for a reasonnable size:
    if( m_CurrentTextMod->m_Size.x< TEXTS_MIN_SIZE )
        m_CurrentTextMod->m_Size.x = TEXTS_MIN_SIZE;
    if( m_CurrentTextMod->m_Size.y< TEXTS_MIN_SIZE )
        m_CurrentTextMod->m_Size.y = TEXTS_MIN_SIZE;

    msg = m_TxtWidthCtlr->GetValue();
    int width = ReturnValueFromString( g_UserUnit, msg, m_Parent->m_InternalUnits );

    // Test for a reasonnable width:
    if( width <= 1 )
        width = 1;
    int maxthickness = Clamp_Text_PenSize(width, m_CurrentTextMod->m_Size );
    if( width > maxthickness )
    {
        DisplayError(NULL, _("The text thickness is too large for the text size. It will be clamped"));
        width = maxthickness;
    }
    m_CurrentTextMod->SetThickness( width );

    m_CurrentTextMod->m_NoShow = (m_Show->GetSelection() == 0) ? 0 : 1;
    int text_orient = (m_Orient->GetSelection() == 0) ? 0 : 900;
    m_CurrentTextMod->m_Orient = text_orient;

    m_CurrentTextMod->SetDrawCoord();
    if( m_DC )     // Display new text
    {
        m_CurrentTextMod->Draw( m_Parent->DrawPanel, m_DC, GR_XOR,
            (m_CurrentTextMod->m_Flags & IS_MOVED) ? MoveVector : wxPoint( 0, 0 ) );
    }
    m_Parent->OnModify();
    if( m_Module )
        m_Module->m_LastEdit_Time = time( NULL );

    EndModal(1);
}
