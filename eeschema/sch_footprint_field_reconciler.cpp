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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sch_footprint_field_reconciler.h"

#include <lib_id.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_symbol.h>


SCH_FOOTPRINT_FIELD_RECONCILER::SCH_FOOTPRINT_FIELD_RECONCILER(
        const wxString& aCacheNickname, const std::vector<wxString>& aSourceLibNicknames,
        REPORTER* aReporter ) :
        m_cacheNickname( aCacheNickname ),
        m_sourceLibs( aSourceLibNicknames.begin(), aSourceLibNicknames.end() ),
        m_reporter( aReporter )
{
}


SCH_FP_FIELD_RECONCILE_RESULT SCH_FOOTPRINT_FIELD_RECONCILER::Reconcile( SCHEMATIC& aSchematic )
{
    SCH_FP_FIELD_RECONCILE_RESULT result;

    // no cache nickname = no project lib to point fields at
    if( m_cacheNickname.IsEmpty() )
        return result;

    SCH_SCREENS screens( aSchematic.Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*     symbol = static_cast<SCH_SYMBOL*>( item );
            const wxString& fpText = symbol->GetField( FIELD_T::FOOTPRINT )->GetText();

            if( fpText.IsEmpty() )
                continue;

            LIB_ID fpid;

            if( fpid.Parse( fpText ) >= 0 )
                continue;   // malformed FPID, leave untouched

            if( fpid.GetUniStringLibItemName().IsEmpty() )
                continue;

            wxString nick = fpid.GetUniStringLibNickname();

            // field already on a registered source lib resolves there
            if( !nick.IsEmpty() && m_sourceLibs.count( nick ) )
            {
                result.m_keptSource++;
                continue;
            }

            fpid.SetLibNickname( m_cacheNickname );
            symbol->SetFootprintFieldText( fpid.Format().wx_str() );
            result.m_relinkedToCache++;
        }
    }

    if( m_reporter && result.m_relinkedToCache > 0 )
    {
        m_reporter->Report(
                wxString::Format( _( "Re-linked %d imported footprint assignment(s) to library "
                                     "'%s'." ),
                                  result.m_relinkedToCache, m_cacheNickname ),
                RPT_SEVERITY_INFO );
    }

    return result;
}
