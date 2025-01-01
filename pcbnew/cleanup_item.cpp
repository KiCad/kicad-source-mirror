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


#include <cleanup_item.h>
#include <pcb_base_frame.h>


CLEANUP_ITEM::CLEANUP_ITEM( int aErrorCode )
{
    m_errorCode = aErrorCode;
    m_errorTitle = GetErrorText( aErrorCode );
}


wxString CLEANUP_ITEM::GetErrorText( int aCode, bool aTranslate ) const
{
    wxString msg;

    if( aCode < 0 )
        aCode = m_errorCode;

    switch( aCode )
    {
    // For cleanup tracks and vias:
    case CLEANUP_SHORTING_TRACK:    msg = _HKI( "Remove track shorting two nets" );             break;
    case CLEANUP_SHORTING_VIA:      msg = _HKI( "Remove via shorting two nets" );               break;
    case CLEANUP_REDUNDANT_VIA:     msg = _HKI( "Remove redundant via" );                       break;
    case CLEANUP_DUPLICATE_TRACK:   msg = _HKI( "Remove duplicate track" );                     break;
    case CLEANUP_MERGE_TRACKS:      msg = _HKI( "Merge co-linear tracks" );                     break;
    case CLEANUP_DANGLING_TRACK:    msg = _HKI( "Remove track not connected at both ends" );    break;
    case CLEANUP_DANGLING_VIA:      msg = _HKI( "Remove via connected on less than 2 layers" ); break;
    case CLEANUP_ZERO_LENGTH_TRACK: msg = _HKI( "Remove zero-length track" );                   break;
    case CLEANUP_TRACK_IN_PAD:      msg = _HKI( "Remove track inside pad" );                    break;

    // For cleanup graphics:
    case CLEANUP_NULL_GRAPHIC:      msg = _HKI( "Remove zero-size graphic" );                   break;
    case CLEANUP_DUPLICATE_GRAPHIC: msg = _HKI( "Remove duplicated graphic" );                  break;
    case CLEANUP_LINES_TO_RECT:     msg = _HKI( "Convert lines to rectangle" );                 break;
    case CLEANUP_MERGE_PAD:         msg = _HKI( "Merge overlapping shapes into pad" );          break;

    default:
        wxFAIL_MSG( wxT( "Missing cleanup item description" ) );
        msg = _HKI( "Unknown cleanup action" );
        break;
    }

    if( aTranslate )
        return wxGetTranslation( msg );
    else
        return msg;
}


