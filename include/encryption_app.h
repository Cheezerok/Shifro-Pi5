#pragma once

#include "display_pi.h"
#include "kuznechik.h"
#include "keyboard.h"
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <atomic>

// Пины для подключения
#define RESET_PIN 25
#define DC_PIN 24
#define DISPLAY_WIDTH 160
#define DISPLAY_HEIGHT 128

// Структура для хранения информации о файле
struct FileInfo {
    std::string name;
    bool selected;
    std::string full_path;
};

// Структура для хранения информации об устройстве
struct DeviceInfo {
    std::string dev_path;    // Путь к устройству (/dev/sda1)
    std::string mount_point; // Точка монтирования (/media/sda1)
    bool is_mounted;        // Флаг монтирования
};

// Класс для работы с шифрованием
class EncryptionApp {
public:
    EncryptionApp();
    ~EncryptionApp();
    
    bool init();
    void run();
    
    void drawMenu();
    void drawMenuItem(int index, bool is_selected);
    void showMessage(const std::string& message, bool isError, int timeout_seconds);
    void drawCurrentFile(const std::string& fileName, int16_t x, int16_t y, uint16_t color);
    void drawFileSelectionMenu(bool force_full_redraw = false);
    void showUsbWaitScreen(const std::string& message);
    
private:
    // Опции меню
    enum class MenuOption {
        ENCRYPT = 0,
        DECRYPT = 1
    };
    
    // Дисплей
    TFTDisplay display;
    
    // Мембранная клавиатура
    MembraneKeyboard keyboard;
    
    // Меню
    size_t selected_item;
    std::vector<std::string> items;
    
    // Файловый менеджер
    std::vector<FileInfo> file_list;
    size_t current_file_index;
    size_t files_per_page;
    size_t current_page;
    
    // Клавиатура
    struct termios old_tio;
    
    // Ключ шифрования
    const std::string encryptionKey = "TEST_KEY";
    
    // Методы для работы с клавиатурой
    void setupTerminal();
    void resetTerminalSettings();
    int getch();
    bool kbhit();
    
    // Методы для работы с меню
    void handleKeypress(char key);
    void handleMembraneKeypress(int key);
    void processSelection(MenuOption option);
    
    // Методы для работы с файлами
    void loadFilesFromDrive(const std::string& path);
    void handleFileSelectionKeypress(int key);
    bool waitForDriveMount(const std::string& prompt);
    void processSelectedFiles(bool encrypting);
    void transferFiles(const std::string& source_path, const std::string& dest_path, bool encrypting);
    
    // Методы для работы с USB-накопителями
    std::vector<std::string> findUsbMountPoints();
    std::vector<DeviceInfo> getConnectedDevices();
    bool isUsbStorage(const std::string& path);
    std::string waitForUsbDrive(const std::string& message, int timeout_seconds);
    
    // Методы шифрования
    bool encryptFiles(const std::string& sourceDir, const std::string& targetDir);
    bool decryptFiles(const std::string& sourceDir, const std::string& targetDir);
    std::vector<char> encryptData(const std::vector<char>& data, const std::string& key);
    std::vector<char> decryptData(const std::vector<char>& data, const std::string& key);
    void encryptFile(const std::string& source_file, const std::string& dest_file);
    void decryptFile(const std::string& source_file, const std::string& dest_file);
    
    std::vector<unsigned char> calculateHMAC(const std::vector<char>& data, const std::string& key);
    std::vector<unsigned char> simpleHash(const std::vector<unsigned char>& data);
    // Получение путей для USB-накопителей
    std::pair<std::string, std::string> getStoragePaths();
    
    // Проверка наличия физического устройства
    bool checkDeviceExists(const std::string& device);
    
    // Флаг ожидания нажатия кнопки
    std::atomic<bool> waiting_for_key;
}; 