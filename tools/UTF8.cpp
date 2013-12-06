
#include <stdio.h>
#include <string>
#include <wx/string.h>
#include <stdint.h>


/**
 * Class UTF8
 * is an 8 bit std::string assuredly encoded in UTF8 that supplies special
 * conversion support to and from wxString, and has iteration over
 * UTF8 code points.
 */
class UTF8 : public std::string
{

public:

    UTF8( const wxString& o ) :
        std::string( (const char*) o.utf8_str() )
    {
        // @todo: should not be inline.
    }

    UTF8( const char* txt ) :
        std::string( txt )
    {
        // ok inline
    }

    explicit UTF8( const std::string& o ) :
        std::string( o )
    {
        // ok inline
    }

    UTF8() :
        std::string()
    {
        // ok inline
    }

    UTF8& operator = ( const wxString& o )
    {
        // @todo: should not be inline.
        std::string::operator=( (const char*) o.utf8_str() );
        return *this;
    }

    UTF8& operator = ( const std::string& o )
    {
        std::string::operator = ( o );
        return *this;
    }

    operator wxString () const
    {
        // @todo: should not be inline.
        return wxString( c_str(), wxConvUTF8 );
    }

    static int uni_forward( const_iterator it, uint32_t* result )
    {
        // @todo: have this read UTF8 characters into result, not bytes.
        // What's here now is scaffolding, reading single byte characters only.
        *result = (unsigned char) *it;
        return 1;
    }

    /**
     * class uni_iter
     * is a non-mutable iterator that walks through code points in the UTF8 encoded
     * string.  The normal ++(), ++(int), ->(), and *() operators are all supported and
     * they return a uint32_t holding the unicode character appropriate for respective
     * operation.
     */
    class uni_iter : public std::string::const_iterator
    {
        const_iterator  it;

    public:
        uni_iter( const_iterator start ) :
            it( start )
        {
        }

        /// pre-increment and return unicode at new position
        uint32_t operator++()
        {
            uint32_t    result;

            // advance, and toss the result
            it += uni_forward( it, &result );

            // get the next result, but do not advance:
            uni_forward( it, &result );

            return result;
        }

        /// post-increment and return unicode at initial position
        uint32_t operator++( int )
        {
            uint32_t    result;

            // grab the result and advance.
            it += uni_forward( it, &result );
            return result;
        }

        /// return unicode at current position
        uint32_t operator->() const
        {
            uint32_t    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        /// return unicode at current position
        uint32_t operator*() const
        {
            uint32_t    result;

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

    uni_iter ubegin() const
    {
        return uni_iter( begin() );
    }

    uni_iter uend() const
    {
        return uni_iter( end() );
    }
};


wxString aFunctionTaking_wxString( const wxString& wx )
{
    printf( "%s: '%s'\n", __func__, UTF8( wx ).c_str() );

    return wx;
}


int main()
{
    UTF8        u1 = "output";
    std::string str = "input";
    wxString    wx = wxT( "input" );

    u1 = str;

    wxString    wx2 = u1;

    UTF8        u2 = wx2;

    u2 += 'X';

    printf( "utf2:'%s'\n", u2.c_str() );

    // key accomplishments here:
    // 1) passing a UTF8 to a function which normally takes a wxString.
    // 2) return a wxString back into a UTF8.
    UTF8    result = aFunctionTaking_wxString( u2 );

    printf( "result:'%s'\n", result.c_str() );

    // test the unicode iterator:
    for( UTF8::uni_iter it = u2.ubegin();  it != u2.uend();  )
    {
        printf( " _%c_", it++ );

        // after UTF7::uni_forward() is implemented, it++ %c is no longer useable.
        // printf( " _%02x_", it++ );
    }
    printf( "\n" );

    return 0;
}

