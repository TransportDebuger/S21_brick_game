message(STATUS "Patching Doxyfile: INPUT='${INPUT}' OUTPUT='${OUTPUT}' DOCDIR='${DOCDIR}'")

file(READ ${INPUT} CONTENTS)

get_filename_component(ABS_DOCDIR "${DOCDIR}" ABSOLUTE)
message(STATUS "Resolved DOCDIR as absolute path: ${ABS_DOCDIR}")

# Удаляем все строки, содержащие OUTPUT_DIRECTORY =
# Используем более широкий паттерн, чтобы поймать любые варианты
string(REGEX REPLACE 
    "^[ \\t]*OUTPUT_DIRECTORY[ \\t]*=.*$\n?" 
    "" 
    CONTENTS 
    "${CONTENTS}"
)

# Добавляем строку один раз
string(APPEND CONTENTS "OUTPUT_DIRECTORY = ${ABS_DOCDIR}\n")

# Записываем обратно
file(WRITE ${OUTPUT} "${CONTENTS}")

message(STATUS "Successfully patched Doxyfile -> ${OUTPUT}")
message(STATUS "OUTPUT_DIRECTORY set to: ${ABS_DOCDIR}")