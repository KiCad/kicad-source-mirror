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

#include <drc_item.h>
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
        return wxString( _("Pin not connected (use a \"no connection\" flag to suppress this error)") );
    case ERCE_PIN_NOT_DRIVEN:
        return wxString( _( "Pin connected to other pins, but not driven by any pin" ) );
    case ERCE_PIN_TO_PIN_WARNING:
        return wxString( _("Conflict problem between pins. Severity: warning") );
    case ERCE_PIN_TO_PIN_ERROR:
        return wxString( _("Conflict problem between pins. Severity: error") );
    case ERCE_HIERACHICAL_LABEL:
        return wxString( _("Mismatch between hierarchical labels and pins sheets") );
    case ERCE_NOCONNECT_CONNECTED:
        return wxString( _("A pin with a \"no connection\" flag is connected") );
    case ERCE_NOCONNECT_NOT_CONNECTED:
        return wxString( _("A \"no connection\" flag is not connected to anything") );
    case ERCE_LABEL_NOT_CONNECTED:
        return wxString( _("Label not connected anywhere else in the schematic") );
    case ERCE_SIMILAR_LABELS:
        return wxString( _("Labels are similar (lower/upper case difference only)" ) );
    case ERCE_SIMILAR_GLBL_LABELS:
        return wxString( _("Global labels are similar (lower/upper case difference only)" ) );
    case ERCE_DIFFERENT_UNIT_FP:
        return wxString( _("Different footprint assigned in another unit of the same component" ) );
    case ERCE_DIFFERENT_UNIT_NET:
        return wxString( _("Different net assigned to a shared pin in another unit of the same component" ) );
    case ERCE_BUS_ALIAS_CONFLICT:
        return wxString( _("Conflict between bus alias definitions across schematic sheets") );
    case ERCE_DRIVER_CONFLICT:
        return wxString( _( "More than one name given to this bus or net" ) );
    case ERCE_BUS_ENTRY_CONFLICT:
        return wxString( _( "Net is graphically connected to a bus but not a bus member" ) );
    case ERCE_BUS_LABEL_ERROR:
        return wxString( _( "Label attached to bus item does not describe a bus" ) );
    case ERCE_BUS_TO_BUS_CONFLICT:
        return wxString( _( "No nets are shared between two bus items" ) );
    case ERCE_BUS_TO_NET_CONFLICT:
        return wxString( _( "Invalid connection between bus and net items" ) );
    case ERCE_GLOBLABEL:
        return wxString( _( "Global label not connected anywhere else in the schematic" ) );
    default:
        wxFAIL_MSG( "Missing ERC error description" );
        return wxString( wxT("Unknown.") );
    }
}

wxString DRC_ITEM::ShowCoord( EDA_UNITS_T aUnits, const wxPoint& aPos )
{
    return wxString::Format( "@(%s, %s)",
                             MessageTextFromValue( aUnits, aPos.x ),
                             MessageTextFromValue( aUnits, aPos.y ) );
}


wxString DRC_ITEM::ShowHtml( EDA_UNITS_T aUnits ) const
{
    wxString mainText = m_MainText;
    // a wxHtmlWindows does not like < and > in the text to display
    // because these chars have a special meaning in html
    mainText.Replace( wxT("<"), wxT("&lt;") );
    mainText.Replace( wxT(">"), wxT("&gt;") );

    wxString errText = GetErrorText();
    errText.Replace( wxT("<"), wxT("&lt;") );
    errText.Replace( wxT(">"), wxT("&gt;") );

    wxColour hrefColour = wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT );

    if( m_noCoordinate )
    {
        // omit the coordinate, a NETCLASS has no location
        return wxString::Format( "<p><b>%s</b><br>&nbsp;&nbsp; %s", errText, mainText );
    }
    else if( m_hasSecondItem )
    {
        wxString auxText = m_AuxiliaryText;
        auxText.Replace( wxT("<"), wxT("&lt;") );
        auxText.Replace( wxT(">"), wxT("&gt;") );

        // an html fragment for the entire message in the listbox.  feel free
        // to add color if you want:
        return wxString::Format( "<p><b>%s</b><br>&nbsp;&nbsp; <font color='%s'><a href=''>%s</a></font>: %s<br>&nbsp;&nbsp; %s: %s",
                                 errText,
                                 hrefColour.GetAsString( wxC2S_HTML_SYNTAX ),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 mainText,
                                 ShowCoord( aUnits, m_AuxiliaryPosition ),
                                 auxText  );
    }
    else
    {
        return wxString::Format( "<p><b>%s</b><br>&nbsp;&nbsp; <font color='%s'><a href=''>%s</a></font>: %s",
                                 errText,
                                 hrefColour.GetAsString( wxC2S_HTML_SYNTAX ),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 mainText );
    }
}


wxString DRC_ITEM::ShowReport( EDA_UNITS_T aUnits ) const
{
    if( m_hasSecondItem )
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n    %s: %s\n" ),
                                 m_ErrorCode,
                                 GetErrorText(),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 m_MainText,
                                 ShowCoord( aUnits, m_AuxiliaryPosition ),
                                 m_AuxiliaryText );
    }
    else
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n" ),
                                 m_ErrorCode,
                                 GetErrorText(),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 m_MainText );
    }
}


