/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
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
        aFormatter->Print( aNestLevel, "(title_block\n" );

        if( !GetTitle().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( GetTitle() ).c_str() );

        if( !GetDate().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( GetDate() ).c_str() );

        if( !GetRevision().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n",
                               aFormatter->Quotew( GetRevision() ).c_str() );

        if( !GetCompany().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n",
                               aFormatter->Quotew( GetCompany() ).c_str() );

        for( int ii = 0; ii < 9; ii++ )
        {
            if( !GetComment(ii).IsEmpty() )
                aFormatter->Print( aNestLevel+1, "(comment %d %s)\n", ii+1,
                                  aFormatter->Quotew( GetComment(ii) ).c_str() );
        }

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}


bool TITLE_BLOCK::TextVarResolver( wxString* aToken, const PROJECT* aProject ) const
{
    bool tokenUpdated = false;

    if( aToken->IsSameAs( wxT( "ISSUE_DATE" ) ) )
    {
        *aToken = GetDate();
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
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            *aToken = GetComment( c - '0' );
            tokenUpdated = true;
        }
    }

    if( tokenUpdated )
    {
       *aToken = ExpandTextVars( *aToken, nullptr, aProject );
       return true;
    }

    return false;
}


