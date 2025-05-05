
#pragma once

#include <allegro_stream.h>

namespace ALLEGRO
{

/**
 * Class that parses a single FILE_STREAM into a RAW_BOARD,
 * and handles any state involved in parsing.
 */
class PARSER
{
public:
    PARSER( FILE_STREAM& aStream ) :
        m_stream( aStream )
    {}

    std::unique_ptr<RAW_BOARD> Parse();

private:
    void readObjects( RAW_BOARD& aBoard );

    FILE_STREAM& m_stream;
};

} // namespace ALLEGRO
