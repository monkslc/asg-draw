#include <math.h>

#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"

void LoadSVGFile(char *file, Document *doc) {
    pugi::xml_document xml;
    pugi::xml_parse_result result = xml.load_file(file);

    pugi::xml_node svg_tree = xml.child("svg");
    ViewPort viewport = ViewPort(&svg_tree);

    pugi::xml_object_range<pugi::xml_node_iterator> nodes = svg_tree.children();
    AddNodesToDocument(&viewport, nodes, doc);
}

void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc) {
    for (pugi::xml_node_iterator node : nodes) {
        if (strcmp(node->name(), "defs") == 0) {
            continue;
        }

        if (strcmp(node->name(), "metadata") == 0) {
            continue;
        }

        if (strcmp(node->name(), "g") == 0) {
            AddNodesToDocument(viewport, node->children(), doc);
            continue;
        }

        if (strcmp(node->name(), "rect") == 0) {
            float x = RoundFloatingInput(std::stof(node->attribute("x").value()) / viewport->uupix);
            float y = RoundFloatingInput(std::stof(node->attribute("y").value()) / viewport->uupiy);
            float w = RoundFloatingInput(std::stof(node->attribute("width").value()) / viewport->uupix);
            float h = RoundFloatingInput(std::stof(node->attribute("height").value()) / viewport->uupiy);

            doc->shapes.emplace_back(x, y, w, h);
            continue;
        }
    }
}

// The svg input files usually has values that are slightly different than what was intended
// when creating them due to floating point errors. We currently round the input shapes to the
// nearest thousandth to correct this.
float RoundFloatingInput(float x) {
   return roundf(x * 1000) / 1000;
}

char* FindChar(char *str, char ch) {
    while (*str && *str != ch )
        str++;

    return str;
}

char* SkipChar(char *str, char ch) {
    str = FindChar(str, ch);
    str++;

    return str;
}