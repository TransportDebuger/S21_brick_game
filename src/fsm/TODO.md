# FSM Library - TODO List

## Makefile и CMakeLists.txt

- [x] Убедиться, что цель `all` в CMakeLists.txt зависит от `test`, `build`, `dvi`, `install_target`
- [x] Переименовать `run-tests` в `test` в CMakeLists.txt для соответствия Makefile
- [x] Переименовать `all-build` в `all` в CMakeLists.txt
- [x] Переименовать `clean_build` в `clean` в CMakeLists.txt
- [x] Убедиться, что цель `install` в CMakeLists.txt копирует и `.a`, и `.so` файлы
- [x] Обновить сообщение в цели `help` в CMakeLists.txt, чтобы оно точно соответствовало Makefile
- [x] Убедиться, что переменные окружения `INSTALLDIR`, `LIBTYPE`, `CC`, `CFLAGS` корректно обрабатываются в CMakeLists.txt

## Тестирование

- [x] Убедиться, что цель `test` в CMakeLists.txt запускает `linter-test`, `unit-test`, `mem-test`, `gcov-report`
- [x] Проверить, что `unit-test` действительно собирает и запускает тесты
- [x] Проверить, что `mem-test` использует valgrind
- [x] Проверить, что `gcov-report` генерирует отчёт о покрытии

## Сборка

- [x] Убедиться, что цель `build` в CMakeLists.txt корректно обрабатывает `LIBTYPE=static` и `LIBTYPE=dynamic`
- [x] Проверить, что для динамической библиотеки используется флаг `-fPIC`
- [x] Убедиться, что цель `build` использует правильные флаги компиляции

## Документация

- [x] Убедиться, что цель `dvi` в CMakeLists.txt использует переменную `DOCDIR`
- [x] Проверить, что `dvi` генерирует документацию Doxygen

## Установка и удаление

- [x] Убедиться, что цель `install` копирует заголовочные файлы в `${INSTALLDIR}/include`
- [x] Убедиться, что цель `uninstall` удаляет всю установленную директорию

## Прочие

- [x] Проверить, что все пути в CMakeLists.txt абсолютные и корректные
- [x] Убедиться, что CMakeLists.txt не использует зарезервированные имена целей для кастомных целей
- [x] Проверить, что поведение всех целей в CMakeLists.txt идентично Makefile
- [ ] Провести финальное тестирование всех целей