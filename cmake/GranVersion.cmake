# ============================================================================
#  Gran UI — чтение номера версии из include/gran/gran_version.h
#  Gran UI — reading the version number from include/gran/gran_version.h
#
#  Номер версии записан только в заголовке (единый источник); здесь он разбирается
#  до project(), чтобы project(VERSION ...) и программы CMake видели то же число.
#  The version number is written only in the header (a single source); here it is parsed
#  before project() so that project(VERSION ...) and CMake programs see the same number.
# ============================================================================

# Чтение одного числа GRAN_VERSION_<part> из заголовка в переменную out.
# Reads one GRAN_VERSION_<part> number from the header into the variable out.
function(gran_read_version_part header part out)
    file(STRINGS "${header}" line REGEX "^#define GRAN_VERSION_${part} [0-9]+$")
    if(NOT line)
        message(FATAL_ERROR "gran_version.h: GRAN_VERSION_${part} not found")
    endif()
    string(REGEX REPLACE "^#define GRAN_VERSION_${part} ([0-9]+)$" "\\1" value "${line}")
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

# Чтение всей версии: GRAN_VERSION (X.Y.Z) и GRAN_VERSION_SUFFIX ("-dev" или пусто).
# Reads the whole version: GRAN_VERSION (X.Y.Z) and GRAN_VERSION_SUFFIX ("-dev" or empty).
function(gran_read_version header)
    gran_read_version_part("${header}" MAJOR major)
    gran_read_version_part("${header}" MINOR minor)
    gran_read_version_part("${header}" PATCH patch)
    file(STRINGS "${header}" line REGEX "^#define GRAN_VERSION_PRERELEASE \"[^\"]*\"$")
    string(REGEX REPLACE "^#define GRAN_VERSION_PRERELEASE \"([^\"]*)\"$" "\\1" suffix "${line}")
    set(GRAN_VERSION "${major}.${minor}.${patch}" PARENT_SCOPE)
    set(GRAN_VERSION_SUFFIX "${suffix}" PARENT_SCOPE)
endfunction()
