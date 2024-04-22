#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZIP_SIGNATURE 0x504b0304 // Сигнатура ZIP-архива

#define JPEG_BEGIN_SIGNATURE 0xD8FF // Начало JPEG, reverse FF DB
#define JPEG_END_SIGNATURE 0xD9FF // Конец JPEG, reverse FF D9

#define ZIP 1
#define JPEG 2
#define ERROR 0

// Функция для проверки, является ли файл Rarjpeg-ом
int FindFormat(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Ошибка открытия файла\n");
        return 0;
    }

    fseek(file, 0, SEEK_SET); // Переходим в начало файла
    __uint16_t signature_begin;
    fread(&signature_begin, sizeof(__uint16_t), 1, file); // Считываем 2 байта с начала файла
    fseek(file, -2, SEEK_END); // Переходим в конец файла
    __uint16_t signature_end;
    fread(&signature_end, sizeof(__uint16_t), 1, file); // Считываем 2 байта с конца файла

    fclose(file);


    if (signature_begin == JPEG_BEGIN_SIGNATURE)
    {
        if (signature_end == JPEG_END_SIGNATURE)
            return JPEG;
        else
            return ZIP;
    } else
    {
        return ERROR;
    }    

}

__uint8_t FindSignature_JPG(FILE *__restrict__ file)  // Поиск сигнатуры обычного архива
{
    __uint8_t signature;
    __uint8_t step = 0;
    while (1) // Поиск, пока не будет найдена сигнатура или закончится файл
    {
        fread(&signature, sizeof(__uint8_t), 1, file); // Считываем 1 байт
        switch (signature)
        {
        case 0x50:
            step = 1;
            break;
        case 0x4B:
            if (step == 1)
                step = 2;
            else 
                step = 0;
            break;
        case 0x03:
            if (step == 2)
                step = 3;
            else 
                step = 0;
            break;
        case 0x04:
            if (step == 3)
                step = 4;
            else 
                step = 0;
            break;
        default:
            step = 0;
            break;
        }

        if (feof(file))
        {
            return 0;
        }

        if (step == 4)
            break;
    }
    

    return 1;
}

int FindEndJPEG(FILE *file)
{
    if (file == NULL) {
        perror("Ошибка открытия файла\n");
        return 0;
    }
    fseek(file, 0, SEEK_SET); // Переходим в начало файла

    __uint8_t buf;
    __uint16_t signature_begin_zip;

    while (1)
    {

        fread(&buf, 1, 1, file);
        if (buf == 0xFF)
        {
            fread(&buf, 1, 1, file);
            if (buf == 0xD9)
            {

                fread(&signature_begin_zip, sizeof(__uint16_t), 1, file); // Считываем 2 байта
                if (signature_begin_zip == 0x4b50)
                {
                    break; // Find JpegZip 
                }
            }
        }
        if (feof(file))
        {
            return ERROR;
        }
    }

    __uint16_t format_begin_zip;
    fread(&format_begin_zip, sizeof(__uint16_t), 1, file); // Считываем 2 байта различной сигнатуры zip

    __int8_t format_archive = -1;
    switch (format_begin_zip)
    {
    case 0x0403:
        printf("Сигнатура обычного архива\n");
        format_archive = 1;
        break;
    case 0x0605:
        printf("Сигнатура пустого архива\n");
        format_archive = 2;
        break;
    case 0x0807:
        printf("Сигнатура архива, разделенного на части\n");
        format_archive = -1; // Не реализована обработка
        break;
    default:
        printf("Неизвестная сигнатура архива\n");
        format_archive = -1; // Не реализована обработка
        break;
    }

    if (fseek(file, -4, SEEK_CUR) != 0) {
        perror("Error seeking in file");
        return 1;
    }

    // Read Local Header
    switch (format_archive)
    {
    case 1:

        while (1)
        {
            /*
            struct LocalFileHeader
            {
                // Обязательная сигнатура, равна 0x04034b50
                uint32_t signature; (skeep) 
                // Минимальная версия для распаковки
                uint16_t versionToExtract(2 byte); 2
                // Битовый флаг
                uint16_t generalPurposeBitFlag(2 byte); 4
                // Метод сжатия (0 - без сжатия, 8 - deflate)
                uint16_t compressionMethod(2 byte); 6
                // Время модификации файла
                uint16_t modificationTime(2 byte); 8
                // Дата модификации файла
                uint16_t modificationDate(2 byte); 10
                // Контрольная сумма
                uint32_t crc32(4 byte); 14
                // Сжатый размер
                uint32_t compressedSize(4 byte); 18
                // Несжатый размер
                uint32_t uncompressedSize(4 byte); 22
                // Длина название файла
                uint16_t filenameLength(2 byte); 24
                // Длина поля с дополнительными данными
                uint16_t extraFieldLength;(2 byte); 26
                // Название файла (размером filenameLength)
                uint8_t *filename; (filenameLength byte) 26 + filenameLength
                // Дополнительные данные (размером extraFieldLength)
                uint8_t *extraField (extraFieldLength byte) 26 + filenameLength + extraFieldLength
            };
            */

            if (!FindSignature_JPG(file))
                return 1;

            if (fseek(file, 22, SEEK_CUR) != 0) {
                perror("Error seeking in file");
                return 1;
            }

            __uint16_t filenameLength; 
            __uint16_t extraFieldLength;
            fread(&filenameLength, sizeof(__uint16_t), 1, file); // Считываем 2 байта filenameLength
            fread(&extraFieldLength, sizeof(__uint16_t), 1, file); // Считываем 2 байта extraFieldLength
            printf("Длина имени файла:%d\n", filenameLength);
            char filename[filenameLength + 1];
            filename[filenameLength] = '\0';
            fread(filename, sizeof(char), filenameLength, file); // Считываем filenameLength байтов
            printf("Filename as string: %s\n", filename);
            printf("Filename as bytes: ");
            for (int i = 0; i < filenameLength; i++) {
                printf("%02X ", (unsigned char)filename[i]);
            }
            printf("\n");

            if (fseek(file, extraFieldLength, SEEK_CUR) != 0) {
                perror("Error seeking in file");
                return 1;
            }
        }
        break;
    default:
        break;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Использование: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    switch (FindFormat(filename))
    {
    case ZIP:
        printf("Обнаружен склеянный JPEG файл\n");
        FILE *file = fopen(filename, "rb");
        FindEndJPEG(file);
        break;
    case JPEG:
        printf("Обнаружен JPEG файл\n");
        break;

    case ERROR:
        printf("Не обнаружен файл\n");
        break;
    
    default:
        break;
    }

    return 0;
}