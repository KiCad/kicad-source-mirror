
#include <stdio.h>
#include <string>
#include <wx/string.h>


/**
 * Class UTF8
 * is an 8 bit std::string assuredly encoded in UTF8 that supplies special
 * conversion support to and from wxString.
 */
class UTF8 : public std::string
{

public:

    UTF8( const wxString& o ) :
        std::string( (const char*) o.utf8_str() )
    {
    }

    UTF8( const char* txt ) :
        std::string( txt )
    {
    }

    UTF8( const std::string& o ) :
        std::string( o )
    {
    }

    UTF8() :
        std::string()
    {
    }

    UTF8& operator = ( const wxString& o )
    {
        std::string::operator=( (const char*) o.utf8_str() );
    }

    operator wxString () const
    {
        return wxString( c_str(), wxConvUTF8 );
    }
};


void aFunctionTaking_wxString( const wxString& wx )
{
    printf( "%s: '%s'\n", __func__, UTF8( wx ).c_str() );
}


int main()
{
    UTF8        utf;
    std::string str = "input";
    wxString    wx = wxT( "input" );

    utf = str;

    wxString    wx2 = utf;

    UTF8        utf2 = wx2;

    printf( "here is some text:%s\n", utf2.c_str() );

    // this is the key accomplishment here, passing a UTF8 to a function taking wxString:
    aFunctionTaking_wxString( utf2 );

    return 0;
}
