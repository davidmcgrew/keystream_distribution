# Makefile for keystream_distribution utility
#

CXX      = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS  = -ltbb

keystream_distribution: keystream_distribution.cc rc4.h progress_bar.h
	$(CXX) $(CXXFLAGS) keystream_distribution.cc -o keystream_distribution $(LDFLAGS)

clean:
	rm keystream_distribution



