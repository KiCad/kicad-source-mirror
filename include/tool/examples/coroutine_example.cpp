#include <cstdio>
#include <string>

#include <tool/coroutine.h>

typedef COROUTINE<int, int> MyCoroutine;

class MyClass
{
public:
    int CountTo( int n )
    {
        printf( "%s: Coroutine says hi. I will count from 1 to %d and yield each value.\n",
                __FUNCTION__,
                n );

        for( int i = 1; i <= n; i++ )
        {
            printf( "%s: Yielding %d\n", __FUNCTION__, i );
            cofunc.Yield( i );
        }
    }

    void Run()
    {
        cofunc = MyCoroutine( this, &MyClass::CountTo );
        printf( "%s: Calling coroutine that will count from 1 to 5.\n", __FUNCTION__ );
        cofunc.Call( 5 );

        while( cofunc.Running() )
        {
            printf( "%s: Got value: %d\n", __FUNCTION__, cofunc.ReturnValue() );
            cofunc.Resume();
        }

        printf( "%s: Done!\n", __FUNCTION__ );
    }

    MyCoroutine cofunc;
};


main() {
    MyClass obj;

    obj.Run();

    return 0;
}
