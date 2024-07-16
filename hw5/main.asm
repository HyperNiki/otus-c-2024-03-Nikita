bits 64
extern malloc, puts, printf, fflush, abort
global main

section   .data
empty_str: db 0x0                     ; Пустая строка
int_format: db "%ld ", 0x0            ; Формат для printf
data: dq 4, 8, 15, 16, 23, 42         ; Исходные данные
data_length: equ ($-data) / 8         ; Длина массива data в элементах

section   .text
;;; print_int proc
print_int:                            ; Процедура печати целого числа
    push rbp                          ; Сохраняем базовый указатель
    mov rbp, rsp                      ; Устанавливаем новый базовый указатель
    sub rsp, 16                       ; Выделяем место на стеке

    mov rsi, rdi                      ; Передаем значение числа в rsi
    mov rdi, int_format               ; Передаем формат строки в rdi
    xor rax, rax                      ; Обнуляем rax для printf
    call printf                       ; Вызываем printf

    xor rdi, rdi                      ; Обнуляем rdi для fflush
    call fflush                       ; Вызываем fflush

    mov rsp, rbp                      ; Восстанавливаем стек
    pop rbp                           ; Восстанавливаем базовый указатель
    ret                               ; Возвращаемся из процедуры

;;; p proc
p:                                    ; Процедура, проверяющая, является ли число нечетным
    mov rax, rdi                      ; Копируем rdi в rax
    and rax, 1                        ; Побитовое И с 1 (проверка на нечетность)
    ret                               ; Возвращаемся из процедуры

;;; add_element proc
add_element:                          ; Процедура добавления элемента в связанный список
    push rbp                          ; Сохраняем базовый указатель
    push rbx                          ; Сохраняем rbx
    push r14                          ; Сохраняем r14

    mov rbp, rsp                      ; Устанавливаем новый базовый указатель; rbp = rsp; rsp - вершина стека; Нужно, чтобы восстановить стек
    sub rsp, 16                       ; Выделяем место на стеке

    ; Начало вызова (т.е. без восстановления переменных)
    
    mov r14, rdi                      ; Копируем значение массива в r14
    mov rbx, rsi                      ; Копируем указатель на следующий элемент в rbx; изначально равен 0 из-за rax

    mov rdi, 16                       ; Устанавливаем размер для malloc
    call malloc                       ; Вызываем malloc; Выделение памяти
    test rax, rax                     ; Проверяем результат malloc
    jz abort                          ; Если malloc вернул 0, выходим с ошибкой

    mov [rax], r14                    ; Сохраняем значение в новом узле
    mov [rax + 8], rbx                ; Сохраняем указатель на следующий элемент

    ; Конец вызова

    mov rsp, rbp                      ; Восстанавливаем стек

    pop r14                           ; Восстанавливаем r14
    pop rbx                           ; Восстанавливаем rbx
    pop rbp                           ; Восстанавливаем базовый указатель
    ret                               ; Возвращаемся из процедуры

;;; m proc
m:                                    ; Процедура для применения функции ко всем элементам списка
    push rbp                          ; Сохраняем базовый указатель
    mov rbp, rsp                      ; Устанавливаем новый базовый указатель
    sub rsp, 16                       ; Выделяем место на стеке

    test rdi, rdi                     ; Проверяем, не нулевой ли указатель (конец списка)
    jz outm                           ; Если нулевой, выходим

    push rbp                          ; Сохраняем базовый указатель
    push rbx                          ; Сохраняем rbx

    mov rbx, rdi                      ; Копируем текущий узел в rbx
    mov rbp, rsi                      ; Копируем функцию в rbp

    mov rdi, [rdi]                    ; Получаем значение текущего узла
    call rsi                          ; Вызываем функцию для текущего значения

    mov rdi, [rbx + 8]                ; Переходим к следующему узлу
    mov rsi, rbp                      ; Передаем функцию
    call m                            ; Рекурсивно вызываем m

    pop rbx                           ; Восстанавливаем rbx
    pop rbp                           ; Восстанавливаем базовый указатель

outm:
    mov rsp, rbp                      ; Восстанавливаем стек
    pop rbp                           ; Восстанавливаем базовый указатель
    ret                               ; Возвращаемся из процедуры

;;; f proc
f:                                    ; Процедура фильтрации списка
    mov rax, rsi                      ; Копируем указатель на результат в rax; изаначально 0

    test rdi, rdi                     ; Проверяем, не нулевой ли указатель (конец списка)
    jz outf                           ; Если нулевой, выходим

    push rbx                          ; Сохраняем rbx
    push r12                          ; Сохраняем r12
    push r13                          ; Сохраняем r13

    ; Начало цикла (т.е. без восстановления стека)
    mov rbx, rdi                      ; Копируем текущий узел в rbx
    mov r12, rsi                      ; Копируем указатель на результат в r12
    mov r13, rdx                      ; Копируем функцию проверки в r13

    mov rdi, [rdi]                    ; Получаем значение текущего узла
    call rdx                          ; Вызываем функцию проверки
    test rax, rax                     ; Проверяем результат
    jz z                              ; Если 0, пропускаем добавление

    mov rdi, [rbx]                    ; Получаем значение текущего узла
    mov rsi, r12                      ; Передаем указатель на результат
    call add_element                  ; Добавляем элемент в новый список
    mov rsi, rax                      ; Обновляем указатель на результат
    jmp ff                            ; Переходим к следующему узлу

z:
    mov rsi, r12                      ; Обновляем указатель на результат

ff:
    mov rdi, [rbx + 8]                ; Переходим к следующему узлу
    mov rdx, r13                      ; Передаем функцию проверки
    call f                            ; Рекурсивно вызываем f
;   Конец вызова

    pop r13                           ; Восстанавливаем r13
    pop r12                           ; Восстанавливаем r12
    pop rbx                           ; Восстанавливаем rbx

outf:
    ret                               ; Возвращаемся из процедуры

;;; main proc
main:                                 ; Начало программы
    push rbx                          ; Сохраняем rbx

    xor rax, rax                      ; Обнуляем rax

; Загружаем счётчик цикла   
    mov rbx, data_length              ; Загружаем длину массива данных
; Начало цикла для передачи массива в следующий узел в стеке
adding_loop:
    mov rdi, [data - 8 + rbx * 8]     ; Загружаем значение из массива данных
    mov rsi, rax                      ; Передаем текущий указатель; изначально равен 0
    call add_element                  ; Добавляем элемент в связанный список

    dec rbx                           ; Уменьшаем счетчик
    jnz adding_loop                   ; Повторяем, если не ноль
; Конец цикла

    mov rbx, rax                      ; Сохраняем указатель на голову списка (последний эелмент)

    mov rdi, rax                      ; Передаем голову списка
    mov rsi, print_int                ; Передаем функцию печати
    call m                            ; Печатаем весь список

    mov rdi, empty_str                ; Передаем пустую строку
    call puts                         ; Выводим пустую строку

    mov rdx, p                        ; Передаем функцию проверки
    xor rsi, rsi                      ; Обнуляем указатель на результат
    mov rdi, rbx                      ; Передаем голову списка
    call f                            ; Фильтруем список

    mov rdi, rax                      ; Передаем голову нового списка
    mov rsi, print_int                ; Передаем функцию печати
    call m                            ; Печатаем новый список

    mov rdi, empty_str                ; Передаем пустую строку
    call puts                         ; Выводим пустую строку

    pop rbx                           ; Восстанавливаем rbx

    xor rax, rax                      ; Обнуляем rax
    ret                               ; Возвращаемся из программы