CC=g++ -w
CFLAGS=-I.
OPCVFLAGS=`pkg-config --cflags --libs opencv`

wazeread: main.cpp tmatch.cpp
	$(CC) main.cpp tmatch.cpp -o wazeread $(CFLAGS) $(OPCVFLAGS)
