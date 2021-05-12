clang --debug -I ./includes -I external -o sviggy.exe `
    src/sviggy.cpp `
    src/svg.cpp `
    src/utils.cpp `
    src/dx_state.cpp `
    src/shapes.cpp `
    src/view.cpp `
    src/vec.cpp `
    src/document.cpp `
    src/application.cpp `
    external/pugixml.cpp `
    external/imgui_demo.cpp `
    external/imgui_impl_dx11.cpp `
    external/imgui_impl_win32.cpp `
    external/imgui_tables.cpp `
    external/imgui_widgets.cpp `
    external/imgui.cpp `
    external/imgui_draw.cpp