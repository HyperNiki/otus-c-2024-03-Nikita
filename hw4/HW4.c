#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ctype.h>
#include "cJSON.h"

#define BUFFER_SIZE 1024
#define HTTP_OK 200
// #define ALL_DAYS

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int is_valid_city_name(const char *city) {
    for (int i = 0; city[i] != '\0'; i++) {
        if (!isalpha(city[i])) {
            return 0;
        }
    }
    return 1;
}

void print_weather_info(cJSON *weather) {
    const char *days[3] = {"Сегодня", "Завтра", "Послезавтра"};
    if (cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) {
        cJSON *day;
        u_int8_t number_day = 0;
        day = cJSON_GetArrayItem(weather, number_day);
#ifdef ALL_DAYS
        while (day)
        {
#endif
            if (number_day < 3)
                printf("%s\n", days[number_day]);
            else
                printf("На %d день\n", number_day + 1);

            cJSON *hourly = cJSON_GetObjectItemCaseSensitive(day, "hourly");

            cJSON *hour;
            u_int8_t number_hour = 0;
            hour = cJSON_GetArrayItem(hourly, number_hour);

            while (hour)
            {
                printf("В %d час\n", number_hour * 3);

                // Process hour

                cJSON *weatherDesc = cJSON_GetObjectItemCaseSensitive(hour, "weatherDesc");
                cJSON *weatherCur = cJSON_GetArrayItem(weatherDesc, 0);
                cJSON *weatherState = cJSON_GetObjectItemCaseSensitive(weatherCur, "value");
                if (cJSON_IsString(weatherState)) {
                    printf("Погода: %s\n", weatherState->valuestring);
                }

                cJSON *winddir16Point = cJSON_GetObjectItemCaseSensitive(hour, "winddir16Point");
                if (cJSON_IsString(winddir16Point)) {
                    printf("Направление ветра: %s\n", winddir16Point->valuestring);
                }

                cJSON *windspeedKmph = cJSON_GetObjectItemCaseSensitive(hour, "windspeedKmph");
                if (cJSON_IsString(windspeedKmph)) {
                    printf("Скорость ветра: %s км/ч\n", windspeedKmph->valuestring);
                }

                cJSON *tempC = cJSON_GetObjectItemCaseSensitive(hour, "tempC");
                if (cJSON_IsString(tempC)) {
                    printf("Температура: %s °C\n", tempC->valuestring);
                }
                //

                number_hour++;
                hour = cJSON_GetArrayItem(hourly, number_hour);   
                printf("\n");
            }

#ifdef ALL_DAYS
            number_day++;
            day = cJSON_GetArrayItem(weather, number_day);
            printf("\n\n");
        }
#endif
    }
}

void fetch_weather_data(const char *city) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
    chunk.size = 0;           /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        char url[BUFFER_SIZE];
        snprintf(url, BUFFER_SIZE, "https://wttr.in/%s?format=j1", city);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            goto END_FETCH;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != HTTP_OK) {
            fprintf(stderr, "Ошибка: сервер вернул код ответа %ld\n", http_code);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            goto END_FETCH;
        }

        cJSON *root = cJSON_Parse(chunk.memory);
        if (root == NULL) {
            fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            goto END_FETCH;
        }

        cJSON *nearest_area = cJSON_GetObjectItemCaseSensitive(root, "nearest_area");

        if (cJSON_IsArray(nearest_area) && cJSON_GetArraySize(nearest_area) > 0) {
            cJSON *nearest_area_element = cJSON_GetArrayItem(nearest_area, 0);
            cJSON *region = cJSON_GetObjectItemCaseSensitive(nearest_area_element, "region");
            if (cJSON_IsArray(region) && cJSON_GetArraySize(region) > 0) {
                cJSON *region_element = cJSON_GetArrayItem(region, 0);
                cJSON *region_element_value = cJSON_GetObjectItemCaseSensitive(region_element, "value");

                printf("В %s \n\n\n", region_element_value->valuestring);
            }
        }
        cJSON *weather = cJSON_GetObjectItemCaseSensitive(root, "weather");

        print_weather_info(weather);

        cJSON_Delete(root);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

END_FETCH:
    free(chunk.memory);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Использование: %s <City>\n", argv[0]);
        return 1;
    }

    char *city = argv[1];

    if (!is_valid_city_name(city)) {
        fprintf(stderr, "Ошибка: некорректное название города. Название должно содержать только буквы.\n");
        return 1;
    }

    fetch_weather_data(city);

    return 0;
}
