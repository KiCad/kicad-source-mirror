CXXFLAGS = -I../.. -I../../boost

all: delegate_example coroutine_example

delegate_example:	delegate_example.cpp
	g++ -o $@ $^ $(CXXFLAGS)
	
coroutine_example:	coroutine_example.cpp
	g++ -o $@ $^ $(CXXFLAGS) -lboost_context