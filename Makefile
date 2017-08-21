CC=g++
CFLAGS=-I.
OPCVFLAGS=`pkg-config --cflags --libs opencv`

wazeRead: main.cpp tmatch.cpp
	$(CC) main.cpp tmatch.cpp -o wazeRead $(CFLAGS) $(OPCVFLAGS)
