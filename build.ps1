clang --debug -I ./includes -I external -o sviggy.exe `
    src/sviggy.cpp `
    src/svg.cpp `
    src/utils.cpp `
    external/pugixml.cpp