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

#include <boost/test/unit_test.hpp>

#include <frame_type.h>
#include <invoke_pcb_dialog.h>
#include <kiway.h>
#include <mail_type.h>

#include <map>
#include <vector>


namespace
{

class PRELOAD_KIFACE : public KIFACE
{
public:
    bool OnKifaceStart( PGM_BASE*, int, KIWAY* ) override { return true; }
    void OnKifaceEnd() override {}
    void Reset() override {}
    wxWindow* CreateKiWindow( wxWindow*, int, KIWAY*, int ) override { return nullptr; }
    void* IfaceOrAddress( int ) override { return nullptr; }
    void GetActions( std::vector<TOOL_ACTION*>& ) const override {}
    void PreloadLibraries( KIWAY* ) override { m_preloaded = true; }

    bool m_preloaded = false;
};


class RECORDING_KIWAY : public KIWAY
{
public:
    RECORDING_KIWAY() :
            KIWAY( KFCTL_STANDALONE )
    {
    }

    KIFACE* KiFACE( FACE_T aFaceId, bool ) override { return &m_faces[aFaceId]; }

    void ExpressMail( FRAME_T aDestination, MAIL_T aCommand, std::string&, wxWindow*, bool aFromOtherThread ) override
    {
        if( aCommand == MAIL_RELOAD_LIB && !aFromOtherThread )
            m_reloaded.push_back( aDestination );
    }

    std::map<FACE_T, PRELOAD_KIFACE> m_faces;
    std::vector<FRAME_T>             m_reloaded;
};

} // namespace


BOOST_AUTO_TEST_SUITE( FootprintLibTableReload )


// A synchronous reload of CvPcb holds the UI thread until every footprint library has loaded
BOOST_AUTO_TEST_CASE( CvpcbWaitsForThePreload )
{
    RECORDING_KIWAY kiway;

    ReloadFootprintLibraries( &kiway );

    BOOST_CHECK( kiway.m_faces[KIWAY::FACE_PCB].m_preloaded );

    for( FRAME_T frame : kiway.m_reloaded )
        BOOST_CHECK_NE( frame, FRAME_CVPCB );
}


BOOST_AUTO_TEST_SUITE_END()
