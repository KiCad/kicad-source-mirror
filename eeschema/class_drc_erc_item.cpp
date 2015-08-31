/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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


/******************************************************************/
/* class_drc_erc_item.cpp - DRC_ITEM class functions for eeschema */
/******************************************************************/
#include <fctsys.h>
#include <common.h>

#include <class_drc_item.h>
#include <erc.h>
#include <base_units.h>

wxString DRC_ITEM::GetErrorText() const
{
    switch( m_ErrorCode )
    {
    case ERCE_UNSPECIFIED:
        return wxString( _("ERC err unspecified") );
    case ERCE_DUPLICATE_SHEET_NAME:
        return wxString( _("Duplicate sheet names within a given sheet") );
    case ERCE_PIN_NOT_CONNECTED:
        return wxString( _("Pin not connected (and no connect symbol found on this pin)") );
    case ERCE_PIN_NOT_DRIVEN:
        return wxString( _("Pin connected to some others pins but no pin to drive it") );
    case ERCE_PIN_TO_PIN_WARNING:
        return wxString( _("Conflict problem between pins. Severity: warning") );
    case ERCE_PIN_TO_PIN_ERROR:
        return wxString( _("Conflict problem between pins. Severity: error") );
    case ERCE_HIERACHICAL_LABEL:
        return wxString( _("Mismatch between hierarchical labels and pins sheets"));
    case ERCE_NOCONNECT_CONNECTED:
        return wxString( _("A no connect symbol is connected to more than 1 pin"));
    case ERCE_GLOBLABEL:
        return wxString( _("Global label not connected to any other global label") );

    default:
        return wxString( wxT("Unkown.") );
    }
}

wxString DRC_ITEM::ShowCoord( const wxPoint& aPos )
{
    wxString ret;
    ret << aPos;
    return ret;
}
