/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <tools/backannotate.h>
#include <reannotate.h>


WX_STRING_REPORTER_FILTERED::WX_STRING_REPORTER_FILTERED( SEVERITY aSeverity ) :
        m_MinSeverity( aSeverity )
{

}


WX_STRING_REPORTER_FILTERED::~WX_STRING_REPORTER_FILTERED()
{
}


REPORTER& WX_STRING_REPORTER_FILTERED::Report( const wxString &aText, SEVERITY aSeverity )
{
    if ( aSeverity < m_MinSeverity )
        return *this;

    m_string << aText << "\n";
    return *this;
}


bool WX_STRING_REPORTER_FILTERED::HasMessage() const
{
    return !m_string.IsEmpty();
}


void ReannotateFromPCBNew( SCH_EDIT_FRAME* aFrame, std::string& aNetlist )
{
    wxString annotateerrors;
    WX_STRING_REPORTER_FILTERED reporter( SEVERITY::RPT_SEVERITY_ERROR );

    BACK_ANNOTATE backAnno( aFrame,
                            reporter,
                            false,       // aRelinkFootprints
                            false,       // aProcessFootprints
                            false,       // aProcessValues
                            true,        // aProcessReferences
                            false,       // aProcessNetNames
                            false );     // aDryRun

    // TODO (WS): This is completely broken.  BackAnnotate symbols never fails so the attempt
    //            to pass information back through the Kiway payload to Pcbnew never happens.
    //            Attempting to pass information back through the Kiway payload in and of
    //            itself is broken because Kiway payloads were never intended to be bidirectional.
    if( !backAnno.BackAnnotateSymbols( aNetlist ) )
    {
        aNetlist = _( "Errors reported by Eeschema:\n" ) + reporter.m_string.ToStdString();
        aNetlist += _( "\nAnnotation not performed!\n" );     // Assume the worst
    }
    else
    {
        aNetlist.clear();       //Empty means good
    }

    aFrame->GetCurrentSheet().UpdateAllScreenReferences();
    aFrame->SetSheetNumberAndCount();
    aFrame->SyncView();
    aFrame->OnModify();
    aFrame->GetCanvas()->Refresh();
}
