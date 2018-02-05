SFML_INCLUDE = "SFML-2.4.2/include"
SFML_LIB = "SFML-2.4.2/lib"

all: main.cpp
	g++ -o CPFMaker  -I $(SFML_INCLUDE) -L $(SFML_LIB) -std=c++11 -Wall main.cpp -lsfml-graphics -lsfml-system -lsfml-window
