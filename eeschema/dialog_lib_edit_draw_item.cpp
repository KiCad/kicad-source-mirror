#include "dialog_lib_edit_draw_item.h"


DIALOG_LIB_EDIT_DRAW_ITEM::DIALOG_LIB_EDIT_DRAW_ITEM( wxWindow* parent,
                                                      const wxString& itemName ) :
    DIALOG_LIB_EDIT_DRAW_ITEM_BASE( parent )
{
    SetTitle( itemName + wxT( " " ) + GetTitle() );
}


void DIALOG_LIB_EDIT_DRAW_ITEM::SetWidth( const wxString& width )
{
    m_textWidth->SetValue( width );
}


wxString DIALOG_LIB_EDIT_DRAW_ITEM::GetWidth( void )
{
    return m_textWidth->GetValue();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllConversions( void )
{
    return m_checkApplyToAllConversions->IsChecked();
}


void DIALOG_LIB_EDIT_DRAW_ITEM::SetApplyToAllConversions( bool applyToAll )
{
    m_checkApplyToAllConversions->SetValue( applyToAll );
}


void DIALOG_LIB_EDIT_DRAW_ITEM::EnableApplyToAllConversions( bool enable )
{
    m_checkApplyToAllConversions->Enable( enable );
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllUnits( void )
{
    return m_checkApplyToAllUnits->IsChecked();
}


void DIALOG_LIB_EDIT_DRAW_ITEM::SetApplyToAllUnits( bool applyToAll )
{
    m_checkApplyToAllUnits->SetValue( applyToAll );
}


void DIALOG_LIB_EDIT_DRAW_ITEM::EnableApplyToAllUnits( bool enable )
{
    m_checkApplyToAllUnits->Enable( enable );
}


int DIALOG_LIB_EDIT_DRAW_ITEM::GetFillStyle( void )
{
    if( m_radioFillNone->GetValue() )
        return 0;
    if( m_radioFillForeground->GetValue() )
        return 1;
    if( m_radioFillBackground->GetValue() )
        return 2;

    return 0;
}


void DIALOG_LIB_EDIT_DRAW_ITEM::SetFillStyle( int fillStyle )
{
    if( fillStyle == 1 )
        m_radioFillForeground->SetValue( true );
    else if( fillStyle == 2 )
        m_radioFillBackground->SetValue( true );
    else
        m_radioFillNone->SetValue( true );
}


void DIALOG_LIB_EDIT_DRAW_ITEM::EnableFillStyle( bool enable )
{
    m_radioFillNone->Enable( enable );
    m_radioFillForeground->Enable( enable );
    m_radioFillBackground->Enable( enable );
}


void DIALOG_LIB_EDIT_DRAW_ITEM::SetWidthUnits( const wxString& units )
{
    m_staticWidthUnits->SetLabel( units );
}
