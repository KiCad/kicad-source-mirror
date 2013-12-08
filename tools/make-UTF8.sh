

WXCONFIG=wx-config
#WXCONFIG=/opt/wx2.9/bin/wx-config

g++ -g $($WXCONFIG --cppflags) UTF8.cpp -o test  $($WXCONFIG --libs)

