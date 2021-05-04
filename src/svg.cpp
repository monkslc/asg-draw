#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

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

        if (strcmp(node->name(), "text") == 0) {
            float x = RoundFloatingInput(std::stof(node->attribute("x").value()) / viewport->uupix);
            float y = RoundFloatingInput(std::stof(node->attribute("y").value()) / viewport->uupiy);
            doc->texts.emplace_back(x, y, std::string(node->text().get()));
        }

    }
}