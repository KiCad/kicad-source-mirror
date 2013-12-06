WXCONFIG=wx-config
INCLUDE=/usr/include/wx-2.8

g++ -I $INCLUDE $($WXCONFIG --cppflags) UTF8.cpp -o test  $($WXCONFIG --libs)

