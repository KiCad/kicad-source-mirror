/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _REPORTER_H_
#define _REPORTER_H_

#include <memory>
#include <map>

#include <eda_units.h>
#include <widgets/report_severity.h>
#include <kicommon.h>

/**
 * @file reporter.h
 * @author Wayne Stambaugh
 * @note A special thanks to Dick Hollenbeck who came up with the idea that inspired
 *       me to write this.
 * @warning Do not add any dependencies to wxWidgets (or any other third party UI library )
 *          to the REPORTER object.  All wxWidgets objects should be defined by pointer or
 *          reference and forward declared so that using reporters in low level KiCad objects
 *          will not require pulling in wxWidgets to building them.
 */

class wxString;
class wxStatusBar;
class wxTextCtrl;
class WX_HTML_REPORT_PANEL;
class WX_INFOBAR;


/**
 * A pure virtual class used to derive REPORTER objects from.
 *
 * The purpose of the REPORTER object is to offer a way for a procedural function
 * to report multiple errors without having to:
 * <ul>
 * <li> know too much about the caller's UI, i.e. wx. </li>
 * <li> stop after the first error </li>
 * </ul>
 * the reporter has 4 severity levels (flags) tagging the messages:
 *  - information
 *  - warning
 *  - error
 *  - action (i.e. indication of changes - add component, change footprint, etc. )
 *
 * They are indicators for the message formatting and displaying code,
 * filtering is not made here.
 */

class KICOMMON_API REPORTER
{
public:
    REPORTER() :
            m_reportedSeverityMask( 0 )
    { }

    virtual ~REPORTER()
    { }

    /**
     * Location where the message is to be reported.
     * LOC_HEAD messages are printed before all others (typically intro messages)
     * LOC_BODY messages are printed in the middle
     * LOC_TAIL messages are printed after all others (typically status messages)
     */
    enum LOCATION {
        LOC_HEAD = 0,
        LOC_BODY,
        LOC_TAIL
    };

    /**
     * Report a string with a given severity.
     *
     * @param aText is the string to report.
     * @param aSeverity is an indicator ( RPT_UNDEFINED, RPT_INFO, RPT_WARNING, RPT_ERROR,
     *                  RPT_ACTION ) used to filter and format messages
     */

    virtual REPORTER& Report( const wxString& aText,
                              SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED )
    {
        m_reportedSeverityMask |= aSeverity;
        return *this;
    }

    /**
     * Places the report at the end of the list, for objects that support report ordering
     */
    virtual REPORTER& ReportTail( const wxString& aText,
                                  SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED )
    {
        return Report( aText, aSeverity );
    }

    /**
     * Places the report at the beginning of the list for objects that support ordering.
     */
    virtual REPORTER& ReportHead( const wxString& aText,
                                  SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED )
    {
        return Report( aText, aSeverity );
    }

    REPORTER& Report( const char* aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED );

    REPORTER& operator <<( const wxString& aText ) { return Report( aText ); }

    /**
     * Returns true if any messages were reported.
     */
    virtual bool HasMessage() const
    {
        return m_reportedSeverityMask != 0;
    }

    /**
     * Returns true if the reporter has one or more messages matching the specified
     * severity mask.
     */
    virtual bool HasMessageOfSeverity( int aSeverityMask ) const
    {
        return ( m_reportedSeverityMask & aSeverityMask ) != 0;
    }

    virtual EDA_UNITS GetUnits() const
    {
        return EDA_UNITS::MM;
    }

    virtual void Clear()
    {
        m_reportedSeverityMask = 0;
    }

private:
    int m_reportedSeverityMask;
};


/**
 * A wrapper for reporting to a wxTextCtrl object.
 */
class KICOMMON_API WX_TEXT_CTRL_REPORTER : public REPORTER
{
public:
    WX_TEXT_CTRL_REPORTER( wxTextCtrl* aTextCtrl ) :
        REPORTER(),
        m_textCtrl( aTextCtrl )
    {
    }

    virtual ~WX_TEXT_CTRL_REPORTER()
    {
    }

    REPORTER& Report( const wxString& aText,
                      SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

private:
    wxTextCtrl* m_textCtrl;
};


/**
 * A wrapper for reporting to a wxString object.
 */
class KICOMMON_API WX_STRING_REPORTER : public REPORTER
{
public:
    WX_STRING_REPORTER() :
            REPORTER()
    { }

    virtual ~WX_STRING_REPORTER()
    { }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    const wxString& GetMessages() const;
    void            Clear() override;

private:
    wxString m_string;
};


/**
 * A singleton reporter that reports to nowhere.
 *
 * Used as to simplify code by avoiding the reportee to check for a non-NULL reporter object.
 */
class KICOMMON_API NULL_REPORTER : public REPORTER
{
public:
    NULL_REPORTER()
    { }

    virtual ~NULL_REPORTER()
    { }

    static REPORTER& GetInstance();

    REPORTER& Report( const wxString& aText,
                      SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
};


/**
 * Reporter forwarding messages to stdout or stderr as appropriate
 */
class KICOMMON_API CLI_REPORTER : public REPORTER
{
public:
    CLI_REPORTER()
    { }

    virtual ~CLI_REPORTER()
    { }

    static REPORTER& GetInstance();

    REPORTER& Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
};


/**
 * Debug type reporter, forwarding messages to std::cout.
 */
class KICOMMON_API STDOUT_REPORTER : public REPORTER
{
public:
    STDOUT_REPORTER()
    { }

    virtual ~STDOUT_REPORTER()
    { }

    static REPORTER& GetInstance();

    REPORTER& Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
};


class KICOMMON_API WXLOG_REPORTER : public REPORTER
{
public:
    WXLOG_REPORTER()
    { }

    virtual ~WXLOG_REPORTER()
    { }

    static REPORTER& GetInstance();

    REPORTER& Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
};


class KICOMMON_API LOAD_INFO_REPORTER : public REPORTER
{
public:
    LOAD_INFO_REPORTER()
    { }

    virtual ~LOAD_INFO_REPORTER()
    { }

    static LOAD_INFO_REPORTER& GetInstance();

    REPORTER& Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    void SetRedirectTarget( REPORTER* aReporter );
    REPORTER* GetRedirectTarget() const;

private:
    REPORTER* m_redirectTarget = nullptr;
};


class KICOMMON_API LOAD_INFO_REPORTER_SCOPE
{
public:
    explicit LOAD_INFO_REPORTER_SCOPE( REPORTER* aReporter );
    ~LOAD_INFO_REPORTER_SCOPE();

private:
    LOAD_INFO_REPORTER& m_reporter;
    REPORTER*           m_previousReporter;
};


/**
 * RAII class to set and restore the fontconfig reporter.
 *
 * Ensures the fontconfig reporter is properly reset even if an exception occurs
 * during file loading operations.
 */
class KICOMMON_API FONTCONFIG_REPORTER_SCOPE
{
public:
    explicit FONTCONFIG_REPORTER_SCOPE( REPORTER* aReporter );
    ~FONTCONFIG_REPORTER_SCOPE();

private:
    REPORTER* m_previousReporter;
};


class KICOMMON_API REDIRECT_REPORTER : public REPORTER
{
public:
    REDIRECT_REPORTER( REPORTER* aRedirectTarget ) : m_redirectTarget( aRedirectTarget ) {}

    REPORTER& Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    REPORTER* m_redirectTarget;
};


/**
 * A wrapper for reporting to a specific text location in a statusbar.
 */
class KICOMMON_API STATUSBAR_REPORTER : public REPORTER
{
public:
    STATUSBAR_REPORTER( wxStatusBar* aStatusBar, int aPosition = 0 )
            : REPORTER(),
              m_statusBar( aStatusBar ),
              m_position( aPosition )
    { }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

private:
    wxStatusBar* m_statusBar;
    int          m_position;
};

#endif     // _REPORTER_H_
