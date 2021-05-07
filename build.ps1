clang --debug -I ./includes -I external -o sviggy.exe `
    src/sviggy.cpp `
    src/svg.cpp `
    src/utils.cpp `
    src/d2_state.cpp `
    src/shapes.cpp `
    src/view.cpp `
    src/vec.cpp `
    external/pugixml.cpp