#include <iostream>
#include <fstream>
#include <cstdio>
#include <iomanip>
#include <stdint.h>

#include "md5.h"


using namespace std;

int main( int argc, char** argv )
{
    if( argc != 2 )
    {
        cerr << "No filename given\n";
        return 0;
    }

    FILE* fp = fopen( argv[1], "rb" );

    if( !fp )
    {
        cout << "Could not open file '" << argv[1] << "'\n";
        return 0;
    }

    struct md5_ctx msum;
    md5_init_ctx( &msum );
    unsigned char rb[16];
    int res = md5_stream( fp, rb );

    cout << "Result: " << res << "\n";
    cout << "md5sum:\n";
    for( int i = 0; i < 16; ++i )
    {
        cout << setfill( '0' ) << setw(2) << nouppercase << hex << ((int)rb[i] & 0xff);
    }
    cout << "\n";

    fclose( fp );

    /*
    ifstream afile;
    afile.open( argv[1], ios::in | ios::binary | ios::ate );
    streampos fsize;

    if( !afile.is_open() )
    {
        cout << "Could not open file '" << argv[1] << "'\n";
        return 0;
    }

    fsize = afile.tellg();
    afile.seekg( 0, ios::beg );



    afile.close();
    */

    return 0;
}