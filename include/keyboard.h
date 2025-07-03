#pragma once

#include <functional>
#include <vector>
#include <gpiod.h>
#include <thread>
#include <atomic>
#include <mutex>

class MembraneKeyboard {
public:
    // Константы для GPIO пинов кнопок
    static const int GPIO_UP = 6;      // Кнопка вверх
    static const int GPIO_DOWN = 5;    // Кнопка вниз
    static const int GPIO_SELECT = 19; // Кнопка выбора
    static const int GPIO_CONFIRM = 13;// Кнопка подтверждения

    // Константы для действий кнопок (для обратной совместимости)
    static const int BTN_UP = 1;      // Действие вверх
    static const int BTN_DOWN = 2;    // Действие вниз
    static const int BTN_SELECT = 3;  // Действие выбора
    static const int BTN_CONFIRM = 4; // Действие подтверждения

    // Конструктор и деструктор
    MembraneKeyboard();
    ~MembraneKeyboard();

    // Инициализация клавиатуры
    bool init();

    // Установка callback-функции для обработки нажатий
    void setKeyPressCallback(std::function<void(int)> callback);

    // Запуск и остановка опроса клавиатуры
    void start();
    void stop();

    // Сброс состояния кнопок
    void resetButtonStates();

    // Получение текущего состояния кнопки по GPIO
    bool getButtonState(int gpio_pin);

private:
    // GPIO пины для строк и столбцов
    static const int ROW_COUNT = 1;
    static const int COL_COUNT = 4;
    
    // Номера GPIO пинов (прямое соответствие)
    const unsigned int ROW_PINS[ROW_COUNT] = {26}; // Общий провод
    const unsigned int COL_PINS[COL_COUNT] = {GPIO_UP, GPIO_DOWN, GPIO_SELECT, GPIO_CONFIRM};
    
    // Обработчик нажатий
    std::function<void(int)> keyPressCallback;
    
    // Флаг работы потока опроса
    std::atomic<bool> running;
    
    // Поток опроса клавиатуры
    std::thread polling_thread;
    
    // GPIO chip и линии
    gpiod_chip* chip;
    gpiod_line* row_lines[ROW_COUNT];
    gpiod_line* col_lines[COL_COUNT];
    
    // Внутренний метод опроса клавиатуры (используется в потоке)
    void pollKeyboardInternal();
    
    // Освобождение ресурсов GPIO
    void cleanup();

    // Состояние кнопок (для каждой кнопки храним её GPIO пин)
    std::vector<std::pair<int, bool>> button_states;
    
    // Мьютексы для защиты доступа к общим данным
    std::mutex state_mutex;    // Для доступа к состоянию кнопок
    std::mutex callback_mutex; // Для доступа к callback-функции

    // Преобразование GPIO пина в код действия
    int gpioToAction(int gpio_pin);
}; 