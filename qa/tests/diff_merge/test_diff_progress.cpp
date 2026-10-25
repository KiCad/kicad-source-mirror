/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/kicad_differ.h>
#include <ki_exception.h>
#include <widgets/progress_reporter_base.h>

#include <wx/filename.h>


namespace
{

class CANCELLED_REPORTER : public PROGRESS_REPORTER_BASE
{
public:
    CANCELLED_REPORTER() :
            PROGRESS_REPORTER_BASE( 1 )
    {
    }

protected:
    bool updateUI() override { return false; }
};


class PROGRESSING_DIFFER : public KICAD_DIFF::KICAD_DIFFER
{
public:
    KICAD_DIFF::DOCUMENT_DIFF Diff() override
    {
        m_options.progress( 0.2 );
        m_pairedAfterCancel = true;
        return {};
    }

    bool m_pairedAfterCancel = false;
};

} // namespace


BOOST_AUTO_TEST_SUITE( DiffProgress )


BOOST_AUTO_TEST_CASE( CancelStopsTheDiffAtItsNextProgress )
{
    CANCELLED_REPORTER reporter;
    PROGRESSING_DIFFER differ;

    KICAD_DIFF::ReportDiffProgress( differ, &reporter );

    BOOST_CHECK_THROW( differ.Diff(), IO_CANCELLED );
    BOOST_CHECK( !differ.m_pairedAfterCancel );
}


// A downloaded version sits in a temporary folder that names the user
BOOST_AUTO_TEST_CASE( LoadingMessageNamesTheFileAndVersionButNotTheFolder )
{
    wxFileName file( wxFileName::GetTempDir(), wxS( "vme-wren.kicad_pcb" ) );

    file.AppendDir( wxS( "older" ) );

    BOOST_CHECK_EQUAL( KICAD_DIFF::LoadingMessage( file.GetFullPath(), wxS( "v213d641c" ) ).ToStdString(),
                       "Loading vme-wren.kicad_pcb (v213d641c)..." );

    // Versions labels already name the file
    BOOST_CHECK_EQUAL( KICAD_DIFF::LoadingMessage( file.GetFullPath(), wxS( "vme-wren.kicad_pcb v1" ) ).ToStdString(),
                       "Loading vme-wren.kicad_pcb v1..." );
}

BOOST_AUTO_TEST_SUITE_END()
