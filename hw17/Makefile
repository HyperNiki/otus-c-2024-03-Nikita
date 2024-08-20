# Название исполняемого файла
TARGET = HW17

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LIBS = -lGL -lGLU -lglut
# Исходные файлы
SRC = HW17.c

# Сборка исполняемого файла
default: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Очистка скомпилированных файлов
clean:
	rm -f $(TARGET)

# Правило для очистки всех временных файлов
.PHONY: default clean test