#include "keyboard.h"
#include <iostream>
#include <chrono>
#include <map>
#include <array>
#include <algorithm>

MembraneKeyboard::MembraneKeyboard() 
    : chip(nullptr), running(false) {
    // Инициализация указателей на линии GPIO
    for (int i = 0; i < ROW_COUNT; i++) {
        row_lines[i] = nullptr;
    }
    for (int i = 0; i < COL_COUNT; i++) {
        col_lines[i] = nullptr;
    }
    // Инициализация состояний кнопок
    resetButtonStates();
}

void MembraneKeyboard::resetButtonStates() {
    std::lock_guard<std::mutex> lock(state_mutex);
    button_states.clear();
    // Инициализируем состояния для всех GPIO пинов
    button_states = {
        {GPIO_UP, false},
        {GPIO_DOWN, false},
        {GPIO_SELECT, false},
        {GPIO_CONFIRM, false}
    };
}

bool MembraneKeyboard::getButtonState(int gpio_pin) {
    std::lock_guard<std::mutex> lock(state_mutex);
    for (const auto& state : button_states) {
        if (state.first == gpio_pin) {
            return state.second;
        }
    }
    return false;
}

int MembraneKeyboard::gpioToAction(int gpio_pin) {
    switch (gpio_pin) {
        case GPIO_UP:     return BTN_UP;
        case GPIO_DOWN:   return BTN_DOWN;
        case GPIO_SELECT: return BTN_SELECT;
        case GPIO_CONFIRM: return BTN_CONFIRM;
        default: return 0;
    }
}

MembraneKeyboard::~MembraneKeyboard() {
    stop();
    cleanup();
}

bool MembraneKeyboard::init() {
    // Открываем GPIO chip
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        return false;
    }

    // Настраиваем линии для строк (выходы)
    for (int i = 0; i < ROW_COUNT; i++) {
        row_lines[i] = gpiod_chip_get_line(chip, ROW_PINS[i]);
        if (!row_lines[i]) {
            cleanup();
            return false;
        }
        if (gpiod_line_request_output(row_lines[i], "membrane_kb", 1) < 0) {
            cleanup();
            return false;
        }
    }

    // Настраиваем линии для столбцов (входы с подтяжкой к питанию)
    for (int i = 0; i < COL_COUNT; i++) {
        col_lines[i] = gpiod_chip_get_line(chip, COL_PINS[i]);
        if (!col_lines[i]) {
            cleanup();
            return false;
        }
        if (gpiod_line_request_input_flags(col_lines[i], "membrane_kb", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
            cleanup();
            return false;
        }
    }

    // Сбрасываем состояние кнопок при инициализации
    resetButtonStates();
    
    return true;
}

void MembraneKeyboard::start() {
    if (running) return;
    
    // Сбрасываем состояние кнопок перед запуском
    resetButtonStates();
    
    running = true;
    polling_thread = std::thread(&MembraneKeyboard::pollKeyboardInternal, this);
}

void MembraneKeyboard::stop() {
    if (!running) return;
    
    running = false;
    if (polling_thread.joinable()) {
        polling_thread.join();
    }
}

void MembraneKeyboard::setKeyPressCallback(std::function<void(int)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    keyPressCallback = callback;
}

void MembraneKeyboard::pollKeyboardInternal() {

    
    // Массив для хранения последних состояний каждой кнопки
    static std::map<int, std::array<bool, 3>> button_history;
    static std::map<int, bool> last_reported_states;
    
    // Инициализация состояний
    for (int gpio : {GPIO_UP, GPIO_DOWN, GPIO_SELECT, GPIO_CONFIRM}) {
        if (button_history.find(gpio) == button_history.end()) {
            button_history[gpio] = {false, false, false};
            last_reported_states[gpio] = false;
        }
    }
    
    while (running) {
        try {
            // Устанавливаем все строки в высокий уровень
            for (int i = 0; i < ROW_COUNT; i++) {
                gpiod_line_set_value(row_lines[i], 1);
            }
            
            // Небольшая задержка для стабилизации
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            // Проверяем каждую строку
            for (int row = 0; row < ROW_COUNT; row++) {
                // Устанавливаем текущую строку в низкий уровень
                gpiod_line_set_value(row_lines[row], 0);
                
                // Задержка для стабилизации сигнала
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                
                // Проверяем каждый столбец
                for (int col = 0; col < COL_COUNT; col++) {
                    int value = gpiod_line_get_value(col_lines[col]);
                    
                    // Обновляем историю состояний для текущей кнопки
                    int gpio = COL_PINS[col];
                    if (gpio != -1) {
                        // Сдвигаем историю состояний
                        button_history[gpio][0] = button_history[gpio][1];
                        button_history[gpio][1] = button_history[gpio][2];
                        button_history[gpio][2] = (value == 0); // Кнопка нажата, если значение 0
                        
                        // Проверяем, есть ли стабильное состояние
                        bool current_state = (button_history[gpio][0] && 
                                           button_history[gpio][1] && 
                                           button_history[gpio][2]);
                        
                        // Если состояние изменилось и стабильно
                        if (current_state != last_reported_states[gpio]) {

                            
                            {
                                std::lock_guard<std::mutex> lock(state_mutex);
                                // Обновляем состояние в основном хранилище
                                auto it = std::find_if(button_states.begin(), button_states.end(),
                                    [gpio](const auto& pair) { return pair.first == gpio; });
                                if (it != button_states.end()) {
                                    it->second = current_state;
                                }
                            }
                            
                            last_reported_states[gpio] = current_state;
                            
                            if (current_state) {
                                int action = gpioToAction(gpio);
                                if (action != 0) {
                                    std::function<void(int)> callback;
                                    {
                                        std::lock_guard<std::mutex> lock(callback_mutex);
                                        callback = keyPressCallback;
                                    }
                                    if (callback) {
                                        callback(action);
                                    }
                                }
                            }
                        }
                    }
                }
                
                gpiod_line_set_value(row_lines[row], 1);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
        } catch (const std::exception& e) {

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    

}

void MembraneKeyboard::cleanup() {
    stop();
    
    for (int i = 0; i < ROW_COUNT; i++) {
        if (row_lines[i]) {
            gpiod_line_release(row_lines[i]);
            row_lines[i] = nullptr;
        }
    }
    
    for (int i = 0; i < COL_COUNT; i++) {
        if (col_lines[i]) {
            gpiod_line_release(col_lines[i]);
            col_lines[i] = nullptr;
        }
    }
    
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
    

} 