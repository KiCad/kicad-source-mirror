/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KIWAY_EXPRESS_H_
#define KIWAY_EXPRESS_H_

// @see http://wiki.wxwidgets.org/Custom_Events_Tutorial
#include <frame_type.h>
#include <mail_type.h>
#include <kicommon.h>
#include <wx/string.h>
#include <wx/event.h>


/**
 * Carry a payload from one #KIWAY_PLAYER to another within a #PROJECT.
 */
class KICOMMON_API KIWAY_EXPRESS : public wxEvent
{
public:
    /**
     * Return the destination player id of the message.
     */
    FRAME_T Dest() { return m_destination; }

    /**
     * Returns the #MAIL_T associated with this mail.
     */
    MAIL_T Command()
    {
        return (MAIL_T) GetId();    // re-purposed control id.
    }

    /**
     * Return the payload, which can be any text but it typically self identifying s-expression.
     */
    std::string&  GetPayload() { return m_payload; }
    void SetPayload( const std::string& aPayload ) { m_payload = aPayload; }

    KIWAY_EXPRESS* Clone() const override { return new KIWAY_EXPRESS( *this ); }

    //KIWAY_EXPRESS() {}

    KIWAY_EXPRESS( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                   wxWindow* aSource = nullptr );

    KIWAY_EXPRESS( const KIWAY_EXPRESS& anOther );

    /// The wxEventType argument to wxEvent() and identifies an event class
    /// in a hurry.  These wxEventTypes also allow a common class to be used
    /// multiple ways.  Should be allocated at startup by wxNewEventType();
    static const wxEventType wxEVENT_ID;

    //DECLARE_DYNAMIC_CLASS( KIWAY_EXPRESS )

private:
    FRAME_T         m_destination;    ///< could have been a bitmap indicating multiple recipients.
    std::string&    m_payload;        ///< very often s-expression text, but not always.

    // possible new ideas here.
};


typedef void ( wxEvtHandler::*kiwayExpressFunction )( KIWAY_EXPRESS& );

/// Typecast an event handler for the KIWAY_EXPRESS event class
#define kiwayExpressHandler( func ) wxEVENT_HANDLER_CAST( kiwayExpressFunction, func )

/// Event table definition for the KIWAY_EXPRESS event class
#define EVT_KIWAY_EXPRESS( func ) \
    wx__DECLARE_EVT0( KIWAY_EXPRESS::wxEVENT_ID, kiwayExpressHandler( func ) )

#endif  // KIWAY_EXPRESS_H_
