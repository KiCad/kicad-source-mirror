/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_downgrade_report.h"


DIALOG_DOWNGRADE_REPORT::DIALOG_DOWNGRADE_REPORT( wxWindow* aParent, const wxString& aTargetName,
                                                  const COMPATIBILITY_REPORT& aReport ) :
        DIALOG_DOWNGRADE_REPORT_BASE( aParent )
{
    m_reportList->AppendTextColumn( _( "Action" ), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );
    m_reportList->AppendTextColumn( _( "Feature" ), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );
    m_reportList->AppendTextColumn( _( "Detail" ), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );

    auto addRows = [&]( DOWNGRADE_BUCKET aBucket, const wxString& aLabel )
    {
        for( const COMPAT_ENTRY& entry : aReport.Entries() )
        {
            if( entry.m_bucket != aBucket )
                continue;

            wxString feature = entry.m_feature;

            if( entry.m_count > 1 )
                feature += wxString::Format( wxT( " (%d)" ), entry.m_count );

            wxVector<wxVariant> row;
            row.push_back( wxVariant( aLabel ) );
            row.push_back( wxVariant( feature ) );
            row.push_back( wxVariant( entry.m_detail ) );
            m_reportList->AppendItem( row );
        }
    };

    addRows( DOWNGRADE_BUCKET::LOWER, _( "Approximate" ) );
    addRows( DOWNGRADE_BUCKET::DROP, _( "Drop" ) );
    addRows( DOWNGRADE_BUCKET::BLOCK, _( "Cannot export" ) );

    if( aReport.IsBlocked() )
    {
        m_headline->SetLabel( wxString::Format( _( "This project uses features %s cannot represent." ), aTargetName ) );
        m_sdbSizerOK->Hide();
        SetupStandardButtons( { { wxID_CANCEL, _( "Close" ) } } );
        m_sdbSizerCancel->SetDefault();
    }
    else
    {
        m_headline->SetLabel( wxString::Format( _( "Exporting to %s will change some features:" ), aTargetName ) );
        SetupStandardButtons( { { wxID_OK, _( "Export" ) } } );
    }

    finishDialogSettings();
}
