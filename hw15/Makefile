# Название исполняемого файла
TARGET = HW15

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
LIBS = -lsqlite3
DATA = oscar.sql
# Исходные файлы
SRC = HW15.c

# Сборка исполняемого файла
default: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Очистка скомпилированных файлов
clean:
	rm -f $(TARGET)

db_create:
	sqlite3 DataBase < oscar.sql

# Тестирование
test: $(TARGET)
	make db_create
	@echo "Running tests..."
	@./$(TARGET) DataBase oscar age
	@echo "Tests completed."

# Правило для очистки всех временных файлов
.PHONY: default clean test