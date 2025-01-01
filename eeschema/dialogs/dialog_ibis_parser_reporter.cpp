/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialog_ibis_parser_reporter.h>
#include <sch_edit_frame.h>
#include "widgets/wx_html_report_panel.h"


DIALOG_IBIS_PARSER_REPORTER::DIALOG_IBIS_PARSER_REPORTER( wxWindow* aParent ) :
        DIALOG_IBIS_PARSER_REPORTER_BASE( aParent ), m_frame( aParent )
{
    m_messagePanel->SetLabel( _( "Ibis parser log" ) );
    m_messagePanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
    m_messagePanel->SetLazyUpdate( true );
    m_messagePanel->GetSizer()->SetSizeHints( this );

    SetupStandardButtons( { { wxID_OK, _( "Close" ) } } );

    finishDialogSettings();
}


bool DIALOG_IBIS_PARSER_REPORTER::TransferDataToWindow()
{
    return true;
}


DIALOG_IBIS_PARSER_REPORTER::~DIALOG_IBIS_PARSER_REPORTER()
{
}
