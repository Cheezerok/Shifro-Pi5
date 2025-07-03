#include "encryption_app.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <cstring>  // Для функции strerror
#include <cerrno>   // Для переменной errno

// Обработчик сигналов для корректного завершения
void signalHandler(int signum) {
    std::cout << "Получен сигнал " << signum << std::endl;
    exit(signum);
}

int main() {
    // Регистрация обработчиков сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "Инициализация приложения..." << std::endl;
    
    // Проверяем доступ к SPI и GPIO
    if (access("/dev/spidev0.0", F_OK) != 0) {
        std::cerr << "Ошибка: SPI интерфейс недоступен" << std::endl;
        std::cerr << "Выполните: sudo raspi-config" << std::endl;
        std::cerr << "И включите SPI в Interface Options" << std::endl;
        return 1;
    }
    
    if (access("/dev/gpiochip0", F_OK) != 0) {
        std::cerr << "Ошибка: GPIO интерфейс недоступен" << std::endl;
        return 1;
    }
    
    // Инициализация приложения
    EncryptionApp app;
    if (!app.init()) {
        std::cerr << "Ошибка инициализации приложения" << std::endl;
        return 1;
    }
    
    // Запуск основного цикла приложения
    std::cout << "Запуск приложения..." << std::endl;
    app.run();
    
    // Корректное завершение программы
    std::cout << "Завершение работы" << std::endl;
    return 0;
} 