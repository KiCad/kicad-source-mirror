

#include <string>
#include <core/utf8.h>
#include <wx/string.h>


void callee( const wxString& aString )
{
    UTF8 arg = aString;

    printf( "%s: '%s'\n", __func__, arg.c_str() );
}


int main( int argc, char** argv )
{
    UTF8        bozo = "This is a test of UTF-8: ü‱☺😕😱";

    callee( bozo );

    wxString s = bozo;

    UTF8 b = s;

    if( s.IsEmpty() )
    {
        printf( "string is empty\n" );
    }

    if(  s != bozo.wx_str() )
    {
        printf( "wxString conversion error\n" );
    }

    if( b != bozo )
    {
        printf( "From string conversion error\n" );
    }

    auto pos = bozo.begin();
    auto end = bozo.end();

    while( pos != end )
    {
        printf( "%c", *pos++ );
    }

    printf("\n");

    return 0;
}
