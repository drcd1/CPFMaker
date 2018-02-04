SFML_INCLUDE = "C:/SFML-2.4.2/include"
SFML_LIB = "C:/SFML-2.4.2/lib"

g++ -o a.exe -I $(SFML_INCLUDE) -L $(SFML_LIB) main.cpp -lsfml-graphics -lsfml-system -lsfml-window
