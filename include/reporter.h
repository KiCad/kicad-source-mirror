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
 * is a pure virtual class used to derive REPORTER objects from.
 *
 * The purpose of the REPORTER object is to offer a way for a procedural function
 * to report multiple errors without having to:
 * <ul>
 * <li> know too much about the caller's UI, i.e. wx. </li>
 * <li> stop after the first error </li>
 * </ul>
 * the reporter has 3 levels (flags) for filtering:
 * no filter
 * report warning
 * report errors
 * They are indicators for the calling code, filtering is not made here
 */
class REPORTER
{
    bool m_reportAll;       // Filter flag: set to true to report all messages
    bool m_reportWarnings;  // Filter flag: set to true to report warning
    bool m_reportErrors;    // Filter flag: set to true to report errors

public:
    /**
     * Function Report
     * is a pure virtual function to override in the derived object.
     *
     * @param aText is the string to report.
     */
    virtual REPORTER& Report( const wxString& aText ) = 0;

    REPORTER& Report( const char* aText );

    REPORTER& operator <<( const wxString& aText ) { return Report( aText ); }

    REPORTER& operator <<( const wxChar* aText ) { return Report( wxString( aText ) ); }

    REPORTER& operator <<( wxChar aChar ) { return Report( wxString( aChar ) ); }

    REPORTER& operator <<( const char* aText ) { return Report( aText ); }

    /**
     * Returns true if all messages should be reported
     */
    bool ReportAll() { return m_reportAll; }

    /**
     * Returns true if all messages or warning messages should be reported
     */
    bool ReportWarnings() { return m_reportAll | m_reportWarnings; }

    /**
     * Returns true if all messages or error messages should be reported
     */
    bool ReportErrors() { return m_reportAll | m_reportErrors; }

    /**
     * Set the report filter state, for all messages
     * @param aEnable = filter state (true/false)
     */
    void SetReportAll( bool aEnable) { m_reportAll = aEnable; }

    /**
     * Set the report filter state, for warning messages
     * note: report can be disable only if m_reportAll = false
     * @param aEnable = filter state (true/false)
     */
    void SetReportWarnings( bool aEnable) { m_reportWarnings = aEnable; }

    /**
     * Set the report filter state, for error messages
     * note: report can be disable only if m_reportAll = false
     * @param aEnable = filter state (true/false)
     */
    void SetReportErrors( bool aEnable) { m_reportErrors = aEnable; }
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
        SetReportAll( true );
        SetReportWarnings( true );
        SetReportErrors( true );
    }

    REPORTER& Report( const wxString& aText );
};


/**
 * Class WX_STRING_REPORTER
 * is a wrapper for reporting to a wxString object.
 */
class WX_STRING_REPORTER : public REPORTER
{
    wxString* m_string;

public:
    WX_STRING_REPORTER( wxString* aString ) :
        REPORTER(),
        m_string( aString )
    {
    }

    REPORTER& Report( const wxString& aText );
};

#endif     // _REPORTER_H_
