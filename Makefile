TARGET = bin/final
SRC = $(wildcard src/srv/*.c)
OBJ = $(patsubst src/srv/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -n -p 8080 -f mydb.db


default: $(TARGET)

$(TARGET): $(OBJ)
	gcc -o $@ $^

obj/%.o: src/srv/%.c
	gcc -c $< -o $@ -Iinclude

clean:
	rm -f obj/%.o 
	rm -f bin/*
	rm -f *.db