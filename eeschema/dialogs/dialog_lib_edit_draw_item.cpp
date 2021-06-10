/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_lib_edit_draw_item.cpp
 */

#include <lib_item.h>
#include <dialog_lib_edit_draw_item.h>
#include <symbol_edit_frame.h>
#include <confirm.h>


DIALOG_LIB_EDIT_DRAW_ITEM::DIALOG_LIB_EDIT_DRAW_ITEM( SYMBOL_EDIT_FRAME* aParent,
                                                      LIB_ITEM* aItem ) :
    DIALOG_LIB_EDIT_DRAW_ITEM_BASE( aParent ),
    m_frame( aParent ),
    m_item( aItem ),
    m_lineWidth( aParent, m_widthLabel, m_widthCtrl, m_widthUnits, true )
{
    SetTitle( aItem->GetTypeName() + wxT( " " ) + GetTitle() );

    SetInitialFocus( m_widthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();

    if( !aParent->IsSymbolEditable() || aParent->IsSymbolAlias() )
    {
        m_sdbSizerCancel->SetDefault();
        m_sdbSizerOK->SetLabel( _( "Read Only" ) );
        m_sdbSizerOK->Enable( false );
    }
    else
    {
        m_sdbSizerOK->SetDefault();
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LIB_SYMBOL* symbol = m_item->GetParent();

    m_lineWidth.SetValue( m_item->GetWidth() );
    m_checkApplyToAllUnits->SetValue( m_item->GetUnit() == 0 );
    m_checkApplyToAllUnits->Enable( symbol && symbol->GetUnitCount() > 1 );
    m_checkApplyToAllConversions->SetValue( m_item->GetConvert() == 0 );

    bool enblConvOptStyle = symbol && symbol->HasConversion();

    // if a symbol contains no graphic items, symbol->HasConversion() returns false.
    // but when creating a new symbol, with DeMorgan option set, the ApplyToAllConversions
    // must be enabled even if symbol->HasConversion() returns false in order to be able
    // to create graphic items shared by all body styles
    if( m_frame->GetShowDeMorgan() )
        enblConvOptStyle = true;

    m_checkApplyToAllConversions->Enable( enblConvOptStyle );

    m_fillCtrl->SetSelection( static_cast<int>( m_item->GetFillMode() ) );
    m_fillCtrl->Enable( m_item->IsFillable() );

    return true;
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Min. 1 um and max. 1 m are more than enough.
    if( !m_lineWidth.Validate( 0, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    if( m_lineWidth.GetValue() == 0
        && ( (FILL_TYPE) m_fillCtrl->GetSelection() ) != FILL_TYPE::FILLED_SHAPE )
    {
        DisplayError( this, _( "Line width may not be 0 for shapes other than filled with body "
            "outline color." ) );
        m_widthCtrl->SetFocus();
        return false;
    }

    if( m_item->IsFillable() )
        m_item->SetFillMode( (FILL_TYPE) std::max( m_fillCtrl->GetSelection(), 0 ) );

    m_item->SetWidth( m_lineWidth.GetValue() );

    if( GetApplyToAllConversions() )
        m_item->SetConvert( 0 );
    else
        m_item->SetConvert( m_frame->GetConvert() );

    if( GetApplyToAllUnits() )
        m_item->SetUnit( 0 );
    else
        m_item->SetUnit( m_frame->GetUnit() );

    return true;
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllConversions()
{
    return m_checkApplyToAllConversions->IsChecked();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllUnits()
{
    return m_checkApplyToAllUnits->IsChecked();
}

