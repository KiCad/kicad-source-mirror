

#include <string>
#include <utf8.h>
#include <wx/string.h>


void callee( const wxString& aString )
{
    UTF8 arg = aString;

    printf( "%s: '%s'\n", __func__, arg.c_str() );
}


int main( int argc, char** argv )
{
    UTF8        bozo = "ü";

    callee( bozo );

    wxString s = bozo;

    wxString b = bozo;

    if( s.IsEmpty() )
    {
        printf( "string is empty\n" );
    }

    if(  s != bozo.wx_str() )
    {
        printf( "string miscompare\n" );
    }

    return 0;
}
