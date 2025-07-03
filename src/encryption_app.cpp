#include "encryption_app.h"
#include "font.h"         // Обычный шрифт
#include "cyrillic_font.h" // Кириллический шрифт
#include "utf8_helper.h"  // Новый помощник для UTF-8
#include "table.h"        // Таблицы для алгоритма Кузнечик
#include "cmac.h"        // Добавляем поддержку CMAC
#include "counter_mode.h" // Добавляем поддержку режима гаммирования
#include <iostream>
#include <fstream>
#include <filesystem>
#include <dirent.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <algorithm>
#include <regex>
#include <set>
#include <sstream>
#include "display_pi.h"
#include "keyboard.h"  // Используем keyboard.h из директории include
#include <atomic>
#include <vector>
#include "kuznechik.h"
#include <sys/statvfs.h>
#include <mntent.h>
#include <sys/stat.h>  // Добавляем для stat() и S_ISDIR

// Функция для преобразования UTF-8 в индекс для кириллического шрифта
uint8_t utf8ToFontIndex(unsigned char c1, unsigned char c2) {
    
    // Таблица соответствий UTF-8 кодов и индексов шрифта для русских букв
    if (c1 == 0xD0) {
        switch (c2) {
            // Заглавные буквы
            case 0x90: return 128; // А
            case 0x91: return 129; // Б
            case 0x92: return 130; // В
            case 0x93: return 131; // Г
            case 0x94: return 132; // Д
            case 0x95: return 133; // Е
            case 0x96: return 134; // Ж
            case 0x97: return 135; // З
            case 0x98: return 136; // И
            case 0x99: return 137; // Й
            case 0x9A: return 138; // К
            case 0x9B: return 139; // Л
            case 0x9C: return 140; // М
            case 0x9D: return 141; // Н
            case 0x9E: return 142; // О
            case 0x9F: return 143; // П
            case 0xA0: return 144; // Р
            case 0xA1: return 145; // С
            case 0xA2: return 146; // Т
            case 0xA3: return 147; // У
            case 0xA4: return 148; // Ф
            case 0xA5: return 149; // Х
            case 0xA6: return 150; // Ц
            case 0xA7: return 151; // Ч
            case 0xA8: return 152; // Ш
            case 0xA9: return 153; // Щ
            case 0xAA: return 154; // Ъ
            case 0xAB: return 155; // Ы
            case 0xAC: return 156; // Ь
            case 0xAD: return 157; // Э
            case 0xAE: return 158; // Ю
            case 0xAF: return 159; // Я
            
            // Строчные буквы (первая часть)
            case 0xB0: return 160; // а
            case 0xB1: return 161; // б
            case 0xB2: return 162; // в
            case 0xB3: return 163; // г
            case 0xB4: return 164; // д
            case 0xB5: return 165; // е
            case 0xB6: return 166; // ж
            case 0xB7: return 167; // з
            case 0xB8: return 168; // и
            case 0xB9: return 169; // й
            case 0xBA: return 170; // к
            case 0xBB: return 171; // л
            case 0xBC: return 172; // м
            case 0xBD: return 173; // н
            case 0xBE: return 174; // о
            case 0xBF: return 175; // п
        }
    } else if (c1 == 0xD1) {
        switch (c2) {
            // Строчные буквы (вторая часть)
            case 0x80: return 176; // р
            case 0x81: return 177; // с
            case 0x82: return 178; // т
            case 0x83: return 179; // у
            case 0x84: return 180; // ф
            case 0x85: return 181; // х
            case 0x86: return 182; // ц
            case 0x87: return 183; // ч
            case 0x88: return 184; // ш
            case 0x89: return 185; // щ
            case 0x8A: return 186; // ъ
            case 0x8B: return 187; // ы
            case 0x8C: return 188; // ь
            case 0x8D: return 189; // э
            case 0x8E: return 190; // ю
            case 0x8F: return 191; // я
        }
    }
    
    // Для ASCII символов
    if (c1 < 128) {
        return c1;
    }
    

    return '?'; 
}

// Конструктор
EncryptionApp::EncryptionApp()
    : display(0, RESET_PIN, DC_PIN, DISPLAY_WIDTH, DISPLAY_HEIGHT),
      selected_item(0),
      waiting_for_key(false),
      current_file_index(0),
      current_page(0) {
    

}

// Деструктор
EncryptionApp::~EncryptionApp() {
    
    // Останавливаем опрос клавиатуры
    keyboard.stop();
    
    resetTerminalSettings();
}


bool EncryptionApp::init() {
    
    setupTerminal();
    
    // Инициализация дисплея
    if (!display.init()) {
        return false;
    }
    
    // Инициализация мембранной клавиатуры
    if (!keyboard.init()) {
        return false;
    }
    
    // Установка callback-функции для обработки нажатий клавиш
    keyboard.setKeyPressCallback([this](int key) {
        this->handleMembraneKeypress(key);
    });
    
    // Запуск потока опроса клавиатуры
    keyboard.start();
    
    // Установка ориентации дисплея и шрифта
    display.setRotation(DisplayRotation::ROTATION_0);
    display.setFont(CYRILLIC_FONT);

    return true;
}

// Настройка терминала для работы с клавиатурой
void EncryptionApp::setupTerminal() {
    // Сохраняем текущие настройки терминала
    if (tcgetattr(STDIN_FILENO, &old_tio) < 0) {
        return;
    }
    
    // Копируем настройки для модификации
    struct termios new_tio = old_tio;
    
    // Отключаем канонический режим и эхо
    new_tio.c_lflag &= ~(ICANON | ECHO);
    // Устанавливаем минимальное количество символов и время ожидания
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    
  
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) < 0) {

    }
    
   
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    

}

// Восстановление исходных настроек терминала
void EncryptionApp::resetTerminalSettings() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_tio) < 0) {
        std::cerr << "Ошибка при восстановлении настроек терминала" << std::endl;
    } else {
        std::cout << "Настройки терминала восстановлены" << std::endl;
    }
}

// Функция для получения символа с клавиатуры без ожидания Enter
int EncryptionApp::getch() {
    unsigned char ch;
    int n = read(STDIN_FILENO, &ch, 1);
    if (n == 1) {
        return ch;
    }
    return -1;
}


bool EncryptionApp::kbhit() {
    unsigned char ch;
    int n = read(STDIN_FILENO, &ch, 1);
    if (n > 0) {
        
        ungetc(ch, stdin);
        return true;
    }
    return false;
}

// Основной цикл приложения
void EncryptionApp::run() {
    
    // Отрисовка меню
    try {
        drawMenu();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при отрисовке главного меню: " << e.what() << std::endl;
    }
    
    // Основной цикл приложения
    bool running = true;
    auto last_status_check = std::chrono::steady_clock::now();
    int status_check_counter = 0;
    
    while (running) {
        try {
            // Проверяем состояние приложения каждые 5 секунд
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_status_check).count() >= 5) {

                last_status_check = now;
            }
            
            // Небольшая задержка для снижения нагрузки на CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

// Отрисовка меню
void EncryptionApp::drawMenu() {
    // Очистка экрана
    display.clearScreen(COLOR_BLACK);
    
    
    // Отрисовка заголовка компании
    std::string company_name = "СТЦ";
    
    // Рисуем рамку вокруг заголовка
    int16_t header_width = company_name.length() * (CYRILLIC_FONT.width + 2) + 20;
    int16_t header_x = (display.getWidth() - header_width) / 2;
    int16_t header_y = 10;
    
    // Рисуем рамку
    display.drawRect(header_x, header_y, 
                    header_width, CYRILLIC_FONT.height + 10, 
                    COLOR_GREEN);

    // Рисуем заголовок по центру рамки
    drawCurrentFile(company_name, -1, header_y + 5, COLOR_GREEN);
    
    // Отрисовка декоративных линий
    int16_t line_y = header_y + CYRILLIC_FONT.height + 15;
    display.drawLine(10, line_y, display.getWidth() - 10, line_y, COLOR_GREEN);
    display.drawLine(10, line_y + 2, display.getWidth() - 10, line_y + 2, COLOR_GREEN);

    // Вычисляем размеры для меню
    int16_t menu_height = 2 * (CYRILLIC_FONT.height + 15); 
    int16_t start_y = (display.getHeight() - menu_height) / 2 + 20; 
    
    // Отрисовка пунктов меню
    for (size_t i = 0; i < 2; i++) {
        drawMenuItem(i, i == selected_item);
    }
}

void EncryptionApp::drawMenuItem(int index, bool is_selected) {
    static int last_selected = -1;
    if (last_selected == index && !is_selected) return;  // Не перерисовываем неизменившиеся пункты
    
    const char* menu_items[] = {
        "2 - РАСШИФРОВАТЬ",
        "1 - ЗАШИФРОВАТЬ"
    };
    
    // Вычисляем начальную позицию Y 
    int16_t menu_height = 2 * (CYRILLIC_FONT.height + 15);  
    int16_t start_y = (display.getHeight() - menu_height) / 2 + 20;  
    
    // Инвертируем индекс для отрисовки снизу вверх
    int display_index = 1 - index;
    int16_t item_y = start_y + display_index * (CYRILLIC_FONT.height + 15);
    
    // Цвет текста в зависимости от выбора
    uint16_t text_color = is_selected ? COLOR_WHITE : COLOR_GREEN;
    // Цвет фона в зависимости от выбора
    uint16_t bg_color = is_selected ? COLOR_BLUE : COLOR_BLACK;
    
    const char* menu_text = menu_items[index];
    
    // Вычисляем размеры пункта меню
    int16_t item_width = strlen(menu_text) * (CYRILLIC_FONT.width + 2) + 20;
    int16_t item_x = (display.getWidth() - item_width) / 2;
    
    // Рисуем рамку вокруг пункта меню
    display.fillRect(item_x - 5, item_y - 5, 
                    item_width + 10, CYRILLIC_FONT.height + 10, 
                    bg_color);
    display.drawRect(item_x - 5, item_y - 5, 
                    item_width + 10, CYRILLIC_FONT.height + 10, 
                    is_selected ? COLOR_WHITE : COLOR_GREEN);
    
    // Отрисовка текста с автоцентрированием
    drawCurrentFile(menu_text, -1, item_y, text_color);
    
    last_selected = is_selected ? index : -1;
}

void EncryptionApp::showMessage(const std::string& message, bool isError, int timeout_seconds) {
    display.clearScreen(COLOR_BLACK);
    
    // Адаптируем сообщения под короткие варианты
    std::string display_message = message;
    int16_t text_y = this->display.getHeight() / 2 - 10;
    uint16_t color = isError ? COLOR_RED : COLOR_GREEN;
    this->drawCurrentFile(display_message, -1, text_y, color);
    
    // Ждем указанное время
    std::this_thread::sleep_for(std::chrono::seconds(timeout_seconds));
}

// Обработка нажатий клавиш
void EncryptionApp::handleKeypress(char key) {
    switch (key) {
        case 'w':
        case 'W':
        case 65:
            if (selected_item > 0) {
                drawMenuItem(selected_item, false);
                selected_item--;
                drawMenuItem(selected_item, true);
            }
            break;
            
        case 's':
        case 'S':
        case 66: 
            if (selected_item < 1) {
                drawMenuItem(selected_item, false);
                selected_item++;
                drawMenuItem(selected_item, true);
            }
            break;
            
        case '1':
            if (selected_item != 0) {
                drawMenuItem(selected_item, false);
                selected_item = 0;
                drawMenuItem(selected_item, true);
            }
            processSelection(MenuOption::ENCRYPT);
            break;
            
        case '2':
            if (selected_item != 1) {
                drawMenuItem(selected_item, false);
                selected_item = 1;
                drawMenuItem(selected_item, true);
            }
            processSelection(MenuOption::DECRYPT);
            break;
            
        case '3':
            // Обновляем только текущую строку
            drawMenuItem(selected_item, true);
            break;
            
        case '\n':
        case '\r':
        case ' ':
            processSelection(static_cast<MenuOption>(selected_item));
            break;
            
        case 'q':
        case 'Q':
            drawMenu();
            break;
    }
}

bool EncryptionApp::checkDeviceExists(const std::string& device) {
    // Проверяем наличие физического устройства
    std::ifstream dev(device.c_str());
    return dev.good();
}

// Реализация новых методов
void EncryptionApp::loadFilesFromDrive(const std::string& path) {
    file_list.clear();
    current_file_index = 0;
    current_page = 0;
    files_per_page = 4;

    try {
        // Проверяем существование пути
        if (!std::filesystem::exists(path)) {
        return;
    }
    
        // Проверяем, что это директория
        if (!std::filesystem::is_directory(path)) {
                    return;
                }
                
        // Проверяем доступ к директории
        std::error_code ec;
        std::filesystem::directory_iterator it(path, ec);
        if (ec) {
                    return;
                }

        // Считаем файлы
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                FileInfo file;
                file.name = entry.path().filename().string();
                file.selected = false;
                file.full_path = entry.path().string();
                file_list.push_back(file);
            }
        }

        
    } catch (const std::filesystem::filesystem_error& e) {
        throw;
    } catch (const std::exception& e) {
        throw;
    }
}

void EncryptionApp::drawFileSelectionMenu(bool force_full_redraw) {
    static bool first_draw = true;
    static int last_selected = -1;
    static size_t last_page = 0;

    bool need_full_update = first_draw || force_full_redraw || last_page != current_page;

    if (need_full_update) {
        display.clearScreen(COLOR_BLACK);
        display.drawRect(5, 5, display.getWidth() - 10, display.getHeight() - 10, COLOR_GREEN);
        std::string header = "ВЫБЕРИТЕ ФАЙЛЫ:";
        int16_t header_y = 10;
        drawCurrentFile(header, -1, header_y, COLOR_GREEN);
        display.drawLine(10, header_y + CYRILLIC_FONT.height + 5,
                       display.getWidth() - 10, header_y + CYRILLIC_FONT.height + 5,
                       COLOR_GREEN);
        first_draw = false;
        last_page = current_page;
    }

    int16_t list_start_y = 35;
    int16_t list_end_y = display.getHeight() - 10;
    int16_t list_height = list_end_y - list_start_y;
    files_per_page = list_height / (CYRILLIC_FONT.height + 10);

    size_t start_idx = current_page * files_per_page;
    int16_t item_y = list_start_y;

    if (need_full_update) {
        // Полностью обновляем список файлов
        display.fillRect(10, list_start_y, display.getWidth() - 20, list_height, COLOR_BLACK);
        for (size_t i = 0; i < files_per_page && (start_idx + i) < file_list.size(); ++i) {
            const auto& file = file_list[start_idx + i];
            bool is_current = (start_idx + i == current_file_index);
            uint16_t text_color = is_current ? COLOR_WHITE : COLOR_GREEN;
            uint16_t bg_color = is_current ? COLOR_BLUE : COLOR_BLACK;
            display.fillRect(10, item_y - 2, display.getWidth() - 20, CYRILLIC_FONT.height + 4, bg_color);
            std::string checkbox = file.selected ? "[X] " : "[ ] ";
            std::string filename = file.name;
            if (filename.length() > 20) filename = filename.substr(0, 17) + "...";
            std::string line = checkbox + filename;
            drawCurrentFile(line, 15, item_y, text_color);
            item_y += CYRILLIC_FONT.height + 10;
        }
        // Номер страницы
        if (file_list.size() > files_per_page) {
            size_t total_pages = (file_list.size() + files_per_page - 1) / files_per_page;
            std::string page_info = std::to_string(current_page + 1) + "/" + std::to_string(total_pages);
            drawCurrentFile(page_info, -1, list_end_y - CYRILLIC_FONT.height - 5, COLOR_GREEN);
        }
        last_selected = current_file_index;
    } else {
        // Обновляем только одну строку (старую и новую)
        for (int i = 0; i < files_per_page; ++i) {
            size_t idx = start_idx + i;
            if (idx >= file_list.size()) break;
            if ((int)idx == last_selected || (int)idx == current_file_index) {
                int16_t y = list_start_y + i * (CYRILLIC_FONT.height + 10);
                bool is_current = (idx == current_file_index);
                uint16_t text_color = is_current ? COLOR_WHITE : COLOR_GREEN;
                uint16_t bg_color = is_current ? COLOR_BLUE : COLOR_BLACK;
                display.fillRect(10, y - 2, display.getWidth() - 20, CYRILLIC_FONT.height + 4, bg_color);
                std::string checkbox = file_list[idx].selected ? "[X] " : "[ ] ";
                std::string filename = file_list[idx].name;
                if (filename.length() > 20) filename = filename.substr(0, 17) + "...";
                std::string line = checkbox + filename;
                drawCurrentFile(line, 15, y, text_color);
            }
        }
        last_selected = current_file_index;
    }
}

void EncryptionApp::handleMembraneKeypress(int key) {
    std::string action_str;
    switch (key) {
        case MembraneKeyboard::BTN_UP: action_str = "BTN_UP (Вверх)"; break;
        case MembraneKeyboard::BTN_DOWN: action_str = "BTN_DOWN (Вниз)"; break;
        case MembraneKeyboard::BTN_SELECT: action_str = "BTN_SELECT (Подтверждение/4)"; break;
        case MembraneKeyboard::BTN_CONFIRM: action_str = "BTN_CONFIRM (Выбор/3)"; break;
        default: action_str = "UNKNOWN"; break;
    }
    
    if (waiting_for_key) {

        
        bool any_selected = false;
        bool need_redraw = false;
        bool need_full_redraw = false;
        
        try {
            switch (key) {
                case MembraneKeyboard::BTN_UP: // GPIO 6 
                    if (current_file_index > 0) {
                        current_file_index--;
                        if (current_file_index < current_page * files_per_page) {
                            current_page--;
                            need_full_redraw = true;
                        }
                        need_redraw = true;
                    } else {
                        std::cout << ">>> ФАЙЛЫ: Достигнут верхний предел списка" << std::endl;
                    }
                    break;
                    
                case MembraneKeyboard::BTN_DOWN: // GPIO 5 - Вниз
                    if (current_file_index < file_list.size() - 1) {
                        current_file_index++;
                        if (current_file_index >= (current_page + 1) * files_per_page) {
                            current_page++;
                            need_full_redraw = true;
                        }
                        need_redraw = true;
                    } else {
                        std::cout << ">>> ФАЙЛЫ: Достигнут нижний предел списка" << std::endl;
                    }
                    break;
                    
                case MembraneKeyboard::BTN_SELECT: // GPIO 13 - Кнопка 3 - ВЫБОР файла (инвертировать чекбокс)
                    if (!file_list.empty() && current_file_index < file_list.size()) {
                        file_list[current_file_index].selected = !file_list[current_file_index].selected;
                        
                        // Обновляем только текущую строку вместо всего меню
                        size_t start_idx = current_page * files_per_page;
                        int16_t list_start_y = 35;
                        int16_t item_y = list_start_y + (current_file_index - start_idx) * (CYRILLIC_FONT.height + 10);
                        
                        // Перерисовываем только текущую строку
                        display.fillRect(10, item_y - 2, display.getWidth() - 20, CYRILLIC_FONT.height + 4, COLOR_BLUE);
                        std::string checkbox = file_list[current_file_index].selected ? "[X] " : "[ ] ";
                        std::string filename = file_list[current_file_index].name;
                        if (filename.length() > 20) filename = filename.substr(0, 17) + "...";
                        std::string line = checkbox + filename;
                        drawCurrentFile(line, 15, item_y, COLOR_WHITE);
                        
                    } else {
                        std::cout << ">>> ФАЙЛЫ: Список файлов пуст или индекс вне диапазона" << std::endl;
                    }
                    break;
                    
                case MembraneKeyboard::BTN_CONFIRM: // GPIO 19 - Кнопка 4 - ПОДТВЕРЖДЕНИЕ выбора
                    // Проверяем, есть ли выбранные файлы
                    for (const auto& file : file_list) {
                        if (file.selected) {
                            any_selected = true;
                            break;
                        }
                    }
                    if (any_selected) {
                        for (const auto& file : file_list) {
                            if (file.selected) std::cout << "  [X] " << file.name << std::endl;
                        }
                        waiting_for_key = false;
                        
                        if (selected_item == 0) { // Шифрование
                            std::string dest_drive = waitForUsbDrive("УСТАНОВИТЕ\nФЛЕШ НОСИТЕЛЬ\nС ЗАШИФРОВАННОЙ\nИНФОРМАЦИЕЙ", 60);
                            if (dest_drive.empty()) {
                                showMessage("Нет накопителя", true, 2);
                                drawMenu();
                                return;
                            }
                            // Переносим выбранные файлы с шифрованием
                            transferFiles(findUsbMountPoints()[0], dest_drive, true);
                        } else { // Расшифрование
                            std::string dest_drive = waitForUsbDrive("УСТАНОВИТЕ\nФЛЕШ НОСИТЕЛЬ\nС ОТКРЫТОЙ\nИНФОРМАЦИЕЙ", 60);
                            if (dest_drive.empty()) {
                                showMessage("Нет накопителя", true, 2);
                                drawMenu();
                                return;
                            }
                            // Переносим выбранные файлы с расшифрованием
                            transferFiles(findUsbMountPoints()[0], dest_drive, false);
                        }
                        return; 
                    } else {
                        display.clearScreen(COLOR_BLACK);
                        showMessage("Выберите файлы", true, 1);
                        drawFileSelectionMenu(true);
                        return;
                    }
                    break;
                    
                default:
                    break;
            }
        } catch (const std::exception& e) {
            showMessage("Ошибка", true, 1);
            drawFileSelectionMenu(true); // Принудительное обновление после ошибки
        }
        
        // Обновляем отображение если нужно
        if (need_redraw) {
            drawFileSelectionMenu(need_full_redraw);
        }
    } else {
        
        switch (key) {
            case MembraneKeyboard::BTN_UP: // GPIO 6 - Первый пункт меню (Шифрование)
                if (selected_item != 0) {
                    drawMenuItem(selected_item, false);
                    selected_item = 0;
                    drawMenuItem(selected_item, true);
                }
                processSelection(MenuOption::ENCRYPT);
                break;
                
            case MembraneKeyboard::BTN_DOWN: // GPIO 5 - Второй пункт меню (Расшифровка)
                if (selected_item != 1) {
                    drawMenuItem(selected_item, false);
                    selected_item = 1;
                    drawMenuItem(selected_item, true);
                }
                processSelection(MenuOption::DECRYPT);
                break;
                
            case MembraneKeyboard::BTN_CONFIRM: // GPIO 19 - Кнопка 3 - Выбор текущего пункта
            case MembraneKeyboard::BTN_SELECT: // GPIO 13 - Кнопка 4 - Подтверждение текущего пункта
                processSelection(static_cast<MenuOption>(selected_item));
                break;
                
            default:
                break;
        }
    }
}

// Модифицируем метод processSelection для использования нового функционала
void EncryptionApp::processSelection(MenuOption option) {

    // Сбрасываем состояние клавиатуры перед началом операции
    keyboard.resetButtonStates();
    
    switch (option) {
        case MenuOption::ENCRYPT: {
            // Сначала инициализируем режим выбора файлов
            
            // Сбрасываем состояние перед инициализацией
            current_file_index = 0;
            current_page = 0;
            file_list.clear();
            
            // Инициализируем режим выбора файлов
            waiting_for_key = true;
            
            // Ждем подключения флешки с исходными файлами
            std::string source_drive = waitForUsbDrive("УСТАНОВИТЕ\nФЛЕШ НОСИТЕЛЬ\nС ОТКРЫТОЙ\nИНФОРМАЦИЕЙ", 60);
            
            if (source_drive.empty()) {
                showMessage("Нет накопителя", true, 2);
                waiting_for_key = false;
                drawMenu();
                return;
            }
            
            try {
                loadFilesFromDrive(source_drive);
                if (file_list.empty()) {
                    showMessage("Нет файлов", true, 2);
                    waiting_for_key = false;
                    drawMenu();
                    return;
                }
                
                // Очищаем экран перед отрисовкой меню выбора файлов
                display.clearScreen(COLOR_BLACK);
                // Отрисовываем меню выбора файлов
                drawFileSelectionMenu(true);
                return;
                
            } catch (const std::exception& e) {
                showMessage("Ошибка", true, 2);
                waiting_for_key = false;
                drawMenu();
                return;
            }
            break;
        }
        case MenuOption::DECRYPT: {
            // Сбрасываем состояние
            current_file_index = 0;
            current_page = 0;
            file_list.clear();
            waiting_for_key = true;
            
            // Ждем подключения флешки с зашифрованными файлами
            std::string source_drive = waitForUsbDrive("УСТАНОВИТЕ\nФЛЕШ НОСИТЕЛЬ\nС ЗАШИФРОВАННОЙ\nИНФОРМАЦИЕЙ", 60);
            
            if (source_drive.empty()) {
                showMessage("Нет накопителя", true, 2);
                waiting_for_key = false;
                drawMenu();
                return;
            }
            
            try {
                loadFilesFromDrive(source_drive);
                if (file_list.empty()) {
                    showMessage("Нет файлов", true, 2);
                    waiting_for_key = false;
                    drawMenu();
                    return;
                }
                
                // Очищаем экран перед отрисовкой меню выбора файлов
                display.clearScreen(COLOR_BLACK);
                // Отрисовываем меню выбора файлов
                drawFileSelectionMenu(true);
                return;
                
            } catch (const std::exception& e) {
                showMessage("Ошибка: " + std::string(e.what()), true, 3);
                waiting_for_key = false;
                drawMenu();
                return;
            }
            break;
        }
    }
}

// Обновляем отображение текущего файла
void EncryptionApp::drawCurrentFile(const std::string& fileName, int16_t x, int16_t y, uint16_t color) {
    if (fileName.empty()) {
        return;
    }

    const char* utf8_str = fileName.c_str();
    size_t pos = 0;
    size_t char_pos = 0;
    int16_t letter_spacing = 2;  // Базовый отступ между буквами
    
    // Сначала подсчитаем реальную длину текста в символах (с учетом UTF-8)
    size_t real_length = 0;
    std::vector<int16_t> char_widths;  // Хранит ширину каждого символа
    size_t temp_pos = 0;
    while (temp_pos < fileName.length()) {
        unsigned char c1 = utf8_str[temp_pos];
        unsigned char c2 = (temp_pos + 1 < fileName.length()) ? utf8_str[temp_pos + 1] : 0;
        
        // Определяем ширину символа
        int16_t char_width = CYRILLIC_FONT.width;
        if ((c1 == 0xD0 && c2 == 0xA6) ||  // Ц
            (c1 == 0xD1 && c2 == 0x8C) ||  // ь
            (c1 == 0xD0 && c2 == 0x86)) {  // ц
            char_width += 1;  // Добавляем место для хвостика
        }
        
        char_widths.push_back(char_width);
        
        if ((c1 >= 0xD0 && c1 <= 0xD1) && (temp_pos + 1 < fileName.length())) {
            temp_pos += 2;  // Двухбайтовый UTF-8
        } else {
            temp_pos += 1;  // Однобайтовый ASCII
        }
        real_length++;
    }
    
    // Вычисляем общую ширину текста с учетом разных размеров символов
    int16_t total_width = 0;
    for (size_t i = 0; i < char_widths.size(); i++) {
        total_width += char_widths[i] + letter_spacing;
    }
    total_width -= letter_spacing;  // Убираем последний отступ
    
    // Если x < 0, центрируем текст по горизонтали
    if (x < 0) {
        x = (display.getWidth() - total_width) / 2;
        if (x < 0) x = 0;
    }

    // Удаляем добавление чекбокса! Просто рисуем строку fileName
    // Очищаем область перед отрисовкой текста
    display.fillRect(x - 1, y - 1, total_width + 2, CYRILLIC_FONT.height + 2, COLOR_BLACK);
    
    // Отрисовываем текст
    int16_t current_x = x;
    size_t char_index = 0;
    while (pos < fileName.length()) {
        unsigned char c1 = utf8_str[pos];
        unsigned char c2 = (pos + 1 < fileName.length()) ? utf8_str[pos + 1] : 0;
        
        // Проверяем, не выходим ли за пределы экрана
        if (current_x + CYRILLIC_FONT.width > display.getWidth()) {
            break;
        }
        
        uint8_t font_index;
        if ((c1 >= 0xD0 && c1 <= 0xD1) && (pos + 1 < fileName.length())) {
            font_index = utf8ToFontIndex(c1, c2);
            pos += 2;
        } else {
            font_index = c1;
            pos += 1;
        }
        
        try {
            // Специальная обработка для проблемных букв
            bool is_problematic = false;
            if ((c1 == 0xD0 && (c2 == 0xA8 || c2 == 0x88)) ||  // Ш, ш
                (c1 == 0xD0 && (c2 == 0xA9 || c2 == 0x89)) ||  // Щ, щ
                (c1 == 0xD0 && (c2 == 0xA4 || c2 == 0xB4))) {  // Ф, ф
                // Для широких букв рисуем тоньше
                display.drawChar(current_x, y, font_index, color);
                is_problematic = true;
            }
            
            if (!is_problematic) {
                // Для остальных букв рисуем с утолщением
                display.drawChar(current_x, y, font_index, color);
                display.drawChar(current_x + 1, y, font_index, color);
            }
            
            // Специальная обработка для букв с хвостиками
            if ((c1 == 0xD0 && c2 == 0xA6) ||  // Ц
                (c1 == 0xD1 && c2 == 0x8C) ||  // ь
                (c1 == 0xD0 && c2 == 0x86)) {  // ц
                // Добавляем хвостик
                display.drawPixel(current_x + CYRILLIC_FONT.width, y + CYRILLIC_FONT.height - 1, color);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Ошибка при отрисовке символа: " << e.what() << std::endl;
        }
        
        current_x += char_widths[char_index] + letter_spacing;
        char_index++;
    }
}

// Получение пути к USB-накопителю
std::pair<std::string, std::string> EncryptionApp::getStoragePaths() {
    std::string first_usb;
    std::string second_usb;
    
    // Устанавливаем фиксированные пути для флешек
    first_usb = "/media/sda1";
    second_usb = "/media/sdb1";
    
    // Дополнительная проверка на доступность для чтения/записи
    if (!first_usb.empty()) {
        try {
            if (!std::filesystem::is_directory(first_usb) || 
                !std::filesystem::exists(first_usb + "/.")) {
                first_usb = "";
            }
        } catch (...) {
            first_usb = "";
        }
    }
    
    if (!second_usb.empty()) {
        try {
            if (!std::filesystem::is_directory(second_usb) || 
                !std::filesystem::exists(second_usb + "/.")) {
                second_usb = "";
            }
        } catch (...) {
            second_usb = "";
        }
    }
    
    return std::make_pair(first_usb, second_usb);
}

void EncryptionApp::encryptFile(const std::string& source_file, const std::string& dest_file) {
    constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB буфер
    
    // Открываем исходный файл
    std::ifstream in(source_file, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Не удалось открыть исходный файл: " + source_file);
    }
    
    // Получаем размер файла
    in.seekg(0, std::ios::end);
    size_t file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    
    // Подготавливаем мастер-ключ (32 байта)
    unsigned char masterKey[32] = {0};
    memcpy(masterKey, encryptionKey.c_str(), std::min<size_t>(encryptionKey.length(), 16));
    memcpy(masterKey + 16, encryptionKey.c_str(), std::min<size_t>(encryptionKey.length(), 16));
    
    // Выделяем память под раундовые ключи
    unsigned char roundKeys[160] = {0};
    
    // Разворачиваем ключи
    if (kuznechik_expkey(masterKey, roundKeys) != 0) {
        throw std::runtime_error("Ошибка развертывания ключа");
    }
    
    // Создаем временный вектор для накопления данных для CMAC
    std::vector<char> all_data;
    all_data.reserve(file_size);
    
    // Создаем временный файл
    std::string temp_file = dest_file + ".tmp";
    std::ofstream out(temp_file, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Не удалось создать временный файл: " + temp_file);
    }
    
    
    auto iv = counter_mode::generate_iv();
    counter_mode::Counter ctr;
    ctr.setValue(iv.data());
    
    // Записываем синхропосылку в начало файла
    out.write(reinterpret_cast<const char*>(iv.data()), counter_mode::IV_SIZE);
    
    // Буферы для чтения и шифрования
    std::vector<char> buffer(BUFFER_SIZE);
    std::vector<char> encrypted_buffer;
    size_t total_read = 0;
    
    // Читаем и шифруем файл блоками
    while (total_read < file_size) {
        // Определяем размер следующего блока для чтения
        size_t bytes_to_read = std::min(BUFFER_SIZE, file_size - total_read);
        
        // Читаем блок данных
        in.read(buffer.data(), bytes_to_read);
        std::streamsize bytes_read = in.gcount();
        if (bytes_read <= 0) break;
        
        // Добавляем прочитанные данные для последующего вычисления CMAC
        all_data.insert(all_data.end(), buffer.data(), buffer.data() + bytes_read);
        
        // Вычисляем размер зашифрованных данных (округляем до размера блока)
        size_t blocks = (bytes_read + counter_mode::BLOCK_SIZE - 1) / counter_mode::BLOCK_SIZE;
        size_t padded_size = blocks * counter_mode::BLOCK_SIZE;
        encrypted_buffer.resize(padded_size);
        
        // Шифруем блоками
        for (size_t pos = 0; pos < padded_size; pos += counter_mode::BLOCK_SIZE) {
            unsigned char block[counter_mode::BLOCK_SIZE] = {0};
            unsigned char gamma[counter_mode::BLOCK_SIZE] = {0};
            
            // Копируем данные в блок с паддингом
            size_t block_size = std::min<size_t>(counter_mode::BLOCK_SIZE, bytes_read - pos);
            std::memcpy(block, buffer.data() + pos, block_size);
            
            // Генерируем гамму и накладываем её на данные
            counter_mode::generate_gamma(gamma, ctr, roundKeys);
            counter_mode::apply_gamma(block, gamma, block_size);
            
            // Копируем зашифрованный блок в выходной буфер
            std::memcpy(encrypted_buffer.data() + pos, block, block_size);
        }
        
        // Записываем зашифрованный блок
        out.write(encrypted_buffer.data(), bytes_read);
        if (!out.good()) {
            out.close();
            std::filesystem::remove(temp_file);
            throw std::runtime_error("Ошибка записи в файл");
        }
        
        total_read += bytes_read;
    }
    
    // Вычисляем CMAC для всего файла
    auto mac = cmac::calculateCMAC(all_data, masterKey, roundKeys);
    
    // Записываем MAC в конец файла
    out.write(reinterpret_cast<const char*>(mac.data()), mac.size());
    if (!out.good()) {
        out.close();
        std::filesystem::remove(temp_file);
        throw std::runtime_error("Ошибка записи MAC");
    }
    
    // Закрываем файлы
    in.close();
    out.close();
    
    // Проверяем, что временный файл создан успешно
    if (!std::filesystem::exists(temp_file)) {
        throw std::runtime_error("Временный файл не создан");
    }
    
    // Проверяем размер временного файла
    auto temp_size = std::filesystem::file_size(temp_file);
    if (temp_size <= file_size) {
        std::filesystem::remove(temp_file);
        throw std::runtime_error("Некорректный размер зашифрованного файла");
    }
    
    // Перемещаем временный файл в целевой
    try {
        std::filesystem::rename(temp_file, dest_file);
    } catch (const std::filesystem::filesystem_error& e) {
        try {
            std::filesystem::copy_file(temp_file, dest_file, 
                                     std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(temp_file);
        } catch (const std::filesystem::filesystem_error& e2) {
            throw std::runtime_error("Не удалось создать конечный файл: " + 
                                   std::string(e2.what()));
        }
    }
    
    // Финальная проверка
    if (!std::filesystem::exists(dest_file)) {
        throw std::runtime_error("Файл не создан после всех операций");
    }
    
    // Принудительно сбрасываем буферы файловой системы
    sync();
}

// Метод для расшифрования отдельного файла
void EncryptionApp::decryptFile(const std::string& source_file, const std::string& dest_file) {
    constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB буфер для чтения/записи
    
    // Открываем зашифрованный файл
    std::ifstream in(source_file, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Не удалось открыть файл: " + source_file);
    }
    
    // Получаем размер файла
    in.seekg(0, std::ios::end);
    size_t file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    
    // Проверяем минимальный размер (IV + MAC)
    if (file_size < counter_mode::IV_SIZE + 16) {
        throw std::runtime_error("Файл слишком мал для расшифровки");
    }
    
    // Читаем синхропосылку
    std::vector<uint8_t> iv(counter_mode::IV_SIZE);
    in.read(reinterpret_cast<char*>(iv.data()), counter_mode::IV_SIZE);
    
    // Инициализируем счетчик
    counter_mode::Counter ctr;
    ctr.setValue(iv.data());
    
    // Подготавливаем мастер-ключ (32 байта)
    unsigned char masterKey[32] = {0};
    memcpy(masterKey, encryptionKey.c_str(), std::min<size_t>(encryptionKey.length(), 16));
    memcpy(masterKey + 16, encryptionKey.c_str(), std::min<size_t>(encryptionKey.length(), 16));
    
    // Выделяем память под раундовые ключи (10 ключей по 16 байт)
    unsigned char roundKeys[160] = {0};
    
    // Разворачиваем ключи
    if (kuznechik_expkey(masterKey, roundKeys) != 0) {
        throw std::runtime_error("Ошибка развертывания ключа");
    }
    
    // Создаем временный вектор для накопления расшифрованных данных для CMAC
    std::vector<char> decrypted_data;
    size_t encrypted_size = file_size - counter_mode::IV_SIZE - 16; // Размер без IV и MAC
    decrypted_data.reserve(encrypted_size);
    
    // Открываем файл для записи
    std::ofstream out(dest_file, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Не удалось создать файл: " + dest_file);
    }
    
    // Читаем MAC из конца файла
    std::vector<uint8_t> stored_mac(16);
    in.seekg(-16, std::ios::end);
    in.read(reinterpret_cast<char*>(stored_mac.data()), 16);
    
    // Возвращаемся к позиции после IV для чтения данных
    in.seekg(counter_mode::IV_SIZE, std::ios::beg);
    
    // Буфер для чтения и расшифрования
    std::vector<char> buffer(BUFFER_SIZE);
    std::vector<char> decrypted_buffer;
    size_t total_read = 0;
    
    // Читаем и расшифровываем файл блоками
    while (total_read < encrypted_size) {
        // Определяем размер следующего блока для чтения
        size_t bytes_to_read = std::min(BUFFER_SIZE, encrypted_size - total_read);
        
        // Читаем блок данных
        in.read(buffer.data(), bytes_to_read);
        std::streamsize bytes_read = in.gcount();
        if (bytes_read <= 0) break;
        
        // Вычисляем размер расшифрованных данных (должен быть кратен размеру блока)
        size_t blocks = (bytes_read + counter_mode::BLOCK_SIZE - 1) / counter_mode::BLOCK_SIZE;
        size_t padded_size = blocks * counter_mode::BLOCK_SIZE;
        decrypted_buffer.resize(padded_size);
        
        // Расшифровываем блоками
        for (size_t pos = 0; pos < bytes_read; pos += counter_mode::BLOCK_SIZE) {
            unsigned char block[counter_mode::BLOCK_SIZE] = {0};
            unsigned char gamma[counter_mode::BLOCK_SIZE] = {0};
            
            // Копируем зашифрованный блок
            size_t block_size = std::min<size_t>(counter_mode::BLOCK_SIZE, bytes_read - pos);
            std::memcpy(block, buffer.data() + pos, block_size);
            
            // Генерируем гамму и накладываем её на данные
            counter_mode::generate_gamma(gamma, ctr, roundKeys);
            counter_mode::apply_gamma(block, gamma, block_size);
            
            // Копируем расшифрованный блок
            std::memcpy(decrypted_buffer.data() + pos, block, block_size);
        }
        
        // Сохраняем расшифрованные данные для проверки CMAC
        decrypted_data.insert(decrypted_data.end(), 
                            decrypted_buffer.begin(), 
                            decrypted_buffer.begin() + bytes_read);
        
        // Записываем расшифрованный блок
        out.write(decrypted_buffer.data(), bytes_read);
        if (!out.good()) {
            out.close();
            std::filesystem::remove(dest_file);
            throw std::runtime_error("Ошибка записи в файл");
        }
        
        total_read += bytes_read;
    }
    
    // Закрываем файл для записи
    out.close();
    
    // Проверяем CMAC
    auto calculated_mac = cmac::calculateCMAC(decrypted_data, masterKey, roundKeys);
    
    // Сравниваем вычисленный и сохраненный MAC
    if (!std::equal(calculated_mac.begin(), calculated_mac.end(), stored_mac.begin())) {
        std::filesystem::remove(dest_file);
        throw std::runtime_error("Ошибка: MAC не совпадает");
    }
    
    in.close();
}

std::vector<std::string> EncryptionApp::findUsbMountPoints() {
    std::vector<std::string> mount_points;
    

    for (char drive = 'a'; drive <= 'z'; drive++) {
        std::string mount_point = "/media/sd" + std::string(1, drive) + "1";
        if (std::filesystem::exists(mount_point)) {
            mount_points.push_back(mount_point);
        }
    }
    
    return mount_points;
}


std::vector<DeviceInfo> EncryptionApp::getConnectedDevices() {
    std::vector<DeviceInfo> devices;
    std::set<std::string> mounted_points;
    
    // Читаем точки монтирования
    FILE* mounts = setmntent("/proc/mounts", "r");
    if (mounts) {
        struct mntent* ent;
        while ((ent = getmntent(mounts)) != nullptr) {
            mounted_points.insert(std::string(ent->mnt_dir));
                   }
        endmntent(mounts);
    }
    
   
    for (char drive = 'a'; drive <= 'z'; drive++) {
        std::string base_dev = "/dev/sd" + std::string(1, drive);
        
        // Проверяем разделы 
        for (int partition = 1; partition <= 9; partition++) {
            std::string dev_path = base_dev + std::to_string(partition);
            
            
            // Проверяем существование устройства
            if (std::filesystem::exists(dev_path)) {
                DeviceInfo device;
                device.dev_path = dev_path;
                device.is_mounted = false;
                
                
                for (const auto& mount_point : mounted_points) {
                    std::string expected_mount = "/media/sd" + std::string(1, drive) + std::to_string(partition);
                    if (mount_point == expected_mount) {
                        device.mount_point = mount_point;
                        device.is_mounted = true;
                        break;
        }
                }
                
                devices.push_back(device);
            }
        }
    }
    
    return devices;
}


bool EncryptionApp::isUsbStorage(const std::string& path) {
    
    std::string device_path;
    if (path.find("/media/sda") != std::string::npos) {
        device_path = "/dev/sda1";
    } else if (path.find("/media/sdb") != std::string::npos) {
        device_path = "/dev/sdb1";
    } else {
        return false;
    }

   
    if (access(device_path.c_str(), F_OK) == -1) {
        return false;
    }

    
    struct ::stat path_stat;  
    if (::stat(path.c_str(), &path_stat) != 0) {  
        return false;
    }

    if (!S_ISDIR(path_stat.st_mode)) {
        return false;
    }

    // Проверяем права на запись
    if (access(path.c_str(), W_OK) != 0) {
        return false;
    }

    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return false;
    }

    bool has_files = false;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            has_files = true;
            break;
        }
    }
    closedir(dir);

    if (!has_files) {
        return false;
    }

    return true;
}

// Метод ожидания подключения USB-накопителя
std::string EncryptionApp::waitForUsbDrive(const std::string& message, int timeout_seconds) {
    auto start_time = std::chrono::steady_clock::now();
    std::set<std::string> initial_devices;
    
    // Получаем начальный список устройств
    auto initial_state = getConnectedDevices();
    for (const auto& device : initial_state) {
        if (device.is_mounted) {
            initial_devices.insert(device.mount_point);
        }
    }

    display.clearScreen(COLOR_BLACK);

    // Разбиваем сообщение на строки
    std::vector<std::string> message_lines;
    std::string line;
    std::istringstream iss(message);
    while (std::getline(iss, line)) {
        message_lines.push_back(line);
    }
    
    if (message_lines.empty()) {
        message_lines.push_back(message);
    }

    // Вычисляем размеры с учетом ограничений экрана
    int16_t total_height = message_lines.size() * (CYRILLIC_FONT.height + 5);
    int16_t start_y = (display.getHeight() - total_height) / 2;
    if (start_y < 10) start_y = 10; // Минимальный отступ сверху

    // Находим максимальную ширину текста
    int16_t max_width = 0;
    for (const auto& line : message_lines) {
        int16_t line_width = line.length() * (CYRILLIC_FONT.width + 2);
        if (line_width > max_width) max_width = line_width;
    }

    // Ограничиваем размеры рамки размерами экрана
    int16_t frame_x = (display.getWidth() - (max_width + 20)) / 2;
    if (frame_x < 5) frame_x = 5;
    int16_t frame_y = start_y - 10;
    if (frame_y < 5) frame_y = 5;
    
    int16_t frame_width = max_width + 20;
    if (frame_width > display.getWidth() - 10) 
        frame_width = display.getWidth() - 10;
    
    int16_t frame_height = total_height + 20;
    if (frame_height > display.getHeight() - 10) 
        frame_height = display.getHeight() - 10;

    // Анимация
    int frame_animation = 0;
    const int ANIMATION_STEPS = 4;
    bool is_red = false;
    auto last_color_switch = std::chrono::steady_clock::now();
    
    auto drawAnimatedFrame = [&](int step) {
        // Рисуем основную рамку с мигающим цветом
        uint16_t current_color = is_red ? COLOR_RED : COLOR_GREEN;
        display.drawRect(frame_x, frame_y, frame_width, frame_height, current_color);
        
        // Анимируем только один угол за раз
        switch (step % ANIMATION_STEPS) {
            case 0:
                display.fillRect(frame_x - 1, frame_y - 1, 3, 3, current_color);
                break;
            case 1:
                display.fillRect(frame_x + frame_width - 2, frame_y - 1, 3, 3, current_color);
                break;
            case 2:
                display.fillRect(frame_x + frame_width - 2, frame_y + frame_height - 2, 3, 3, current_color);
                break;
            case 3:
                display.fillRect(frame_x - 1, frame_y + frame_height - 2, 3, 3, current_color);
                break;
        }
    };

    // Отрисовываем текст
    int16_t current_y = start_y;
    for (const auto& line : message_lines) {
        drawCurrentFile(line, -1, current_y, COLOR_GREEN);
        current_y += CYRILLIC_FONT.height + 5;
    }

    auto last_animation_time = std::chrono::steady_clock::now();

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        
        if (elapsed_seconds >= timeout_seconds) {
            return "";
        }

        // Переключение цвета каждые 500мс
        auto color_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_color_switch).count();
        if (color_elapsed >= 500) {
            is_red = !is_red;
            last_color_switch = current_time;
        }

        // Анимация рамки каждые 250мс
        auto animation_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_animation_time).count();
        if (animation_elapsed >= 250) {
            drawAnimatedFrame(frame_animation++);
            last_animation_time = current_time;
        }

        // Проверка устройств
        auto current_devices = getConnectedDevices();
        for (const auto& device : current_devices) {
            if (device.is_mounted && initial_devices.find(device.mount_point) == initial_devices.end()) {
                if (isUsbStorage(device.mount_point)) {
                    return device.mount_point;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void EncryptionApp::transferFiles(const std::string& source_path, const std::string& dest_path, bool encrypting) {

    display.clearScreen(COLOR_BLACK);
    display.drawRect(5, 5, display.getWidth() - 10, display.getHeight() - 10, COLOR_GREEN);
    std::string operation = encrypting ? "ЗАШИФРОВАНИЕ" : "РАСШИФРОВАНИЕ";
    drawCurrentFile(operation, -1, 15, COLOR_GREEN);
    display.drawLine(10, 30, display.getWidth() - 10, 30, COLOR_GREEN);
    
    // Проверяем существование директорий
    if (!std::filesystem::exists(source_path)) {
        throw std::runtime_error("Директория источника не найдена");
    }
    if (!std::filesystem::exists(dest_path)) {
        throw std::runtime_error("Директория назначения не найдена");
    }

    // Проверяем права на запись в директорию назначения
    if (access(dest_path.c_str(), W_OK) != 0) {
        throw std::runtime_error("Нет прав на запись");
    }
    
    size_t total_files = 0;
    for (const auto& file : file_list) if (file.selected) total_files++;
    size_t processed_files = 0;
    
    
    for (const auto& file : file_list) {
        if (!file.selected) continue;
        
        try {

            std::string dest_file;
            if (encrypting) {
                dest_file = dest_path + "/" + file.name + ".enc";
            } else {
                std::string filename = file.name;
                if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".enc") {
                    filename = filename.substr(0, filename.length() - 4);
                }
                dest_file = dest_path + "/" + filename;
            }
            
            
            // Удаляем существующий файл, если он есть
            if (std::filesystem::exists(dest_file)) {
                std::filesystem::remove(dest_file);
            }
            
            display.fillRect(10, 40, display.getWidth() - 20, 50, COLOR_BLACK);
            std::string progress = "Файл " + std::to_string(processed_files + 1) + "/" + std::to_string(total_files);
            drawCurrentFile(progress, -1, 45, COLOR_GREEN);
            
            std::string current_file = file.name;
            if (current_file.length() > 15) current_file = current_file.substr(0, 12) + "...";
            drawCurrentFile(current_file, -1, 65, COLOR_GREEN);
            
            int bar_x = 20, bar_y = 90, bar_w = display.getWidth() - 40, bar_h = 10;
            display.drawRect(bar_x, bar_y, bar_w, bar_h, COLOR_GREEN);
            int filled = (int)((processed_files + 1) * bar_w / total_files);
            display.fillRect(bar_x + 1, bar_y + 1, filled - 2, bar_h - 2, COLOR_GREEN);
            
            // Проверяем размер исходного файла
            std::filesystem::path src_path(file.full_path);
            if (!std::filesystem::exists(src_path)) {
                throw std::runtime_error("Исходный файл не найден: " + file.full_path);
            }
            auto file_size = std::filesystem::file_size(src_path);
            
            if (encrypting) {
                encryptFile(file.full_path, dest_file);
            } else {
                decryptFile(file.full_path, dest_file);
            }
            
            if (!std::filesystem::exists(dest_file)) {
                throw std::runtime_error("Файл не был создан: " + dest_file);
            }
            auto new_size = std::filesystem::file_size(dest_file);
            
            processed_files++;
            
        } catch (const std::exception& e) {
            display.fillRect(10, 40, display.getWidth() - 20, 50, COLOR_BLACK);
            drawCurrentFile("ОШИБКА:", -1, 45, COLOR_RED);
            std::string error_msg = e.what();
            if (error_msg.length() > 20) error_msg = error_msg.substr(0, 17) + "...";
            drawCurrentFile(error_msg, -1, 65, COLOR_RED);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    
    // Показываем сообщение о завершении
    display.clearScreen(COLOR_BLACK);
    drawCurrentFile("ЗАВЕРШЕНО", -1, display.getHeight() / 2 - 10, COLOR_GREEN);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Сбрасываем состояние и возвращаемся в главное меню
    waiting_for_key = false;
    file_list.clear();
    current_file_index = 0;
    current_page = 0;
    
    // Принудительно очищаем экран и перерисовываем меню
    display.clearScreen(COLOR_BLACK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    drawMenu();
}


void EncryptionApp::showUsbWaitScreen(const std::string& message) {
    display.clearScreen(COLOR_BLACK);
    display.drawRect(5, 5, display.getWidth() - 10, display.getHeight() - 10, COLOR_GREEN);
    drawCurrentFile(message, -1, display.getHeight() / 2 - 10, COLOR_GREEN);
}

