parser: parser.o
	g++ -std=c++17 -Wall parser.o -o parser

parser.o: parser.cpp stb_image_write.h
	g++ -std=c++17 -Wall -c parser.cpp

clean:
	rm -f *.o parser