#!/bin/bash

# Проверка прав root
if [ "$EUID" -ne 0 ]; then 
    echo "Пожалуйста, запустите скрипт с правами root (sudo)"
    exit 1
fi

# Создание директорий для монтирования
echo "Создание директорий для монтирования..."
mkdir -p /media

# Копирование правил udev
echo "Установка правил автомонтирования USB..."
cp /etc/udev/rules.d/99-usb-mount.rules /etc/udev/rules.d/
chmod 644 /etc/udev/rules.d/99-usb-mount.rules
udevadm control --reload-rules
udevadm trigger

# Копирование и активация systemd сервиса
echo "Установка и активация сервиса автозапуска..."
cp /etc/systemd/system/shifro.service /etc/systemd/system/
chmod 644 /etc/systemd/system/shifro.service
systemctl daemon-reload
systemctl enable shifro.service
systemctl start shifro.service

echo "Установка завершена!"
echo "Статус сервиса:"
systemctl status shifro.service 