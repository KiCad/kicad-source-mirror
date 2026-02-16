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

#include "allegro_test_utils.h"

#include <board.h>
#include <pcb_io/allegro/pcb_io_allegro.h>

#include <pcbnew_utils/board_file_utils.h>


std::ostream& ALLEGRO::boost_test_print_type( std::ostream& os, FMT_VER const& aFmtVer )
{
    os << static_cast<int>( aFmtVer );
    return os;
};


static std::string getBoardsDataDir()
{
    return KI_TEST::GetPcbnewTestDataDir() + "plugins/allegro/boards/";
}


std::string KI_TEST::AllegroBoardDataDir( const std::string& aBoardName )
{
    if( aBoardName.empty() )
    {
        return getBoardsDataDir();
    }

    return getBoardsDataDir() + aBoardName + "/";
}


std::string KI_TEST::AllegroBoardFile( const std::string& aFileName )
{
    return getBoardsDataDir() + aFileName;
}


KI_TEST::ALLEGRO_CACHED_LOADER::ALLEGRO_CACHED_LOADER() :
        m_allegroPlugin( std::make_unique<PCB_IO_ALLEGRO>() )
{
}


KI_TEST::ALLEGRO_CACHED_LOADER::~ALLEGRO_CACHED_LOADER()
{
}


KI_TEST::ALLEGRO_CACHED_LOADER& KI_TEST::ALLEGRO_CACHED_LOADER::GetInstance()
{
    static ALLEGRO_CACHED_LOADER s_AllegroBoardCache;
    return s_AllegroBoardCache;
}


BOARD* KI_TEST::ALLEGRO_CACHED_LOADER::GetCachedBoard( const std::string& aFilePath )
{
    return getCachedBoard( *m_allegroPlugin, aFilePath );
}
