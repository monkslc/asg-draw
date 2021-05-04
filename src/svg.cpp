#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

#define TAGCMP(node, type) strcmp(node->name(), type) == 0
#define IS_DIGIT(c) (c >= '0' && c <= '9')

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

        if (TAGCMP(node, "g")) {
            AddNodesToDocument(viewport, node->children(), doc);
            continue;
        }

        if (TAGCMP(node, "rect")) {
            float x = RoundFloatingInput(node->attribute("x"     ).as_float() / viewport->uupix);
            float y = RoundFloatingInput(node->attribute("y"     ).as_float() / viewport->uupiy);
            float w = RoundFloatingInput(node->attribute("width" ).as_float() / viewport->uupix);
            float h = RoundFloatingInput(node->attribute("height").as_float() / viewport->uupiy);

            printf("Rect** x: %.9f y: %.9f w: %.9f h %.9f\n", x, y, w, h);

            doc->shapes.emplace_back(x, y, w, h);
            continue;
        }

        if (TAGCMP(node, "text")) {
            float x = RoundFloatingInput(node->attribute("x").as_float() / viewport->uupix);
            float y = RoundFloatingInput(node->attribute("y").as_float() / viewport->uupiy);

            printf("Text** x: %.9f y: %.9f\n", x, y);

            doc->texts.emplace_back(x, y, std::string(node->text().get()));
            continue;
        }

        if (TAGCMP(node, "line")) {
            float x1 = RoundFloatingInput(node->attribute("x1").as_float() / viewport->uupix);
            float y1 = RoundFloatingInput(node->attribute("y1").as_float() / viewport->uupiy);
            float x2 = RoundFloatingInput(node->attribute("x2").as_float() / viewport->uupix);
            float y2 = RoundFloatingInput(node->attribute("y2").as_float() / viewport->uupiy);
            doc->lines.emplace_back(x1, y1, x2, y2);
            continue;
        }

        if (TAGCMP(node, "polygon")) {
            auto points = std::vector<Vec2>();
            const char *points_str = node->attribute("points").value();
            char *iter = (char *)points_str;
            while (*iter != NULL) {
               char *end;

               float x = RoundFloatingInput(std::strtof(iter, &end));
               while (*end && !IsFloatingPointChar(*end)) end++;
               iter = end;

               float y = RoundFloatingInput(std::strtof(iter, &end));
               while (*end && !IsFloatingPointChar(*end)) end++;
               iter = end;

                points.emplace_back(x / viewport->uupix, y / viewport->uupiy);
            }

            doc->polygons.emplace_back(points);
        }
    }
}

bool IsFloatingPointChar(char c) {
    return IS_DIGIT(c) ||
           c == '+'    ||
           c == '-'    ||
           c == '.';
}