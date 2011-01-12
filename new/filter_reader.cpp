

#include <richio.h>
#include <string.h>


/**
 * Class FILTER_READER
 * reads lines of text from another LINE_READER, but only returns non-comment
 * lines and non-blank lines from its ReadLine() function.
 */
class FILTER_READER : public LINE_READER
{
    LINE_READER&  reader;

public:

    /**
     * Constructor ( LINE_READER& )
     * does not take ownership over @a aReader, so will not destroy it.
     */
    FILTER_READER( LINE_READER& aReader ) :
       reader( aReader )
    {
    }

    unsigned ReadLine() throw( IO_ERROR )
    {
        unsigned ret;

        while( ( ret = reader.ReadLine() ) != 0 )
        {
            if( !strchr( "#\n\r", reader[0] ) )
                break;
        }
        return ret;
    }

    const wxString& GetSource() const
    {
        return reader.GetSource();
    }

    char* Line() const
    {
        return reader.Line();
    }

    unsigned LineNumber() const
    {
        return reader.LineNumber();
    }

    unsigned Length() const
    {
        return reader.Length();
    }
};

