clang -I ./includes -I external -O3 -o sviggy-release.exe `
    src/sviggy.cpp `
    src/svg.cpp `
    src/dx_state.cpp `
    src/shapes.cpp `
    src/vec.cpp `
    src/application.cpp `
    src/bin_packing.cpp `
    src/pipeline.cpp `
    external/pugixml.cpp `
    external/imgui_demo.cpp `
    external/imgui_impl_dx11.cpp `
    external/imgui_impl_win32.cpp `
    external/imgui_tables.cpp `
    external/imgui_widgets.cpp `
    external/imgui.cpp `
    external/imgui_draw.cpp