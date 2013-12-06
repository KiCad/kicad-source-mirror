
#include <stdio.h>
#include <string>
#include <wx/string.h>
#include <assert.h>


/**
 * Class UTF8
 * is an 8 bit std::string that is assuredly encoded in UTF8, and supplies special
 * conversion support to and from wxString, and has iteration over unicode characters.
 *
 * @author Dick Hollenbeck
 */
class UTF8 : public std::string
{
public:

    UTF8( const wxString& o );

    /// This is the only constructor for which you could end up with
    /// non-UTF8 encoding, but that would be your fault.
    UTF8( const char* txt ) :
        std::string( txt )
    {
    }

    explicit UTF8( const std::string& o ) :
        std::string( o )
    {
    }

    UTF8() :
        std::string()
    {
    }

    UTF8& operator=( const wxString& o );

    UTF8& operator=( const std::string& o )
    {
        std::string::operator=( o );
        return *this;
    }

    operator wxString () const;

    /// This one is not in std::string, and one wonders why... might be a solid
    /// enough reason to remove it still.
    operator char* () const
    {
        return (char*) c_str();
    }

    /**
     * Function uni_forward
     * advances over a UTF8 encoded multibyte character, capturing the unicode
     * character as it goes, and returning the number of bytes consumed.
     *
     * @param aSequence is the UTF8 byte sequence.
     * @param aResult is where to put the unicode character.
     */
    static int uni_forward( unsigned char* aSequence, unsigned* aResult )
    {
        // @todo: have this read UTF8 characters into result, not bytes.
        // What's here now is scaffolding, reading single byte characters only.
        *aResult = *aSequence;
        return 1;
    }

    /**
     * class uni_iter
     * is a non-mutable iterator that walks through code points in the UTF8 encoded
     * string.  The normal ++(), ++(int), ->(), and *() operators are all supported and
     * they return a unsigned holding the unicode character appropriate for respective
     * operation.
     */
    class uni_iter
    {
        friend class UTF8;

        unsigned char* it;

        uni_iter( const char* start ) :
            it( (unsigned char*) start )
        {
            assert( sizeof(unsigned) >= 4 );
        }

    public:

        /// pre-increment and return unicode at new position
        unsigned operator++()
        {
            unsigned    result;

            // advance, and toss the result
            it += uni_forward( it, &result );

            // get the next result, but do not advance:
            uni_forward( it, &result );
            return result;
        }

        /// post-increment and return unicode at initial position
        unsigned operator++( int )
        {
            unsigned    result;

            // grab the result and advance.
            it += uni_forward( it, &result );
            return result;
        }

        /// return unicode at current position
        unsigned operator->() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        /// return unicode at current position
        unsigned operator*() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        bool operator==( const uni_iter& other ) const  { return it == other.it; }
        bool operator!=( const uni_iter& other ) const  { return it != other.it; }
        bool operator< ( const uni_iter& other ) const  { return it <  other.it; }
        bool operator<=( const uni_iter& other ) const  { return it <= other.it; }
        bool operator> ( const uni_iter& other ) const  { return it >  other.it; }
        bool operator>=( const uni_iter& other ) const  { return it >= other.it; }
    };

    /**
     * Function ubegin
     * returns a @a uni_iter initialized to the start of this UTF8 byte sequence.
     */
    uni_iter ubegin() const
    {
        return uni_iter( data() );
    }

    /**
     * Function uend
     * returns a @a uni_iter initialized to the end of this UTF8 byte sequence.
     */
    uni_iter uend() const
    {
        return uni_iter( data() + size() );
    }
};


wxString wxFunctionTaking_wxString( const wxString& wx )
{
    printf( "%s:'%s'\n", __func__, (char*) UTF8( wx ) );
    printf( "%s:'%s'\n", __func__, (const char*) UTF8( wx ) );
    printf( "%s:'%s'\n", __func__, UTF8( wx ).c_str() );

    return wx;
}


int main()
{
    std::string str = "input";
    UTF8        u1 = "initial";
    wxString    wx = wxT( "input2" );

    printf( "u1:'%s'\n", u1.c_str() );

    u1 = str;

    wxString    wx2 = u1;

    UTF8        u2 = wx2;

    u2 += 'X';

    printf( "u2:'%s'\n", u2.c_str() );

    // key accomplishments here:
    // 1) passing a UTF8 to a function which normally takes a wxString.
    // 2) return a wxString back into a UTF8.
    UTF8    result = wxFunctionTaking_wxString( u2 );

    printf( "result:'%s'\n", result.c_str() );

    // test the unicode iterator:
    for( UTF8::uni_iter it = u2.ubegin();  it != u2.uend();  )
    {
        // test post-increment:
        printf( " _%c_", it++ );

        // after UTF8::uni_forward() is implemented, %c is no longer useable.
        // printf( " _%02x_", it++ );
    }

    printf( "\n" );

    return 0;
}


// These to go into a library *.cpp, they are not inlined so that code space
// is saved creating the intermediate objects and referencing wxConvUTF8.


UTF8::UTF8( const wxString& o ) :
    std::string( (const char*) o.utf8_str() )
{
}


UTF8::operator wxString () const
{
    return wxString( c_str(), wxConvUTF8 );
}


UTF8& UTF8::operator=( const wxString& o )
{
    std::string::operator=( (const char*) o.utf8_str() );
    return *this;
}
