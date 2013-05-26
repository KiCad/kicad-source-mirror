#ifndef _REPORTER_H_
#define _REPORTER_H_

/**
 * @file reporter.h
 * @author Wayne Stambaugh
 * @note A special thanks to Dick Hollenbeck who came up with the idea that inspired
 *       me to write this.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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


class wxString;
class wxTextCtrl;


/**
 * Class REPORTER
 * is a pure virtual class used to derive REPORTOR objects from.
 *
 * The purpose of the REPORTER object is to hide an object that take a string as an input
 * from other objects.  This prevents objects such as wxWidgets UI control internals from
 * being exposed to low level KiCad objects dervice from #BOARD_ITEM and #SCH_ITEM.
 */
class REPORTER
{
public:
    /**
     * Function Report
     * is a pure virtual function to override in the derived object.
     *
     * @param aText is the string to report.
     */
    virtual REPORTER& Report( const wxString& aText ) = 0;

    REPORTER& Report( const char *aText );

    REPORTER& operator <<( const wxString& aText ) { return Report( aText ); }

    REPORTER& operator <<( const wxChar* aText ) { return Report( wxString( aText ) ); }

    REPORTER& operator <<( wxChar aChar ) { return Report( wxString( aChar ) ); }

    REPORTER& operator <<( const char* aText ) { return Report( aText ); }
};


/**
 * Class WX_TEXT_CTRL_REPORTER
 * is wrapper for reporting to a wxTextCtrl object.
 */
class WX_TEXT_CTRL_REPORTER : public REPORTER
{
    wxTextCtrl* m_textCtrl;

public:
    WX_TEXT_CTRL_REPORTER( wxTextCtrl* aTextCtrl ) :
        REPORTER(),
        m_textCtrl( aTextCtrl )
    {
    }

    REPORTER& Report( const wxString& aText );
};

#endif     // _REPORTER_H_
