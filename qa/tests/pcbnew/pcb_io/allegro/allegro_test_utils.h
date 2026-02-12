

#pragma once


#include <ostream>

#include <convert/allegro_pcb_structs.h>


namespace ALLEGRO
{
std::ostream& boost_test_print_type( std::ostream& os, FMT_VER const& aFmtVer )
{
    os << static_cast<int>( aFmtVer );
    return os;
};
} // namespace ALLEGRO
