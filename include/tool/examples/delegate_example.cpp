#include <cstdio>
#include <string>

#include <tool/delegate.h>

class MyClass
{
public:
    int MyMethod( const string& arg )
    {
        return arg.length();
    }
};

typedef DELEGATE<int, const string&> MyDelegate;

main()
{
    MyClass t1;
    MyClass t2;

    MyDelegate ptr1( &t1, &MyClass::MyMethod );
    MyDelegate ptr2( &t2, &MyClass::MyMethod );

    int retval1, retval2;

    retval1 = ptr1( "apples" );
    retval2 = ptr2( "cherries" );

    return 0;
}
