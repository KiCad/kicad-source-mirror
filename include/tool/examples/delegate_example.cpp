#include <cstdio>
#include <string>

#include <tool/delegate.h>

class MyClass
{
public:
    int MyMethod( const string& arg )
    {
        printf( "MyClass(this = %p)::MyMethod() called with string '%s', length %d\n", this,
                arg.c_str(), arg.length() );
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

    printf( "Object 1 returned %d, object 2 returned %d\n", retval1, retval2 );
    return 0;
}
