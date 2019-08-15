/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_libentry.h>
#include <dialog_lib_edit_draw_item.h>
#include <lib_edit_frame.h>


DIALOG_LIB_EDIT_DRAW_ITEM::DIALOG_LIB_EDIT_DRAW_ITEM( LIB_EDIT_FRAME* aParent, LIB_ITEM* aItem ) :
    DIALOG_LIB_EDIT_DRAW_ITEM_BASE( aParent ),
    m_frame( aParent ),
    m_item( aItem ),
    m_lineWidth( aParent, m_widthLabel, m_widthCtrl, m_widthUnits, true )
{
    SetTitle( aItem->GetTypeName() + wxT( " " ) + GetTitle() );

    SetInitialFocus( m_widthCtrl );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();
    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::TransferDataToWindow()
{
    LIB_PART* symbol = m_item->GetParent();

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

    m_fillCtrl->SetSelection( m_item->GetFillMode() );
    m_fillCtrl->Enable( m_item->IsFillable() );

    return true;
}


int DIALOG_LIB_EDIT_DRAW_ITEM::GetWidth()
{
    return m_lineWidth.GetValue();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllConversions()
{
    return m_checkApplyToAllConversions->IsChecked();
}


bool DIALOG_LIB_EDIT_DRAW_ITEM::GetApplyToAllUnits()
{
    return m_checkApplyToAllUnits->IsChecked();
}


int DIALOG_LIB_EDIT_DRAW_ITEM::GetFillStyle( void )
{
    return std::max( m_fillCtrl->GetSelection(), 0 );
}

