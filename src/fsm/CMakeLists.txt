cmake_minimum_required(VERSION 3.16)
project(s21_fsm C)

#Выявление платформы
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    set(PLATFORM_NAME "Linux")
elseif(APPLE)
    set(MACOS TRUE)
    set(PLATFORM_NAME "macOS")
else()
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
endif()

# Настройки проекта
# Имена целей и библиотек (без префикса "lib" - CMake добавит сам)
set(PROJECT_LIB_NAME "s21_fsm")
set(LIB_FILE_NAME "${PROJECT_LIB_NAME}")

# Опции проекта - могут быть переопределены родительским CMake
# Использование: cmake -DS21_FSM_BUILD_SHARED=ON ..
# или в родительском CMakeLists.txt:
# set(S21_FSM_BUILD_SHARED ON)
# add_subdirectory(libs/s21_fsm)
option(S21_FSM_BUILD_SHARED "Build s21_fsm as shared library" OFF)
option(S21_FSM_BUILD_TESTS "Build tests for s21_fsm" ON)
option(S21_FSM_INSTALL_HEADERS "Install header files" ON)
if(S21_FSM_BUILD_SHARED)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

# Использование стандартных путей установки (GNUInstallDirs)
include(GNUInstallDirs)

# Если это корневой проект - используем кастомные пути
# Если подпроект - используем стандартные CMAKE пути
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Это корневой проект
    set(S21_FSM_DOCDIR CMAKE_CURRENT_SOURCE_DIR/docs CACHE PATH "Documentation directory")
    set(COVERAGEDIR "${TESTSRCDIR}/coverage" CACHE PATH "Coverage reports directory")
else()
    set(S21_FSM_DOCDIR CMAKE_CURRENT_SOURCE_DIR/docs)
    set(COVERAGEDIR "${TESTSRCDIR}/coverage")
endif()

# Тип библиотеки: STATIC или SHARED (может быть переопределено: cmake -DLIBTYPE=SHARED)
set(LIBTYPE "STATIC" CACHE STRING "Library type: STATIC or SHARED")
string(TOUPPER "${LIBTYPE}" LIBTYPE)
if(NOT LIBTYPE MATCHES "^(STATIC|SHARED)$")
    message(FATAL_ERROR "Unknown library type: ${LIBTYPE}. Use LIBTYPE=STATIC or LIBTYPE=SHARED")
endif()

# Сбор исходных фалов
# Используем CMAKE_CURRENT_SOURCE_DIR для локальных путей (важно для подпроектов!)
file(GLOB S21_FSM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB S21_FSM_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
# исключаем файлы тестов
list(FILTER S21_FSM_SOURCES EXCLUDE REGEX "test_*")
# Проверка что найдены исходные файлы
if(NOT S21_FSM_SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ОСНОВНАЯ ЦЕЛЬ БИБЛИОТЕКИ (Library Target)
add_library(${PROJECT_LIB_NAME} ${LIB_TYPE} ${S21_FSM_SOURCES})
# Установка свойств цели
set_target_properties(${PROJECT_LIB_NAME} 
                      PROPERTIES OUTPUT_NAME ${PROJECT_LIB_NAME} 
                      C_STANDARD 11
                      C_STANDARD_REQUIRED ON
                      C_EXTENSIONS OFF)

# Флаги компиляции ТОЛЬКО для этой цели (не загрязняем глобальную конфигурацию!)
# Это критически важно для оркестрации - родительский проект может иметь
# другие флаги, и мы их не должны переписывать
target_compile_options(${PROJECT_LIB_NAME} PRIVATE
                        -Wall
                        -Wextra
                        -Werror
                        -std=c11
                        -pedantic)

# Дополнительные флаги для SHARED библиотеки (только для Unix-систем)
if(S21_FSM_BUILD_SHARED AND UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_LIB_NAME} PRIVATE -fPIC)
endif()

# КЛЮЧЕВОЙ МОМЕНТ для оркестрации:
# PUBLIC - означает, что потребители библиотеки (другие цели)
# будут иметь доступ к этим заголовкам
# BUILD_INTERFACE - пути используются при сборке в текущем проекте
# INSTALL_INTERFACE - пути используются после установки
target_include_directories(${PROJECT_LIB_NAME} PUBLIC 
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# СПЕЦИАЛЬНАЯ ВЕРСИЯ БИБЛИОТЕКИ ДЛЯ ТЕСТИРОВАНИЯ С ПОКРЫТИЕМ (Coverage)
file(GLOB S21_FSM_COVERAGE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
list(FILTER S21_FSM_COVERAGE_SOURCES EXCLUDE REGEX "test_")
add_library(${PROJECT_LIB_NAME}_cov STATIC ${S21_FSM_COVERAGE_SOURCES})
set_target_properties(${PROJECT_LIB_NAME}_cov PROPERTIES
                        C_STANDARD 11
                        C_STANDARD_REQUIRED ON
                        C_EXTENSIONS OFF)
target_compile_options(${PROJECT_LIB_NAME}_cov PRIVATE
                        -Wall -Wextra -Werror -std=c11 -pedantic --coverage -g)
target_link_options(${PROJECT_LIB_NAME}_cov PRIVATE --coverage)
target_include_directories(${PROJECT_LIB_NAME}_cov PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}
)
# Помечаем как библиотеку с покрытием (для тестирования)
set_target_properties(${PROJECT_LIB_NAME}_cov PROPERTIES
EXCLUDE_FROM_DEFAULT_BUILD TRUE
)

# Тестирование (опционально)
if(S21_FSM_BUILD_TESTS)
    enable_testing()

    # Пути для тестов
    set(S21_FSM_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set(S21_FSM_TEST_SOURCES "${S21_FSM_TEST_DIR}/test_fsm.c")
    set(S21_FSM_TEST_EXEC "${PROJECT_LIB_NAME}_test_runner")

    # Поиск зависимостей для тестирования
    find_package(PkgConfig QUIET)

    set(S21_FSM_TEST_LIBS "")

    if(PKG_CONFIG_FOUND)
        pkg_check_modules(CHECK check QUIET)
        if(CHECK_FOUND)
            set(S21_FSM_TEST_LIBS ${CHECK_LIBRARIES})
        endif()
    endif()

    # Если pkg-config не нашел, попробуем стандартные библиотеки
    if(NOT S21_FSM_TEST_LIBS)
        if(${MACOS})
            set(S21_FSM_TEST_LIBS "-lcheck;-lm;-lpthread")
        elseif(${LINUX})
            set(S21_FSM_TEST_LIBS "-lcheck; -lsubunit; -lm; -lrt; -lpthread")
        endif()
    endif()

    # Проверяем наличие тестового файла
    if(EXISTS "${S21_FSM_TEST_SOURCES}")
    
        # Цель для запуска тестов
        add_executable(${S21_FSM_TEST_EXEC} ${S21_FSM_TEST_SOURCES})
    
        set_target_properties(${S21_FSM_TEST_EXEC} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
        )
    
        # Флаги компиляции для тестов
        target_compile_options(${S21_FSM_TEST_EXEC} PRIVATE
            -Wall -Wextra -Werror -std=c11 -pedantic
            --coverage 
            -g
        )
    
        target_include_directories(${S21_FSM_TEST_EXEC} PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}
        )

        target_link_options(${PROJECT_LIB_NAME}_test_runner PRIVATE --coverage)
    
        # Линковка: используем версию с покрытием для тестов
        target_link_libraries(${S21_FSM_TEST_EXEC} PRIVATE 
            ${PROJECT_LIB_NAME}_cov
            ${S21_FSM_TEST_LIBS}
        )
    
        # Цель для запуска тестов (с уникальным именем s21_fsm_test)
        add_custom_target(${PROJECT_LIB_NAME}_unit_test
            COMMAND ${CMAKE_COMMAND} -E echo "Running ${PROJECT_LIB_NAME} unit tests..."
            COMMAND $<TARGET_FILE:${S21_FSM_TEST_EXEC}>
            DEPENDS ${S21_FSM_TEST_EXEC}
            COMMENT "Executing unit tests for ${PROJECT_LIB_NAME}"
        )
    
        # Регистрация теста в CTest (стандартный механизм CMake)
        add_test(
            NAME ${PROJECT_LIB_NAME}_unit_test 
            COMMAND $<TARGET_FILE:${S21_FSM_TEST_EXEC}>
        )
    
        # Цель для отчета о покрытии (если найдены lcov и genhtml)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
    
        if(LCOV_PATH AND GENHTML_PATH)
            add_custom_target(${PROJECT_LIB_NAME}_coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${S21_FSM_COVERAGEDIR}/report
                COMMAND ${LCOV_PATH} 
                    --capture 
                    --directory ${CMAKE_CURRENT_BINARY_DIR}
                    --output-file ${S21_FSM_COVERAGEDIR}/coverage.info
                COMMAND ${GENHTML_PATH} 
                    ${S21_FSM_COVERAGEDIR}/coverage.info 
                    --output-directory ${S21_FSM_COVERAGEDIR}/report/
                COMMAND ${CMAKE_COMMAND} -E echo 
                    "Coverage report: file://${S21_FSM_COVERAGEDIR}/report/index.html"
                DEPENDS ${PROJECT_LIB_NAME}_test
                COMMENT "Generating coverage report for ${PROJECT_LIB_NAME}"
            )
        endif()
    
    else()
        message(WARNING "Test file not found: ${S21_FSM_TEST_SOURCES}")
        message(WARNING "Skipping test target for ${PROJECT_LIB_NAME}")
    endif()

endif()

# ПРОВЕРКА СТИЛЯ КОДА И СТАТИЧЕСКИЙ АНАЛИЗ
find_program(CLFORMAT_PATH clang-format)
find_program(CPPCHECK_PATH cppcheck)
if(CLFORMAT_PATH)
    add_custom_target(${PROJECT_LIB_NAME}_format_check
        COMMENT "Checking code style with clang-format..."
        COMMAND ${CMAKE_COMMAND} -E echo "Formatting check for ${PROJECT_LIB_NAME}"
        COMMAND ${CLFORMAT_PATH} --style=Google --dry-run ${S21_FSM_SOURCES} ${S21_FSM_INCLUDES}
    )
endif()
if(CPPCHECK_PATH)
    add_custom_target(${PROJECT_LIB_NAME}_cppcheck
        COMMENT "Running static analysis with cppcheck..."
        COMMAND ${CPPCHECK_PATH}
        --enable=all
        --suppress=missingIncludeSystem
        --suppress=unusedFunction
        --language=c
        --std=c11
        --error-exitcode=1
        ${S21_FSM_SOURCES} ${S21_FSM_INCLUDES}
    )
endif()

# Комбинированная цель для линтеров (если они найдены)
if(CLFORMAT_PATH AND CPPCHECK_PATH)
    add_custom_target(${PROJECT_LIB_NAME}_linter
        DEPENDS ${PROJECT_LIB_NAME}_format_check ${PROJECT_LIB_NAME}_cppcheck
        COMMENT "All linters passed for ${PROJECT_LIB_NAME}"
    )
endif()

# ПРОВЕРКА ПАМЯТИ (VALGRIND)
find_program(VALGRIND_PATH valgrind)
if(VALGRIND_PATH AND S21_FSM_BUILD_TESTS)
    add_custom_target(${PROJECT_LIB_NAME}_memcheck
        COMMAND ${CMAKE_COMMAND} -E echo "Running memory checks with Valgrind..."
        COMMAND ${VALGRIND_PATH} --leak-check=full --error-exitcode=1 $<TARGET_FILE:${S21_FSM_TEST_EXEC}>
        DEPENDS ${S21_FSM_TEST_EXEC}
        COMMENT "Memory leak detection for ${PROJECT_LIB_NAME}"
)
endif()

# ДОКУМЕНТАЦИЯ (DOXYGEN)
find_program(DOXYGEN_PATH doxygen)
if(DOXYGEN_PATH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    add_custom_target(${PROJECT_LIB_NAME}_docs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${S21_FSM_DOCDIR}
        COMMAND ${DOXYGEN_PATH} Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation for ${PROJECT_LIB_NAME}"
    )
endif()

# УСТАНОВКА (для make install)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Только если это корневой проект
    # Установка библиотеки
    install(TARGETS ${PROJECT_LIB_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Установка заголовков
    if(S21_FSM_INSTALL_HEADERS)
        install(FILES ${S21_FSM_INCLUDES}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()

    # Экспорт для других CMake-проектов (advanced feature)
    install(TARGETS ${PROJECT_LIB_NAME}
        EXPORT ${PROJECT_LIB_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(EXPORT ${PROJECT_LIB_NAME}Targets
        FILE ${PROJECT_LIB_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_LIB_NAME}
    )
endif()

#ИНФОРМАЦИЯ О КОНФИГУРАЦИИ
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Выводим информацию только если это корневой проект
    message(STATUS "")
    message(STATUS "=================================================")
    message(STATUS "Project: ${PROJECT_LIB_NAME}")
    message(STATUS "Library Type: ${LIB_TYPE}")
    message(STATUS "Platform: ${PLATFORM_NAME}")
    message(STATUS "=================================================")
    message(STATUS "Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "Build Dir: ${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "Install Dir: ${CMAKE_INSTALL_PREFIX}")
    if(S21_FSM_BUILD_TESTS)
        message(STATUS "Test Exec: ${S21_FSM_TEST_EXEC}")
    endif()
    if(DOXYGEN_PATH)
        message(STATUS "Docs Dir: ${S21_FSM_DOCDIR}")
    endif()
    message(STATUS "=================================================")
    message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "=================================================")
    message(STATUS "Available targets:")
    message(STATUS " ${PROJECT_LIB_NAME} (default) - Compile library")
    if(S21_FSM_BUILD_TESTS)
        message(STATUS " ${PROJECT_LIB_NAME}_unit_test - Run unit tests")
        if(LCOV_PATH AND GENHTML_PATH)
            message(STATUS " ${PROJECT_LIB_NAME}_coverage - Generate coverage report")
        endif()
    endif()
    if(VALGRIND_PATH)
        message(STATUS " ${PROJECT_LIB_NAME}_memcheck - Memory leak detection")
    endif()
    if(CLFORMAT_PATH AND CPPCHECK_PATH)
        message(STATUS " ${PROJECT_LIB_NAME}_linter - Code style and static analysis")
    endif()
    if(DOXYGEN_PATH)
        message(STATUS " ${PROJECT_LIB_NAME}_docs - Generate documentation")
    endif()
    message(STATUS "=================================================")
    message(STATUS "")
else()
# Для подпроектов выводим краткую информацию
    message(STATUS "Added subproject: {LIB_TYPE})")
endif()