#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>

// Типы для указателей на функции
typedef double (*Operation2)(double, double);  // Операции с двумя числами
typedef double (*Operation1)(double);          // Операции с одним числом

// Структура для хранения операций с двумя числами
typedef struct {
    char name[256];
    Operation2 func;
    HMODULE handle;
} Command2;

// Структура для хранения операций с одним числом
typedef struct {
    char name[256];
    Operation1 func;
    HMODULE handle;
} Command1;

// Загрузка операций из DLL
int load_operations(const char *directory, Command2 **commands2, int *count2, Command1 **commands1, int *count1) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Unable to open directory");
        return -1;
    }

    struct dirent *entry;
    *count2 = 0;
    *count1 = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".dll")) { // Проверяем, что файл является DLL
            char path[1024];
            snprintf(path, sizeof(path), "%s\\%s", directory, entry->d_name);

            HMODULE handle = LoadLibrary(path);
            if (!handle) {
                fprintf(stderr, "Error loading %s\n", path);
                continue;
            }

            // Пытаемся загрузить функцию operate2
            Operation2 func2 = (Operation2)GetProcAddress(handle, "operate2");
            if (func2) {
                *commands2 = realloc(*commands2, sizeof(Command2) * (*count2 + 1));
                snprintf((*commands2)[*count2].name, sizeof((*commands2)[*count2].name), "%s", entry->d_name);
                (*commands2)[*count2].func = func2;
                (*commands2)[*count2].handle = handle;
                (*count2)++;
                continue;
            }

            // Пытаемся загрузить функцию operate1
            Operation1 func1 = (Operation1)GetProcAddress(handle, "operate1");
            if (func1) {
                *commands1 = realloc(*commands1, sizeof(Command1) * (*count1 + 1));
                snprintf((*commands1)[*count1].name, sizeof((*commands1)[*count1].name), "%s", entry->d_name);
                (*commands1)[*count1].func = func1;
                (*commands1)[*count1].handle = handle;
                (*count1)++;
                continue;
            }

            // Если функции не найдены, освобождаем DLL
            FreeLibrary(handle);
        }
    }

    closedir(dir);
    return 0;
}

// Освобождение ресурсов
void free_operations(Command2 *commands2, int count2, Command1 *commands1, int count1) {
    for (int i = 0; i < count2; i++) {
        FreeLibrary(commands2[i].handle);
    }
    free(commands2);

    for (int i = 0; i < count1; i++) {
        FreeLibrary(commands1[i].handle);
    }
    free(commands1);
}

// Основная логика калькулятора с использованием DLL
void execute_calculator(Command2 *commands2, int command_count2, Command1 *commands1, int command_count1) {
    int choice;
    printf("Select calculator mode (1 for single number, 2 for two numbers): ");
    scanf("%d", &choice);

    if (choice == 2) {
        // Выполнение операции с двумя числами
        double a, b;
        printf("Enter two numbers: \n");
        printf("A = ");
        scanf("%lf", &a);
        printf("B = ");
        scanf("%lf", &b);

        printf("Select operation (1 - Add, 2 - Subtract, 3 - Multiply, 4 - Divide): ");
        scanf("%d", &choice);

        if (choice >= 1 && choice <= command_count2) {
            double result = commands2[choice - 1].func(a, b);
            printf("Result: %.2f\n", result);
        } else {
            printf("Invalid operation.\n");
        }
    } else if (choice == 1) {
        // Выполнение операции с одним числом
        double a;
        printf("Enter number: \n");
        scanf("%lf", &a);

        printf("Select operation (1 - Square Root): ");
        scanf("%d", &choice);

        if (choice >= 1 && choice <= command_count1) {
            double result = commands1[choice - 1].func(a);
            printf("Result: %.2f\n", result);
        } else {
            printf("Invalid operation.\n");
        }
    } else {
        printf("Invalid choice.\n");
    }
}

// Главная точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Command2 *commands2 = NULL;
    int command_count2 = 0;
    Command1 *commands1 = NULL;
    int command_count1 = 0;

    // Загрузка операций из DLL
    const char *dll_directory = "libs";  // Укажите путь к папке с DLL
    if (load_operations(dll_directory, &commands2, &command_count2, &commands1, &command_count1) == -1) {
        return 1;
    }

    // Выполнение калькулятора
    execute_calculator(commands2, command_count2, commands1, command_count1);

    // Освобождение ресурсов
    free_operations(commands2, command_count2, commands1, command_count1);

    return 0;
}
