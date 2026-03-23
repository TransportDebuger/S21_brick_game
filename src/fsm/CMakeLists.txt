cmake_minimum_required(VERSION 3.16)
project(s21_fsm C)

# --- Подключаем стандартные пути установки ---
include(GNUInstallDirs)

# --- Настройки проекта ---
set(PROJECTNAME s21_fsm)
set(LIBNAME lib${PROJECTNAME})

# --- Пути по умолчанию (переопределяются через окружение) ---
set(DOCDIR_DEFAULT ${CMAKE_CURRENT_SOURCE_DIR}/doc)
set(INSTALLDIR_DEFAULT ../build/lib)

set(DOCDIR $ENV{DOCDIR})
if(NOT DOCDIR)
    set(DOCDIR ${DOCDIR_DEFAULT})
endif()

set(INSTALLDIR $ENV{INSTALLDIR})
if(NOT INSTALLDIR)
    set(INSTALLDIR ${INSTALLDIR_DEFAULT})
endif()

set(LIBTYPE $ENV{LIBTYPE})
if(NOT LIBTYPE)
    set(LIBTYPE static)
endif()

# --- Платформа ---
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
elseif(APPLE)
    set(MACOS TRUE)
endif()

# --- Исходные файлы ---
set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES ${SRCDIR}/s21_fsm.c)
set(INCLUDES ${SRCDIR}/s21_fsm.h)
set(TESTS_DIR ${SRCDIR}/tests)
set(TESTS_SRC ${TESTS_DIR}/test_fsm.c)

# --- Папки сборки ---
set(BUILDDIR ${SRCDIR}/build)
set(OBJDIR ${SRCDIR}/obj)
set(OBJDIR_COV ${SRCDIR}/obj_cov)
set(TESTBUILDDIR ${TESTS_DIR}/build)
set(COVERAGEDIR ${TESTS_DIR}/coverage)

# --- Инструменты ---
find_program(CC NAMES gcc clang)
find_program(CLANG_FORMAT clang-format)
find_program(CPPCHECK cppcheck)
find_program(VALGRIND valgrind)
find_program(LCOV lcov)
find_program(GENHTML genhtml)
find_program(DOXYGEN doxygen)
find_program(PACKAGE_MANAGER NAMES apt-get yum dnf pacman brew)

# --- Флаги компиляции ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CFLAGS -Wall -Wextra -Werror -std=c11 -pedantic -I${SRCDIR})
set(CFLAGS_SHARED ${CFLAGS} -fPIC)
set(COVERAGE_CFLAGS ${CFLAGS} --coverage -g)
set(COVERAGE_LDFLAGS --coverage)

# --- Тестовые библиотеки ---
if(LINUX)
    set(TEST_LIBS -lcheck -lsubunit -lm -lrt -lpthread)
elseif(MACOS)
    set(TEST_LIBS -lcheck -lm -lpthread)
endif()

# --- Основная библиотека ---
add_library(${PROJECTNAME}_static STATIC ${SOURCES})
set_target_properties(${PROJECTNAME}_static PROPERTIES
    OUTPUT_NAME ${LIBNAME}
    ARCHIVE_OUTPUT_DIRECTORY ${BUILDDIR}
    LIBRARY_OUTPUT_DIRECTORY ${BUILDDIR}
    RUNTIME_OUTPUT_DIRECTORY ${BUILDDIR}
)
target_compile_options(${PROJECTNAME}_static PRIVATE ${CFLAGS})
target_include_directories(${PROJECTNAME}_static PUBLIC ${SRCDIR})

if(LIBTYPE STREQUAL "dynamic")
    add_library(${PROJECTNAME}_shared SHARED ${SOURCES})
    set_target_properties(${PROJECTNAME}_shared PROPERTIES
        OUTPUT_NAME ${LIBNAME}
        LIBRARY_OUTPUT_DIRECTORY ${BUILDDIR}
        RUNTIME_OUTPUT_DIRECTORY ${BUILDDIR}
    )
    target_compile_options(${PROJECTNAME}_shared PRIVATE ${CFLAGS_SHARED})
target_include_directories(${PROJECTNAME}_shared PUBLIC ${SRCDIR})
endif()

# --- Цель: build ---
add_custom_target(build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILDDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJDIR}
    COMMAND ${CC} ${CFLAGS} -c ${SOURCES} -o ${OBJDIR}/s21_fsm.o
    COMMAND ${CMAKE_COMMAND} -E remove -f ${BUILDDIR}/${LIBNAME}.a ${BUILDDIR}/${LIBNAME}.so
    COMMAND ${CMAKE_COMMAND} -E remove -f ${BUILDDIR}/${LIBNAME}_cov.a
    COMMAND ${CMAKE_COMMAND} -E chdir ${OBJDIR} ${CMAKE_AR} rcs ${BUILDDIR}/${LIBNAME}.a s21_fsm.o
    COMMAND ${CMAKE_COMMAND} -E echo "Building ${LIBTYPE} library for ${PROJECTNAME}"
    BYPRODUCTS ${BUILDDIR}/${LIBNAME}.a
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building ${LIBTYPE} library for ${PROJECTNAME}"
)

if(LIBTYPE STREQUAL "dynamic")
    add_custom_command(TARGET build POST_BUILD
        COMMAND ${CC} -shared ${OBJDIR}/s21_fsm.o -o ${BUILDDIR}/${LIBNAME}.so
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Creating shared library ${LIBNAME}.so"
    )
endif()

# --- Библиотека с покрытием ---
add_custom_target(build-cov
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJDIR_COV}
    COMMAND ${CC} ${COVERAGE_CFLAGS} -c ${SOURCES} -o ${OBJDIR_COV}/s21_fsm.o
    COMMAND ${CMAKE_COMMAND} -E chdir ${OBJDIR_COV} ${CMAKE_AR} rcs ${BUILDDIR}/${LIBNAME}_cov.a s21_fsm.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building coverage library"
    BYPRODUCTS ${BUILDDIR}/${LIBNAME}_cov.a
)

# --- Тесты ---
if(EXISTS ${TESTS_SRC})
    enable_testing()

    # Цель: unit-test-build
    add_custom_target(unit-test-build
        DEPENDS build-cov
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TESTBUILDDIR}
        COMMAND ${CC} ${COVERAGE_CFLAGS} -c ${TESTS_SRC} -o ${TESTBUILDDIR}/test_fsm.o
        COMMAND ${CC} ${COVERAGE_CFLAGS} ${TESTBUILDDIR}/test_fsm.o ${BUILDDIR}/${LIBNAME}_cov.a ${TEST_LIBS} -o ${TESTBUILDDIR}/test_runner
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building test runner"
        BYPRODUCTS ${TESTBUILDDIR}/test_runner
    )

    # Цель: unit-test-run
    add_custom_target(unit-test-run
        DEPENDS unit-test-build
        COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${BUILDDIR}:$ENV{LD_LIBRARY_PATH} ${TESTBUILDDIR}/test_runner
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running unit tests"
    )

    add_custom_target(unit-test
        DEPENDS unit-test-build unit-test-run
        COMMENT "Running all unit tests"
    )

    # Linters
    if(CLANG_FORMAT)
        add_custom_target(clang-format
            COMMAND ${CLANG_FORMAT} --style=Google --dry-run ${SOURCES} ${INCLUDES}
            COMMENT "Checking code style"
        )
    endif()

    if(CPPCHECK)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK} --enable=all --suppress=missingIncludeSystem
                    --suppress=unusedFunction --language=c --std=c11
                    --error-exitcode=1 ${SOURCES} ${INCLUDES}
            COMMENT "Running static analysis"
        )
    endif()

    add_custom_target(linter-test
        DEPENDS clang-format cppcheck
        COMMENT "Running linters"
    )

    # Valgrind
    if(VALGRIND)
        add_custom_target(mem-test
            DEPENDS unit-test-build
            COMMAND ${VALGRIND} --leak-check=full --show-leak-kinds=all
                    --track-origins=yes --error-exitcode=1
                    ${TESTBUILDDIR}/test_runner
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Memory check with Valgrind"
        )
    endif()

    # Отчёт о покрытии
    if(LCOV AND GENHTML)
        # Цель gcov-report была перемещена на уровень выше, чтобы не зависеть от unit-test-build
    endif()

    # Цель: run-tests
    add_custom_target(run-tests
        DEPENDS linter-test unit-test mem-test gcov-report
        COMMENT "Running all tests"
    )

    add_custom_target(gcov-report
        DEPENDS unit-test-build
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGEDIR}
        COMMAND ${LCOV} --capture --directory ${OBJDIR_COV} --output-file ${COVERAGEDIR}/coverage.info
        COMMAND ${GENHTML} ${COVERAGEDIR}/coverage.info --output-directory ${COVERAGEDIR}/report/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating coverage report"
    )
endif()

# --- Документация (dvi) ---
if(DOXYGEN AND EXISTS "${SRCDIR}/Doxyfile")
    set(DOXYFILE_IN "${SRCDIR}/Doxyfile")
    set(DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile.tmp")

    # Временный файл, зависящий от DOCDIR
    set(DOCDIR_TRACKER "${CMAKE_BINARY_DIR}/.docdir_tracker")

    # Создаём "трекер" изменения DOCDIR
    file(WRITE "${DOCDIR_TRACKER}.in" "DOCDIR=${DOCDIR}\n")
    configure_file("${DOCDIR_TRACKER}.in" "${DOCDIR_TRACKER}" COPYONLY)

    # Генерируем Doxyfile.tmp
    add_custom_command(
        OUTPUT ${DOXYFILE_OUT}
        COMMAND ${CMAKE_COMMAND} -E copy "${DOXYFILE_IN}" "${DOXYFILE_OUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DOCDIR}"
        COMMAND ${CMAKE_COMMAND} -D "INPUT=${DOXYFILE_OUT}" -D "OUTPUT=${DOXYFILE_OUT}" -D "DOCDIR=${DOCDIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_doxyfile.cmake"
        DEPENDS "${DOXYFILE_IN}" "${DOCDIR_TRACKER}"
        COMMENT "Configuring Doxyfile: OUTPUT_DIRECTORY = ${DOCDIR}"
        VERBATIM
    )

    # Цель dvi зависит от сгенерированного файла
    add_custom_target(dvi
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCDIR}
        COMMAND ${DOXYGEN} "${DOXYFILE_OUT}"
        WORKING_DIRECTORY "${SRCDIR}"
        DEPENDS ${DOXYFILE_OUT}
        COMMENT "Generating documentation with Doxygen"
    )
endif()

# --- Установка файлов ---
# --- Установка файлов ---
install(FILES ${INCLUDES} DESTINATION ${INSTALLDIR}/include)
install(TARGETS ${PROJECTNAME}_static
    ARCHIVE DESTINATION ${INSTALLDIR}/bin
)
if(TARGET ${PROJECTNAME}_shared)
    install(TARGETS ${PROJECTNAME}_shared
        LIBRARY DESTINATION ${INSTALLDIR}/bin
    )
endif()

# --- Цель: install ---
add_custom_target(install_target ALL
    DEPENDS build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${INSTALLDIR}/include ${INSTALLDIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy ${INCLUDES} ${INSTALLDIR}/include/
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILDDIR}/${LIBNAME}.a ${INSTALLDIR}/bin/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUILDDIR}/${LIBNAME}.so ${INSTALLDIR}/bin/ || ${CMAKE_COMMAND} -E echo "No shared library to install"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Installing ${LIBNAME} to ${INSTALLDIR}"
)

# --- Удаление ---
add_custom_target(uninstall_target
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${INSTALLDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Uninstalling library"
)

# --- Очистка ---
add_custom_target(clean_build
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            ${OBJDIR} ${OBJDIR_COV} ${BUILDDIR} ${TESTBUILDDIR} ${COVERAGEDIR} ${DOCDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning build artifacts, keeping installed files"
)

# --- Полная сборка ---
add_custom_target(all-build
    DEPENDS run-tests build dvi
    COMMAND ${CMAKE_COMMAND} --build . --target install_target
    COMMENT "Full build: test, build, dvi, install"
)

# --- Справка ---
add_custom_target(help
    COMMAND ${CMAKE_COMMAND} -E echo "Доступные цели:\n  all-build     - полная сборка: тесты, сборка, документация, установка\n  build         - сборка библиотеки (LIBTYPE=static/dynamic)\n  install       - установка заголовков и библиотеки (INSTALLDIR=/path)\n  uninstall     - удаление установленной библиотеки\n  dvi           - генерация документации (Doxygen)\n  test          - запуск всех тестов: линтеры, юнит-тесты, valgrind, покрытие\n  linter-test   - проверка стиля (clang-format) и статический анализ (cppcheck)\n  unit-test     - сборка и запуск юнит-тестов\n  mem-test      - проверка утечек памяти через valgrind\n  gcov-report   - генерация отчёта о покрытии кода тестами\n  clean-all     - удаление всех сгенерированных файлов\n  help          - показать эту справку\n\nПеременные окружения:\n  LIBTYPE=static   - сборка статической библиотеки (по умолчанию)\n  LIBTYPE=dynamic  - сборка динамической библиотеки\n  INSTALLDIR=/path - установить каталог установки\n  DOCDIR=/path     - установить каталог генерации документации\n"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)