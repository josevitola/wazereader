CC=g++ -w
CFLAGS=-I.
OPCVFLAGS=`pkg-config --cflags --libs opencv`

wazeread: main.cpp
	$(CC) main.cpp -o wazeread $(CFLAGS) $(OPCVFLAGS)
