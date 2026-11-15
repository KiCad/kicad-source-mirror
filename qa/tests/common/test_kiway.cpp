/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cli/exit_codes.h>
#include <kiway.h>

#include <wx/log.h>


namespace
{

// Stands in for an install that is missing one kiface
class MISSING_DSO_KIWAY : public KIWAY
{
public:
    MISSING_DSO_KIWAY() :
            KIWAY( KFCTL_CPP_PROJECT_SUITE )
    {
    }

protected:
    const wxString dso_search_path( FACE_T aFaceId ) override
    {
        return wxS( "/nonexistent/_missing.kiface" );
    }
};


// wxLogBuffer would flush to a message box on some platforms
class CAPTURE_LOG : public wxLog
{
public:
    wxString m_text;

protected:
    void DoLogTextAtLevel( wxLogLevel aLevel, const wxString& aMsg ) override { m_text += aMsg; }
};

} // namespace


BOOST_AUTO_TEST_SUITE( KiwayLoad )


BOOST_AUTO_TEST_CASE( MissingKifaceIsReportedNotThrown )
{
    MISSING_DSO_KIWAY kiway;
    CAPTURE_LOG       log;
    wxLog*            prevLog = wxLog::SetActiveTarget( &log );
    KIFACE*           kiface = nullptr;
    int               exitCode = CLI::EXIT_CODES::SUCCESS;

    BOOST_CHECK_NO_THROW( kiface = kiway.KiFACE( KIWAY::FACE_SCH ) );
    BOOST_CHECK_NO_THROW( exitCode = kiway.ProcessJob( KIWAY::FACE_SCH, nullptr ) );

    wxLog::SetActiveTarget( prevLog );

    BOOST_CHECK( kiface == nullptr );
    BOOST_CHECK_EQUAL( exitCode, CLI::EXIT_CODES::ERR_UNKNOWN );
    BOOST_CHECK( log.m_text.Contains( wxS( "Failed to load kiface library" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
