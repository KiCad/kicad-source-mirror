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

#ifndef __DRC_PROTO_H
#define __DRC_PROTO_H

#include <string>

#include <widgets/progress_reporter_base.h>
#include <reporter.h>
#include <optional>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>

class BOARD;

class CONSOLE_LOG
{
public:
    enum COLOR {
        RED = 0,
        GREEN,
        DEFAULT
    };

    CONSOLE_LOG() {};

    void PrintProgress( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf("%s", (const char *) aMessage.c_str() );
        fflush(stdout);

        m_lastLineIsProgressBar = true;
    }


    void Print( const wxString& aMessage )
    {
        if( m_lastLineIsProgressBar )
            eraseLastLine();

        printf("%s", (const char *) aMessage.c_str() );
        fflush(stdout);

        m_lastLineIsProgressBar = false;
    }


    void SetColor( COLOR color )
    {
        std::map<COLOR, wxString> colorMap =
        {
            { RED, "\033[0;31m" },
            { GREEN, "\033[0;32m" },
            { DEFAULT, "\033[0;37m" }
        };

        printf( "%s", (const char*) colorMap[ color ].c_str() );
        fflush(stdout);
    }


private:
    void eraseLastLine()
    {
        printf("\r\033[K");
        fflush(stdout);
    }

    bool m_lastLineIsProgressBar = false;
    std::mutex m_lock;
};

class CONSOLE_PROGRESS_REPORTER : public PROGRESS_REPORTER_BASE
{
public:
    CONSOLE_PROGRESS_REPORTER( CONSOLE_LOG* log ) :
        PROGRESS_REPORTER_BASE( 0 ),
        m_log( log ) {};
    ~CONSOLE_PROGRESS_REPORTER() {};

    virtual void SetCurrentProgress( double aProgress ) override
    {
        PROGRESS_REPORTER_BASE::SetCurrentProgress( aProgress );
        updateUI();
    }

private:
    virtual bool updateUI() override
    {
        m_log->SetColor( CONSOLE_LOG::GREEN );
        m_log->PrintProgress( wxString::Format( "      | %s : %.02f%%",
                                                m_rptMessage,
                                                (double) m_progress / (double) m_maxProgress *
                                                100.0 ) );
        return true;
    }

    CONSOLE_LOG* m_log;
};

class CONSOLE_MSG_REPORTER : public REPORTER
{
public:
    CONSOLE_MSG_REPORTER( CONSOLE_LOG *log ) :
        m_log(log)
        {};
    ~CONSOLE_MSG_REPORTER() {};


    virtual REPORTER& Report( const wxString& aText,
                              SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        switch( aSeverity )
        {
            case RPT_SEVERITY_ERROR:
                m_log->SetColor( CONSOLE_LOG::RED );
                m_log->Print("ERROR | ");
                break;

            default:
                m_log->SetColor( CONSOLE_LOG::DEFAULT );
                m_log->Print("      | ");
        }

        m_log->SetColor( CONSOLE_LOG::DEFAULT );
        m_log->Print( aText + "\n" );
        return *this;
    }

    virtual bool HasMessage() const override
    {
        return true;
    }

private:
    CONSOLE_LOG* m_log;
};


struct PROJECT_CONTEXT
{
    PROJECT* project;
    wxString rulesFilePath;
    std::shared_ptr<BOARD> board;
    std::shared_ptr<NETLIST> netlist;
};

PROJECT_CONTEXT loadKicadProject( const wxString& filename, std::optional<wxString> rulesFilePath );

int runDRCProto( PROJECT_CONTEXT project,
                 std::shared_ptr<KIGFX::VIEW_OVERLAY> aDebugOverlay = nullptr );

#endif


