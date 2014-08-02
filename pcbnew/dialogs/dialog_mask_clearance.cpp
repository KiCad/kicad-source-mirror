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
/**
 * @file dialog_mask_clearance.cpp
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <base_units.h>

#include <class_board.h>

#include <dialog_mask_clearance.h>

/**
 * Class DIALOG_PADS_MASK_CLEARANCE
 * is derived from DIALOG_PADS_MASK_CLEARANCE_BASE.
 * @see dialog_dialog_mask_clearance_base.h and dialog_mask_clearance_base.cpp,
 * which are maintained by wxFormBuilder
 */
DIALOG_PADS_MASK_CLEARANCE::DIALOG_PADS_MASK_CLEARANCE( PCB_EDIT_FRAME* aParent ) :
    DIALOG_PADS_MASK_CLEARANCE_BASE( aParent )
{
    m_parent = aParent;
    m_brdSettings = m_parent->GetBoard()->GetDesignSettings();

    myInit();
    m_sdbButtonsSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_PADS_MASK_CLEARANCE::myInit()
{
    SetFocus();

    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_solderMaskMinWidthUnit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    PutValueInLocalUnits( *m_SolderMaskMarginCtrl, m_brdSettings.m_SolderMaskMargin );
    PutValueInLocalUnits( *m_SolderMaskMinWidthCtrl, m_brdSettings.m_SolderMaskMinWidth );

    // These 2 parameters are usually < 0, so prepare entering a negative
    // value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl, m_brdSettings.m_SolderPasteMargin );

    if(  m_brdSettings.m_SolderPasteMargin == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) +
                                           m_SolderPasteMarginCtrl->GetValue() );

    wxString msg;
    msg.Printf( wxT( "%f" ), m_brdSettings.m_SolderPasteMarginRatio * 100.0 );

    // Sometimes Printf adds a sign if the value is small
    if(  m_brdSettings.m_SolderPasteMarginRatio == 0.0 && msg[0] == '0' )
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );
}


void DIALOG_PADS_MASK_CLEARANCE::OnButtonOkClick( wxCommandEvent& event )
{
    m_brdSettings.m_SolderMaskMargin = ValueFromTextCtrl( *m_SolderMaskMarginCtrl );
    m_brdSettings.m_SolderMaskMinWidth = ValueFromTextCtrl( *m_SolderMaskMinWidthCtrl );

    m_brdSettings.m_SolderPasteMargin = ValueFromTextCtrl( *m_SolderPasteMarginCtrl );

    double      dtmp    = 0;
    wxString    msg     = m_SolderPasteMarginRatioCtrl->GetValue();

    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;

    if( dtmp > +100 )
        dtmp = +100;

    m_brdSettings.m_SolderPasteMarginRatio = dtmp / 100;

    m_parent->OnModify();

    m_parent->GetBoard()->SetDesignSettings( m_brdSettings );

    EndModal( 1 );
}


void DIALOG_PADS_MASK_CLEARANCE::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}
