/**
 * @file  cvpcb/dialogs/dialog_display_options.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>

#include <wxstruct.h>
#include <common.h>
#include <cvpcb.h>
#include <class_drawpanel.h>
#include <cvstruct.h>
#include <class_DisplayFootprintsFrame.h>

#include <dialog_display_options.h>


void DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay( wxCommandEvent& event )
{
    DIALOG_FOOTPRINTS_DISPLAY_OPTIONS* OptionWindow =
        new DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( this );

    OptionWindow->ShowModal();
    OptionWindow->Destroy();
}


DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( PCB_BASE_FRAME* parent )
    : DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( parent )
{
    m_Parent = parent;

    initDialog();
    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( )
{
}


/*!
 * Control creation for DIALOG_FOOTPRINTS_DISPLAY_OPTIONS
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::initDialog()
{
    /* mandatory to use escape key as cancel under wxGTK. */
    SetFocus();

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    m_EdgesDisplayOption->SetSelection( displ_opts->m_DisplayModEdge );
    m_TextDisplayOption->SetSelection( displ_opts->m_DisplayModText );
    m_IsShowPadFill->SetValue( displ_opts->m_DisplayPadFill );
    m_IsShowPadNum->SetValue( displ_opts->m_DisplayPadNum );
    m_IsZoomNoCenter->SetValue( m_Parent->GetCanvas()->GetEnableZoomNoCenter() );
    m_IsMiddleButtonPan->SetValue( m_Parent->GetCanvas()->GetEnableMiddleButtonPan() );
    m_IsMiddleButtonPanLimited->SetValue( m_Parent->GetCanvas()->GetMiddleButtonPanLimited() );
    m_IsMiddleButtonPanLimited->Enable( m_IsMiddleButtonPan->GetValue() );
}



/*!
 * Update settings related to edges, text strings, and pads
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::UpdateObjectSettings( void )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    displ_opts->m_DisplayModEdge = m_EdgesDisplayOption->GetSelection();
    displ_opts->m_DisplayModText = m_TextDisplayOption->GetSelection();
    displ_opts->m_DisplayPadNum  = m_IsShowPadNum->GetValue();
    displ_opts->m_DisplayPadFill = m_IsShowPadFill->GetValue();
    m_Parent->GetCanvas()->SetEnableZoomNoCenter( m_IsZoomNoCenter->GetValue() );
    m_Parent->GetCanvas()->SetEnableMiddleButtonPan( m_IsMiddleButtonPan->GetValue() );
    m_Parent->GetCanvas()->SetMiddleButtonPanLimited( m_IsMiddleButtonPanLimited->GetValue() );
    m_Parent->GetCanvas()->Refresh();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnOkClick( wxCommandEvent& event )
{
    UpdateObjectSettings();
    EndModal( 1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnApplyClick( wxCommandEvent& event )
{
    UpdateObjectSettings();
}
