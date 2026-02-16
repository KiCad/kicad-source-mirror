

#pragma once

#include <memory>
#include <ostream>

#include <convert/allegro_pcb_structs.h>
#include <pcb_io/allegro/pcb_io_allegro.h>

#include <pcbnew_utils/board_test_utils.h>


class PCB_IO_ALLEGRO;

namespace ALLEGRO
{

std::ostream& boost_test_print_type( std::ostream& os, FMT_VER const& aFmtVer );

} // namespace ALLEGRO


namespace KI_TEST
{

std::string AllegroBoardDataDir( const std::string& aBoardName );
std::string AllegroBoardFile( const std::string& aFileName );

/**
 * Singleton class to load Allegro boards and cache them in memory, to avoid
 * repeatedly loading and parsing the same board files for multiple tests.
 */
class ALLEGRO_CACHED_LOADER : public CACHED_BOARD_LOADER
{
public:
    ~ALLEGRO_CACHED_LOADER();

    /**
     * Get the singleton instance of the Allegro board cache loader.
     */
    static ALLEGRO_CACHED_LOADER& GetInstance();

    /**
     * Implementation of CACHED_BOARD_LOADER interface, to get a cached board for the given file path, or load it if not already cached.
     * using the PCB_IO_ALLEGRO plugin.
     */
    BOARD* GetCachedBoard( const std::string& aFilePath ) override;

private:
    ALLEGRO_CACHED_LOADER();

    std::unique_ptr<PCB_IO_ALLEGRO> m_allegroPlugin;
};

}; // namespace KI_TEST
