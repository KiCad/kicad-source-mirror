
#pragma once

#include <memory>

#include <allegro_stream.h>
#include <allegro_pcb_structs.h>

#include <progress_reporter.h>

namespace ALLEGRO
{

/**
 * Class that parses a single FILE_STREAM into a RAW_BOARD,
 * and handles any state involved in that parsing
 *
 * This only handles converting rawfile stream data into
 * structs that represent a near-verbatim representation of the
 * data, with a few small conversions and conveniences.
 */
class PARSER
{
public:
    PARSER( FILE_STREAM& aStream, PROGRESS_REPORTER* aProgressReporter ) :
        m_stream( aStream ),
        m_progressReporter( aProgressReporter )
    {

    }

    /**
     * When set to true, the parser will stop at the first unknown block, rather
     * than throwing an error.
     *
     * This is mostly useful for debugging, as at least you can dump the blocks
     * and see what they are. But in real life, this would result in a very incomplete
     * board state.
     */
    void EndAtUnknownBlock( bool aEndAtUnknownBlock )
    {
        m_endAtUnknownBlock = aEndAtUnknownBlock;
    }

    std::unique_ptr<RAW_BOARD> Parse();

private:
    void readObjects( RAW_BOARD& aBoard );

    FILE_STREAM&       m_stream;
    bool               m_endAtUnknownBlock = false;
    PROGRESS_REPORTER* m_progressReporter = nullptr;
};

} // namespace ALLEGRO
