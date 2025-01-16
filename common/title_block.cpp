/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <richio.h>
#include <common.h>
#include <title_block.h>
#include <core/kicad_algo.h>


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter ) const
{
    // Don't write the title block information if there is nothing to write.
    bool isempty = true;
    for( unsigned idx = 0; idx < m_tbTexts.GetCount(); idx++ )
    {
        if( ! m_tbTexts[idx].IsEmpty() )
        {
            isempty = false;
            break;
        }
    }

    if( !isempty  )
    {
        aFormatter->Print( "(title_block" );

        if( !GetTitle().IsEmpty() )
            aFormatter->Print( "(title %s)", aFormatter->Quotew( GetTitle() ).c_str() );

        if( !GetDate().IsEmpty() )
            aFormatter->Print( "(date %s)", aFormatter->Quotew( GetDate() ).c_str() );

        if( !GetRevision().IsEmpty() )
            aFormatter->Print( "(rev %s)", aFormatter->Quotew( GetRevision() ).c_str() );

        if( !GetCompany().IsEmpty() )
            aFormatter->Print( "(company %s)", aFormatter->Quotew( GetCompany() ).c_str() );

        for( int ii = 0; ii < 9; ii++ )
        {
            if( !GetComment(ii).IsEmpty() )
            {
                aFormatter->Print( "(comment %d %s)",
                                   ii+1,
                                   aFormatter->Quotew( GetComment(ii) ).c_str() );
            }
        }

        aFormatter->Print( ")" );
    }
}


void TITLE_BLOCK::GetContextualTextVars( wxArrayString* aVars )
{
    if( !alg::contains( *aVars, wxT( "ISSUE_DATE" ) ) )
    {
        aVars->push_back( wxT( "ISSUE_DATE" ) );
        aVars->push_back( wxT( "CURRENT_DATE" ) );
        aVars->push_back( wxT( "REVISION" ) );
        aVars->push_back( wxT( "TITLE" ) );
        aVars->push_back( wxT( "COMPANY" ) );
        aVars->push_back( wxT( "COMMENT1" ) );
        aVars->push_back( wxT( "COMMENT2" ) );
        aVars->push_back( wxT( "COMMENT3" ) );
        aVars->push_back( wxT( "COMMENT4" ) );
        aVars->push_back( wxT( "COMMENT5" ) );
        aVars->push_back( wxT( "COMMENT6" ) );
        aVars->push_back( wxT( "COMMENT7" ) );
        aVars->push_back( wxT( "COMMENT8" ) );
        aVars->push_back( wxT( "COMMENT9" ) );
    }
}


wxString TITLE_BLOCK::GetCurrentDate()
{
    // We can choose different formats. Should probably be kept in sync with ISSUE_DATE
    // formatting in DIALOG_PAGES_SETTINGS.
    //
    //  return wxDateTime::Now().Format( wxLocale::GetInfo( wxLOCALE_SHORT_DATE_FMT ) );
    //  return wxDateTime::Now().Format( wxLocale::GetInfo( wxLOCALE_LONG_DATE_FMT ) );
    //  return wxDateTime::Now().Format( wxT("%Y-%b-%d") );
    return wxDateTime::Now().FormatISODate();
};


bool TITLE_BLOCK::TextVarResolver( wxString* aToken, const PROJECT* aProject, int aFlags ) const
{
    bool tokenUpdated = false;
    wxString originalToken = *aToken;

    if( aToken->IsSameAs( wxT( "ISSUE_DATE" ) ) )
    {
        *aToken = GetDate();
        tokenUpdated = true;
    }
    else if( aToken->IsSameAs( wxT( "CURRENT_DATE" ) ) )
    {
        *aToken = GetCurrentDate();
        tokenUpdated = true;
    }
    else if( aToken->IsSameAs( wxT( "REVISION" ) ) )
    {
        *aToken = GetRevision();
        tokenUpdated = true;
    }
    else if( aToken->IsSameAs( wxT( "TITLE" ) ) )
    {
        *aToken = GetTitle();
        tokenUpdated = true;
    }
    else if( aToken->IsSameAs( wxT( "COMPANY" ) ) )
    {
        *aToken = GetCompany();
        tokenUpdated = true;
    }
    else if( aToken->Left( aToken->Len() - 1 ).IsSameAs( wxT( "COMMENT" ) ) )
    {
        wxChar c = aToken->Last();

        switch( c )
        {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            *aToken = GetComment( c - '1' );
            tokenUpdated = true;
        }
    }

    if( tokenUpdated )
    {
        if( aToken->IsSameAs( wxT( "CURRENT_DATE" ) ) )
            *aToken = GetCurrentDate();
        else if( aProject )
            *aToken = ExpandTextVars( *aToken, aProject, aFlags );

        // This is the default fallback, so don't claim we resolved it
        if( *aToken == wxT( "${" ) + originalToken + wxT( "}" ) )
            return false;

       return true;
    }

    return false;
}


