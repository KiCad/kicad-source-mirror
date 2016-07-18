/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <dialog_lib_edit_draw_item.h>


DIALOG_LIB_EDIT_DRAW_ITEM::DIALOG_LIB_EDIT_DRAW_ITEM( wxWindow* parent,
                                                      const wxString& itemName ) :
    DIALOG_LIB_EDIT_DRAW_ITEM_BASE( parent )
{
    SetTitle( itemName + wxT( " " ) + GetTitle() );

    // Required under wxGTK if we want to dismiss the dialog with the ESC key
    SetFocus();
    m_sdbSizer1OK->SetDefault();

    FixOSXCancelButtonIssue();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
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
