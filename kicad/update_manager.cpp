/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <update_manager.h>
#include <pgm_base.h>

#include <string>
#include <sstream>

#include "settings/settings_manager.h"
#include "settings/kicad_settings.h"
#include <notifications_manager.h>

#include <kicad_curl/kicad_curl.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <progress_reporter.h>

#include <dialogs/dialog_update_notice.h>

#include <json_common.h>
#include <core/json_serializers.h>

#include <wx/log.h>
#include <wx/event.h>
#include <wx/filefn.h>
#include <wx/translation.h>
#include <wx/notifmsg.h>

#include <background_jobs_monitor.h>

#include <thread_pool.h>

#include <build_version.h>


struct UPDATE_REQUEST
{
    wxString platform;
    wxString arch;
    wxString current_version;
    wxString lang;
    wxString last_check;
};


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( UPDATE_REQUEST, platform, arch, current_version, lang,
                                    last_check )

struct UPDATE_RESPONSE
{
    wxString version;
    wxString release_date;
    wxString details_url;
    wxString downloads_url;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( UPDATE_RESPONSE, version, release_date, details_url,
                                    downloads_url )

#define UPDATE_QUERY_ENDPOINT wxS( "https://downloads.kicad.org/api/v1/update" )


UPDATE_MANAGER::UPDATE_MANAGER() : m_working( false )
{
}


UPDATE_MANAGER::~UPDATE_MANAGER()
{
    if( m_updateBackgroundJob )
    {
        if( m_updateBackgroundJob->m_reporter )
            m_updateBackgroundJob->m_reporter->Cancel();

        if( m_updateTask.valid() )
            m_updateTask.wait();
    }
}


int UPDATE_MANAGER::PostRequest( const wxString& aUrl, std::string aRequestBody,
                                  std::ostream* aOutput, PROGRESS_REPORTER* aReporter,
                                  const size_t aSizeLimit )
{
    bool size_exceeded = false;

    TRANSFER_CALLBACK callback = [&]( size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow )
    {
        if( aSizeLimit > 0 && ( dltotal > aSizeLimit || dlnow > aSizeLimit ) )
        {
            size_exceeded = true;

            // Non zero return means abort.
            return true;
        }

        if( aReporter )
        {
            if( dltotal > 1000 )
            {
                aReporter->SetCurrentProgress( dlnow / (double) dltotal );
                aReporter->Report( wxString::Format( _( "Downloading %lld/%lld kB" ), dlnow / 1000,
                                                     dltotal / 1000 ) );
            }
            else
            {
                if( aReporter )
                    aReporter->SetCurrentProgress( 0.0 );
            }

            return !aReporter->KeepRefreshing();
        }
        else
            return false;
    };

    KICAD_CURL_EASY curl;
    curl.SetHeader( "Accept", "application/json" );
    curl.SetHeader( "Content-Type", "application/json" );
    curl.SetHeader( "charset", "utf-8" );
    curl.SetOutputStream( aOutput );
    curl.SetURL( aUrl.ToUTF8().data() );
    curl.SetPostFields( aRequestBody );
    curl.SetFollowRedirects( true );
    curl.SetTransferCallback( callback, 250000L );

    int code = curl.Perform();

    if( aReporter && !aReporter->IsCancelled() )
        aReporter->SetCurrentProgress( 1.0 );

    if( code != CURLE_OK )
    {
        if( aReporter )
        {
            if( code == CURLE_ABORTED_BY_CALLBACK && size_exceeded )
                aReporter->Report( _( "Download is too large." ) );
            else if( code != CURLE_ABORTED_BY_CALLBACK )
                aReporter->Report( wxString( curl.GetErrorText( code ) ) );
        }

        return 0;
    }

    return curl.GetResponseStatusCode();
}


void UPDATE_MANAGER::CheckForUpdate( wxWindow* aNoticeParent )
{
    if( m_working )
        return;

    m_working = false;

    m_updateBackgroundJob = Pgm().GetBackgroundJobMonitor().Create( _( "Update Check" ) );

    auto update_check = [aNoticeParent, this]() -> void
    {
        std::shared_ptr<BACKGROUND_JOB_REPORTER> reporter = m_updateBackgroundJob->m_reporter;

        std::stringstream update_json_stream;
        std::stringstream request_json_stream;

        wxString aUrl = UPDATE_QUERY_ENDPOINT;
        reporter->SetNumPhases( 1 );
        reporter->Report( _( "Requesting update info" ) );

        UPDATE_REQUEST requestContent;


        // These platform keys are specific to the downloads site
#if defined( __WXMSW__ )
        requestContent.platform = "windows";

    #if defined( KICAD_BUILD_ARCH_X64 )
        requestContent.arch = "amd64";
    #elif defined( KICAD_BUILD_ARCH_X86 )
        requestContent.arch = "i686";
    #elif defined( KICAD_BUILD_ARCH_ARM )
        requestContent.arch = "arm";
    #elif defined( KICAD_BUILD_ARCH_ARM64 )
        requestContent.arch = "arm64";
    #endif
#elif defined( __WXOSX__ )
        requestContent.platform = "macos";
        requestContent.arch = "unified";
#else
        //everything else gets lumped as linux
        requestContent.platform = "linux";
        requestContent.arch = "";
#endif
        wxString verString = GetSemanticVersion();
        verString.Replace( "~", "-" ); // make the string valid for semver
        requestContent.current_version = verString;
        requestContent.lang = Pgm().GetLanguageTag();

        KICAD_SETTINGS* settings = GetAppSettings<KICAD_SETTINGS>( "kicad" );

        requestContent.last_check = settings->m_lastUpdateCheckTime;

        nlohmann::json requestJson = nlohmann::json( requestContent );
        request_json_stream << requestJson;

        int responseCode = PostRequest( aUrl, request_json_stream.str(), &update_json_stream,
                                        reporter.get(), 20480 );

        // Check that the response is 200 (content provided)
        // We can also return 204 for no update
        if( responseCode == 200 )
        {
            nlohmann::json  update_json;
            UPDATE_RESPONSE response;

            try
            {
                update_json_stream >> update_json;
                response = update_json.get<UPDATE_RESPONSE>();

                if( response.version != settings->m_lastReceivedUpdate )
                {
                    aNoticeParent->CallAfter(
                            [aNoticeParent, response]()
                            {
                                auto notice = new DIALOG_UPDATE_NOTICE( aNoticeParent,
                                                                        response.version,
                                                                        response.details_url,
                                                                        response.downloads_url );

                                int retCode = notice->ShowModal();

                                if( retCode != wxID_RETRY )
                                {
                                    // basically saving the last received update prevents us from
                                    // prompting again
                                    if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
                                        cfg->m_lastReceivedUpdate = response.version;
                                }
                            } );
                }
            }
            catch( const std::exception& e )
            {
                wxLogError( wxString::Format( _( "Unable to parse update response: %s" ), e.what() ) );
            }
        }

        settings->m_lastUpdateCheckTime = wxDateTime::Now().FormatISOCombined();

        Pgm().GetBackgroundJobMonitor().Remove( m_updateBackgroundJob );
        m_updateBackgroundJob = nullptr;
        m_working = false;
    };

    thread_pool& tp = GetKiCadThreadPool();
    m_updateTask = tp.submit_task( update_check );
}
