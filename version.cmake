
find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")
endif(GIT_FOUND)
execute_process(COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git
  describe --abbrev=0 --tags OUTPUT_VARIABLE OUTPUT)
string(REPLACE version- "" OUTPUT ${OUTPUT})
string(REPLACE "." ";" OUTPUT ${OUTPUT})
list(GET OUTPUT 0 VERSION_MAJOR)
list(GET OUTPUT 1 VERSION_MINOR)
list(GET OUTPUT 2 VERSION_REVISION)
string(STRIP ${VERSION_REVISION} VERSION_REVISION)