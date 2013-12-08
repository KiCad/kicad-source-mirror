
#include <stdio.h>
#include <string>
#include <wx/string.h>
#include <assert.h>


/**
 * Class UTF8
 * is an 8 bit std::string that is assuredly encoded in UTF8, and supplies special
 * conversion support to and from wxString, and has iteration over unicode characters.
 *
 * <p>I've been careful to supply only conversion facilities and not try
 * and duplicate wxString() with many member functions.  In the end it is
 * to be a std::string.  There are multiple ways to create text into a std::string
 * without the need of too many member functions:
 *
 * <ul>
 *  <li>richio.h's StrPrintf()</li>
 *  <li>std::ostringstream.</li>
 * </ul>
 *
 * <p>Because this class used no virtuals, it should be possible to cast any
 * std::string into a UTF8 using this kind of cast: (UTF8 &) without construction
 * or copying being the effect of the cast.  Be sure the source std::string holds
 * UTF8 encoded text before you do that.
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

    /// For use with _() function on wx 2.8:
    UTF8( const wchar_t* txt );

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
     * advances over a single UTF8 encoded multibyte character, capturing the
     * unicode character as it goes, and returning the number of bytes consumed.
     *
     * @param aSequence is the UTF8 byte sequence, must be aligned on start of character.
     * @param aResult is where to put the unicode character, and may be NULL if no interest.
     * @return int - the count of bytes consumed.
     */
    static int uni_forward( const unsigned char* aSequence, unsigned* aResult = NULL );

    /**
     * class uni_iter
     * is a non-muting iterator that walks through unicode code points in the UTF8 encoded
     * string.  The normal ++(), ++(int), ->(), and *() operators are all supported
     * for read only access and they return an unsigned holding the unicode character
     * appropriate for the respective operator.
     */
    class uni_iter
    {
        friend class UTF8;

        const unsigned char* it;

        // private constructor.
        uni_iter( const char* start ) :
            it( (const unsigned char*) start )
        {
            // for the human: assert( sizeof(unsigned) >= 4 );
        }


    public:

        uni_iter( const uni_iter& o )
        {
            it = o.it;
        }

        /// pre-increment and return uni_iter at new position
        const uni_iter& operator++()
        {
            it += uni_forward( it );

            return *this;
        }

        /// post-increment and return uni_iter at initial position
        uni_iter operator++( int )
        {
            uni_iter ret = *this;

            it += uni_forward( it );
            return ret;
        }

        /*
        /// return unicode at current position
        unsigned operator->() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }
        */

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

        /// Since the ++ operators advance more than one byte, this is your best
        /// loop termination test, < end(), not == end().
        bool operator< ( const uni_iter& other ) const  { return it <  other.it; }
        bool operator<=( const uni_iter& other ) const  { return it <= other.it; }
        bool operator> ( const uni_iter& other ) const  { return it >  other.it; }
        bool operator>=( const uni_iter& other ) const  { return it >= other.it; }
    };

    /**
     * Function ubegin
     * returns a @a uni_iter initialized to the start of "this" UTF8 byte sequence.
     */
    uni_iter ubegin() const
    {
        return uni_iter( data() );
    }

    /**
     * Function uend
     * returns a @a uni_iter initialized to the end of "this" UTF8 byte sequence.
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

    UTF8        u0 = L"wide string";
    UTF8        u1 = "initial";
    wxString    wx = wxT( "input2" );

    printf( "u0:'%s'\n", u0.c_str() );
    printf( "u1:'%s'\n", u1.c_str() );

    u1 = str;

    wxString    wx2 = u1;

    // force a std::string into a UTF8, then into a wxString, then copy construct:
    wxString    wx3 = (UTF8&) u1;

    UTF8        u2 = wx2;

    u2 += 'X';

    printf( "u2:'%s'\n", u2.c_str() );

    // key accomplishments here:
    // 1) passing a UTF8 to a function which normally takes a wxString.
    // 2) return a wxString back into a UTF8.
    UTF8    result = wxFunctionTaking_wxString( u2 );

    printf( "result:'%s'\n", result.c_str() );

    // test the unicode iterator:
    for( UTF8::uni_iter it = u2.ubegin();  it < u2.uend();  )
    {
        // test post-increment:
        printf( " _%c_", *it++ );

        // after UTF8::uni_forward() is implemented, %c is no longer useable.
        // printf( " _%02x_", *it++ );
    }

    printf( "\n" );

    UTF8::uni_iter it = u2.ubegin();

    UTF8::uni_iter it2 = it++;

    printf( "post_inc:'%c' should be 'i'\n", *it2 );

    it2 = ++it;

    printf( "pre_inc:'%c' should be 'p'\n", *it2 );

    printf( "u[1]:'%c' should be 'n'\n", u2[1] );

    return 0;
}


/*

    These to go into a library *.cpp, they are not inlined so that significant
    code space is saved by encapsulating the creation of intermediate objects
    and referencing wxConvUTF8.

*/


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


#ifndef THROW_IO_ERROR
 #define THROW_IO_ERROR(x)      // nothing
#endif

// There is no wxWidgets function that does this, because wchar_t is 16 bits
// on windows and wx wants to encode the output in UTF16 for such.

int UTF8::uni_forward( const unsigned char* aSequence, unsigned* aResult )
{
    unsigned ch = *aSequence;

    if( ch < 0x80 )
    {
        if( aResult )
            *aResult = ch;
        return 1;
    }

    const unsigned char* s = aSequence;

    static const unsigned char utf8_len[] = {
        // Map encoded prefix byte to sequence length.  Zero means
        // illegal prefix.  See RFC 3629 for details
        /*
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00-0F
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70-7F
        */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80-8F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B0-BF
        0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0-C1 + C2-CF
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0-DF
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0-EF
        4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F0-F4 + F5-FF
    };

    int len = utf8_len[ *s - 0x80  /* top half of table is missing */ ];

    switch( len )
    {
    default:
    case 0:
        THROW_IO_ERROR( "invalid start byte" );
        break;

    case 2:
        if( ( s[1] & 0xc0 ) != 0x80 )
        {
            THROW_IO_ERROR( "invalid continuation byte" );
        }

        ch =    ((s[0] & 0x1f) << 6) +
                ((s[1] & 0x3f) << 0);

        assert( ch > 0x007F && ch <= 0x07FF );
        break;

    case 3:
        if( (s[1] & 0xc0) != 0x80 ||
            (s[2] & 0xc0) != 0x80 ||
            (s[0] == 0xE0 && s[1] < 0xA0)
            // || (s[0] == 0xED && s[1] > 0x9F)
        )
        {
            THROW_IO_ERROR( "invalid continuation byte" );
        }

        ch =    ((s[0] & 0x0f) << 12) +
                ((s[1] & 0x3f) << 6 ) +
                ((s[2] & 0x3f) << 0 );

        assert( ch > 0x07FF && ch <= 0xFFFF );
        break;

    case 4:
        if( (s[1] & 0xc0) != 0x80 ||
            (s[2] & 0xc0) != 0x80 ||
            (s[3] & 0xc0) != 0x80 ||
            (s[0] == 0xF0 && s[1] < 0x90) ||
            (s[0] == 0xF4 && s[1] > 0x8F) )
        {
            THROW_IO_ERROR( "invalid continuation byte" );
        }

        ch =    ((s[0] & 0x7)  << 18) +
                ((s[1] & 0x3f) << 12) +
                ((s[2] & 0x3f) << 6 ) +
                ((s[3] & 0x3f) << 0 );

        assert( ch > 0xFFFF && ch <= 0x10ffff );
        break;
    }

    if( aResult )
    {
        *aResult = ch;
    }

    return len;
}


UTF8::UTF8( const wchar_t* txt ) :
    // size initial string safely large enough, then shrink to known size later.
    std::string( wcslen( txt ) * 4, 0 )
{
    /*

        "this" string was sized to hold the worst case UTF8 encoded byte
        sequence, and was initialized with all nul bytes. Overwrite some of
        those nuls, then resize, shrinking down to actual size.

        Use the wx 2.8 function, not new FromWChar(). It knows about wchar_t
        possibly being 16 bits wide on Windows and holding UTF16 input.

    */

    int sz = wxConvUTF8.WC2MB( (char*) data(), txt, size() );

    resize( sz );
}

