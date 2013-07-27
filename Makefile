CXX=g++
CXXFLAGS=-std=c++11

imdtest: main.o
	$(CXX) $(CFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
