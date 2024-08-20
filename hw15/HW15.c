#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s <database> <table> <column>\n", prog_name);
}

static int callback(void* unused, int argc, char** argv, char** col_name)
{
    for(int i = 0; i < argc; i++)
    printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
    printf("\n");
    return 0;
}

static const char* create_query = "CREATE TABLE IF NOT EXISTS oscar (id INTEGER, year INTEGER, age INTEGER, name TEXT, movie TEXT);";

int main(int argc, char *argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *db_name = argv[1];
    const char *table_name = argv[2];
    const char *column_name = argv[3];

    sqlite3 *db;
    char *err_msg = 0;
    sqlite3_stmt *stmt;
    char sql[256];
    
    // Открытие базы данных
    int rc = sqlite3_open(db_name, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_exec(db, create_query, callback, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg); 
    }

    // Формирование SQL-запроса
    snprintf(sql, sizeof(sql), "SELECT %s FROM %s;", column_name, table_name);
    // Подготовка SQL-запроса
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute query: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // Переменные для расчета статистики
    double sum = 0.0, sum_sq = 0.0;
    double min = __DBL_MAX__, max = -__DBL_MAX__;
    int count = 0;

    // Обработка строк результата
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            continue;  // Пропуск NULL значений
        }

        if (sqlite3_column_type(stmt, 0) != SQLITE_FLOAT && sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
            fprintf(stderr, "Non-numeric data encountered in column %s\n", column_name);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        double value = sqlite3_column_double(stmt, 0);

        sum += value;
        sum_sq += value * value;
        if (value < min) min = value;
        if (value > max) max = value;
        count++;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    // Вычисление статистики
    double mean = sum / count;
    double variance = (sum_sq - (sum * sum) / count) / count;

    // Вывод результатов
    printf("Statistics for column '%s':\n", column_name);
    printf("Mean: %f\n", mean);
    printf("Sum: %f\n", sum);
    printf("Min: %f\n", min);
    printf("Max: %f\n", max);
    printf("Variance: %f\n", variance);

    // Очистка ресурсов
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}
