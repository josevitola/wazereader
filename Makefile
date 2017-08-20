CC=g++
CFLAGS=-I.
OPCVFLAGS=`pkg-config --cflags --libs opencv`

mapRead: main.cpp tmatch.cpp
	$(CC) main.cpp tmatch.cpp -o wazeRead $(CFLAGS) $(OPCVFLAGS)
