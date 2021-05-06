#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

#define TAGCMP(node, type) strcmp(node->name(), type) == 0

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
            doc->shapes.push_back(ParseTagRect(node, viewport));
            continue;
        }

        if (TAGCMP(node, "text")) {
            doc->texts.push_back(ParseTagText(node, viewport));
            continue;
        }

        if (TAGCMP(node, "line")) {
            doc->lines.push_back(ParseTagLine(node, viewport));
            continue;
        }

        if (TAGCMP(node, "polygon")) {
            doc->polygons.push_back(ParseTagPolygon(node, viewport));
            continue;
        }

        if (TAGCMP(node, "circle")) {
            doc->circles.push_back(ParseTagCircle(node, viewport));
            continue;
        }

        if (TAGCMP(node, "path")) {
            doc->paths.push_back(ParseTagPath(node, viewport));
            continue;
        }
    }
}

Rect ParseTagRect(pugi::xml_node_iterator node, ViewPort *viewport) {
    float x = RoundFloatingInput(node->attribute("x"     ).as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("y"     ).as_float() / viewport->uupiy);
    float w = RoundFloatingInput(node->attribute("width" ).as_float() / viewport->uupix);
    float h = RoundFloatingInput(node->attribute("height").as_float() / viewport->uupiy);

    return Rect(x, y, w, h);
}

Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport) {
    float x = RoundFloatingInput(node->attribute("x").as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("y").as_float() / viewport->uupiy);

    return Text(x, y, std::string(node->text().get()));
}

Line ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport) {
    float x1 = RoundFloatingInput(node->attribute("x1").as_float() / viewport->uupix);
    float y1 = RoundFloatingInput(node->attribute("y1").as_float() / viewport->uupiy);
    float x2 = RoundFloatingInput(node->attribute("x2").as_float() / viewport->uupix);
    float y2 = RoundFloatingInput(node->attribute("y2").as_float() / viewport->uupiy);

    return Line(x1, y1, x2, y2);
}

Poly ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport) {
    auto points = std::vector<Vec2>();
    char *iter = (char *)node->attribute("points").value();
    while (*iter) {
       char *end;

       float x = ParseFloat(&iter, viewport->uupix);
       while (*iter && !IsFloatingPointChar(*iter)) iter++;

       float y = ParseFloat(&iter, viewport->uupiy);
       while (*iter && !IsFloatingPointChar(*iter)) iter++;

        points.emplace_back(x, y);
    }

    return Poly(points);
}

Path ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport) {
    char *path = (char *) node->attribute("d").value();
    auto commands = std::vector<float>();

    Vec2 pos = Vec2(0.0f, 0.0f);
    Vec2 sub_path_start = pos;

    while (*path) {
        while(!IsAlphabetical(*path)) path++;

        switch (*path) {
            case 'M': {
                ParseCommand(kPathCommandMove, &commands, viewport, &path, 1, &pos, false);
                sub_path_start = pos;
                break;
            }

            case 'm': {
                ParseCommand(kPathCommandMove, &commands, viewport, &path, 1, &pos, true);
                sub_path_start = pos;
                break;
            }

            case 'c': {
                ParseCommand(kPathCommandCubic, &commands, viewport, &path, 3, &pos, true);
                break;
            }

            case 'l': {
                ParseCommand(kPathCommandLine, &commands, viewport, &path, 1, &pos, true);
                break;
            }

            case 'z': {
                commands.push_back(kPathCommandLine);
                commands.push_back(sub_path_start.x);
                commands.push_back(sub_path_start.y);
                path++;
                break;
            }

            default:
                printf("Unrecognized svg command: %c\n", *path);
                path++;
                break;
        }

    }

    return Path(commands);
}

Circle ParseTagCircle(pugi::xml_node_iterator node, ViewPort *viewport) {
    float x = RoundFloatingInput(node->attribute("cx").as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("cy").as_float() / viewport->uupiy);
    float r = RoundFloatingInput(node->attribute("r" ).as_float() / viewport->uupix);

    return Circle(x, y, r);
}

bool IsFloatingPointChar(char c) {
    return IsDigit(c) ||
           c == '+'    ||
           c == '-'    ||
           c == '.';
}

bool IsAlphabetical(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsDigit(char c) {
    return c >= '0' && c <= '9';
}

float ParseFloat(char **path, float uupi) {
    return RoundFloatingInput(std::strtof(*path, path)) / uupi;
}

void ParsePoints(std::vector<float> *commands, ViewPort *viewport, char **iter, uint8_t n, Vec2 *pos, bool relative) {
    Vec2 new_pos = *pos;
    for (auto i=0; i < n; i++) {
       Vec2 new_point = ParsePoint(iter, viewport);

       if (relative) {
           new_point += *pos;
       }

       commands->push_back(new_point.x);
       commands->push_back(new_point.y);

       new_pos = new_point;
    }

    *pos = new_pos;
}

Vec2 ParsePoint(char **iter, ViewPort *viewport) {
    float x = ParseFloat(iter, viewport->uupix);
    (*iter)++;
    float y = ParseFloat(iter, viewport->uupiy);

    return Vec2(x, y);
}

void EatWhitespace(char **iter) {
    while (**iter && **iter == ' ') (*iter)++;
}

void ParseCommand(float command, std::vector<float> *commands, ViewPort *viewport, char **path, uint8_t n, Vec2 *pos, bool relative) {
    (*path)++;

    while (**path && !IsAlphabetical(**path)) {
        commands->push_back(command);
        EatWhitespace(path);
        ParsePoints(commands, viewport, path, n, pos, relative);
        EatWhitespace(path);
    }
}