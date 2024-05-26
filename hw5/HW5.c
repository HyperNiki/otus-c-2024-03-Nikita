/*
    Производится печать массива
    Фильтрация по нахождению печатных чисел 
    Вывод сфильтрованной строки в обратном порядке

    Реализовано, как в main.asm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int data[6] = {4, 8, 15, 16, 23, 42};

void transfer_array(int* data_in, int* data_out, int size)
{
    int len = size;
    while (len--)
    {
        data_out[len-1] = data_in[len-1];
    }
    
}

void print_common(int* data_in, int size)
{
    for(int i = 0; i < size; i++)
        printf("%i ", data_in[i]);
    printf("\r\n");
}

void print_filter(int* data_in, int size)
{
    while (size--)
        if (data_in[size - 1] & 1)
            printf("%i ", data_in[size - 1]);
    printf("\r\n");
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("Использование: %s>\n", argv[0]);
        return 1;
    }

    int* data_tmp;
    data_tmp = malloc(sizeof(data));
    
    int len = sizeof(data) / sizeof(data[0]);

    transfer_array(data, data_tmp, len);

    print_common(data_tmp, len);

    print_filter(data_tmp, len);

    return 0;
}