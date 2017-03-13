/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_modedit_display_options.h>

#include <class_drawpanel.h>
#include <module_editor_frame.h>

#include <view/view.h>

#include <widgets/gal_options_panel.h>


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::Invoke( FOOTPRINT_EDIT_FRAME& aCaller )
{
    DIALOG_MODEDIT_DISPLAY_OPTIONS dlg( aCaller );

    int ret = dlg.ShowModal();

    return ret == wxID_OK;
}


DIALOG_MODEDIT_DISPLAY_OPTIONS::DIALOG_MODEDIT_DISPLAY_OPTIONS( FOOTPRINT_EDIT_FRAME& aParent ) :
    DIALOG_SHIM( &aParent, wxID_ANY, _( "Display Options" ) ),
    m_parent( aParent )
{
    auto mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    // install GAL options pane
    KIGFX::GAL_DISPLAY_OPTIONS& galOptions = m_parent.GetGalDisplayOptions();

    m_galOptsPanel = new GAL_OPTIONS_PANEL( this, galOptions );
    mainSizer->Add( m_galOptsPanel, 1, wxEXPAND, 0 );

    auto btnSizer = new wxStdDialogButtonSizer();
    mainSizer->Add( btnSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5 );

    btnSizer->AddButton( new wxButton( this, wxID_OK ) );
    btnSizer->AddButton( new wxButton( this, wxID_CANCEL ) );

    btnSizer->Realize();

    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::TransferDataToWindow()
{
    // update GAL options
    return m_galOptsPanel->TransferDataToWindow();
}


bool DIALOG_MODEDIT_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    // update GAL options
    m_galOptsPanel->TransferDataFromWindow();

    // refresh view
    KIGFX::VIEW* view = m_parent.GetGalCanvas()->GetView();
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    m_parent.GetCanvas()->Refresh();

    return true;
}
