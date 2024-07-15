/*
    Производится печать массива
    Фильтрация по нахождению печатных чисел 
    Вывод сфильтрованной строки в обратном порядке

    Реализовано, как в main.asm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    int value;
    struct Node* next;
};

struct Node* add_element(struct Node* head, int value) {
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if (!new_node) {
        abort();
    }
    new_node->value = value;
    new_node->next = head;
    return new_node;
}


int data[6] = {4, 8, 15, 16, 23, 42};

struct Node* transfer_array(int* data, int size) {
    struct Node* head = NULL;
    for (int i = size - 1; i >= 0; --i) {
        head = add_element(head, data[i]);
    }
    return head;
}


void print_common(struct Node* head) {
    struct Node* current = head;
    while (current) {
        printf("%d ", current->value);
        current = current->next;
    }
    printf("\r\n");
}

void print_filter(struct Node* head) {
    struct Node* current = head;
    while (current) {
        if (current->value & 1) {
            printf("%d ", current->value);
        }
        current = current->next;
    }
    printf("\r\n");
}

void free_list(struct Node* head) {
    struct Node* current = head;
    struct Node* next;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf("Использование: %s\n", argv[0]);
        return 1;
    }

    struct Node* data_list = transfer_array(data, sizeof(data) / sizeof(data[0]));

    print_common(data_list);

    print_filter(data_list);

    free_list(data_list);

    return 0;
}
