/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <display_footprints_frame.h>
#include <dialog_display_options.h>


void DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay( wxCommandEvent& event )
{
    DIALOG_FOOTPRINTS_DISPLAY_OPTIONS OptionWindow( this );

    OptionWindow.ShowModal();
}


DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( DISPLAY_FOOTPRINTS_FRAME* parent )
    : DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( parent )
{
    m_Parent = parent;

    initDialog();
    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();;
}


DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( )
{
}


void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::initDialog()
{
    /* mandatory to use escape key as cancel under wxGTK. */
    SetFocus();

    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    m_EdgesDisplayOption->SetValue( not displ_opts->m_DisplayModEdgeFill );
    m_TextDisplayOption->SetValue( not displ_opts->m_DisplayModTextFill );
    m_ShowPadSketch->SetValue( not displ_opts->m_DisplayPadFill );
    m_ShowPadNum->SetValue( displ_opts->m_DisplayPadNum );

    m_autoZoomOption->SetValue( m_Parent->GetAutoZoom() );
}


void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::UpdateObjectSettings( void )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();

    displ_opts->m_DisplayModEdgeFill = not m_EdgesDisplayOption->GetValue();
    displ_opts->m_DisplayModTextFill = not m_TextDisplayOption->GetValue();
    displ_opts->m_DisplayPadNum  = m_ShowPadNum->GetValue();
    displ_opts->m_DisplayPadFill = not m_ShowPadSketch->GetValue();
    m_Parent->ApplyDisplaySettingsToGAL();

    m_Parent->SetAutoZoom( m_autoZoomOption->GetValue() );
}


bool DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    UpdateObjectSettings();
    return true;
}


void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnApplyClick( wxCommandEvent& event )
{
    UpdateObjectSettings();
}
