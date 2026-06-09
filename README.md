# MediaSite

Веб-приложение для просмотра фильмов
## Зависимости
| Название                                       | Предназначение  |
|:-----------------------------------------------|:----------------|
| [Network](https://github.com/idDeqpt/Network)  | TCP сервер      |
| [WolfSSL](https://github.com/wolfSSL/wolfssl)  | TLS подключение |
____
# Сборка
## Требования
- CMake 3.14+
- C++17-совместимый компилятор
- Наличие TLS сертификатов
### Windows
```bash
# Вариант 1: Использовать готовый скрипт
GenerateRelease.bat
# Вариант 2: Вручную
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
