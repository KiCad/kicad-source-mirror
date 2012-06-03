
#include <base_struct.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <deque>
#include <dlist.h>
#include <time.h>
#include <common.h>

#define TEST_NODES      100000000


//typedef std::vector<EDA_ITEM*>       EDA_ITEMV;
//typedef std::deque<EDA_ITEM*>        EDA_ITEMV;
typedef boost::ptr_vector<EDA_ITEM>  EDA_ITEMV;

class MY_ITEM : public  EDA_ITEM
{
public:

    MY_ITEM( KICAD_T id ) :
        EDA_ITEM( id )
    {}


#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const
    {
        ShowDummy( os );
    }
#endif
};


void heap_warm_up();

int main( int argc, char** argv )
{
    EDA_ITEMV       v;
    DLIST<EDA_ITEM> dlist;

    unsigned vAllocStart;
    unsigned vAllocStop;
    unsigned vIterateStart;
    unsigned vIterateStop;

    unsigned dAllocStart;
    unsigned dAllocStop;
    unsigned dIterateStart;
    unsigned dIterateStop;

    heap_warm_up();

    vAllocStart = GetRunningMicroSecs();

    for( int i=0;  i<TEST_NODES;  ++i )
    {
        v.push_back( new MY_ITEM( NOT_USED ) );
    }

    vAllocStop = GetRunningMicroSecs();
    vIterateStart = vAllocStop;

    for( EDA_ITEMV::const_iterator it = v.begin();  it != v.end();  ++it )
    {
        if( it->Type() == -22 )
        {
            printf( "never this\n" );
            break;
        }
    }

    vIterateStop = GetRunningMicroSecs();

#if 0
    for( int i=0; i<TEST_NODES;  ++i )
    {
        delete v[i];
    }
#endif

    v.clear();


    dAllocStart = GetRunningMicroSecs();

    for( int i=0;  i<TEST_NODES;  ++i )
    {
        dlist.PushBack( new MY_ITEM( NOT_USED ) );
    }

    dAllocStop = GetRunningMicroSecs();
    dIterateStart = dAllocStop;

    for( const EDA_ITEM* it = dlist;  it;  it = it->Next() )
    {
        if( it->Type() == -22 )
        {
            printf( "never this\n" );
            break;
        }
    }

    dIterateStop = GetRunningMicroSecs();

    printf( "vector alloc: %u usecs  iterate: %u usecs\n",
            vAllocStop - vAllocStart,
            vIterateStop - vIterateStart );

    printf( "dlist alloc: %u usecs  iterate: %u usecs\n",
            dAllocStop - dAllocStart,
            dIterateStop - dIterateStart );
}


void heap_warm_up()
{
    // dry run allocate enough object for process to obtain all memory needed

    EDA_ITEMV  vec;

    for( int i=0; i<TEST_NODES;  ++i )
    {
        vec.push_back( new MY_ITEM( NOT_USED ) );
    }

    for( int i=0; i<TEST_NODES;  ++i )
    {
        // delete vec[i];
    }

    vec.clear();
}
