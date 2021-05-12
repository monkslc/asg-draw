#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

#define TAGCMP(node, type) strcmp(node->name(), type) == 0

void LoadSVGFile(char *file, Document *doc, DXState *dx) {
    pugi::xml_document xml;
    pugi::xml_parse_result result = xml.load_file(file);

    pugi::xml_node svg_tree = xml.child("svg");
    ViewPort viewport = ViewPort(&svg_tree);

    pugi::xml_object_range<pugi::xml_node_iterator> nodes = svg_tree.children();
    AddNodesToDocument(&viewport, nodes, doc, dx);
}

// TODO: just let this accept a node and go through its children
void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc, DXState *dx) {
    for (pugi::xml_node_iterator node : nodes) {

        if (TAGCMP(node, "g")) {
            AddNodesToDocument(viewport, node->children(), doc, dx);
            continue;
        }

        if (TAGCMP(node, "rect")) {
            doc->AddNewPath(ParseTagRect(node, viewport, dx));
            continue;
        }

        if (TAGCMP(node, "text")) {
            doc->texts.push_back(ParseTagText(node, viewport, dx));
            continue;
        }

        if (TAGCMP(node, "line")) {
            doc->AddNewPath(ParseTagLine(node, viewport, dx));
            continue;
        }

        if (TAGCMP(node, "polygon")) {
            doc->AddNewPath(ParseTagPolygon(node, viewport, dx));
            continue;
        }

        if (TAGCMP(node, "circle")) {
            doc->AddNewPath(ParseTagCircle(node, viewport, dx));
            continue;
        }

        if (TAGCMP(node, "path")) {
            doc->AddNewPath(ParseTagPath(node, viewport, dx));
            continue;
        }
    }
}

Path ParseTagRect(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x = RoundFloatingInput(node->attribute("x"     ).as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("y"     ).as_float() / viewport->uupiy);
    float w = RoundFloatingInput(node->attribute("width" ).as_float() / viewport->uupix);
    float h = RoundFloatingInput(node->attribute("height").as_float() / viewport->uupiy);

    float left = x;
    float right = x + w;
    float top = y;
    float bottom = y + h;

    auto commands = std::vector<float>{
        kPathCommandMove, left,  top,
        kPathCommandLine, right, top,
        kPathCommandLine, right, bottom,
        kPathCommandLine, left,  bottom,
        kPathCommandLine, left,  top,
    };

    return Path(commands, dx);
}

Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x = RoundFloatingInput(node->attribute("x").as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("y").as_float() / viewport->uupiy);

    return Text(Vec2(x, y), std::string(node->text().get()), dx);
}

Path ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x1 = RoundFloatingInput(node->attribute("x1").as_float() / viewport->uupix);
    float y1 = RoundFloatingInput(node->attribute("y1").as_float() / viewport->uupiy);
    float x2 = RoundFloatingInput(node->attribute("x2").as_float() / viewport->uupix);
    float y2 = RoundFloatingInput(node->attribute("y2").as_float() / viewport->uupiy);

    auto commands = std::vector<float>{
        kPathCommandMove, x1, y1,
        kPathCommandLine, x2, y2,
    };

    return Path(commands, dx);
}

Path ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    auto commands = std::vector<float>();
    commands.push_back(kPathCommandMove);

    char *iter = (char *)node->attribute("points").value();
    while (*iter) {
       char *end;

       float x = ParseFloat(&iter, viewport->uupix);
       while (*iter && !IsFloatingPointChar(*iter)) iter++;
       commands.push_back(x);

       float y = ParseFloat(&iter, viewport->uupiy);
       while (*iter && !IsFloatingPointChar(*iter)) iter++;
       commands.push_back(y);

       commands.push_back(kPathCommandLine);
    }

    if (commands.size() > 3) {
        // Close the path
        float start_x = commands[1];
        commands.push_back(start_x);

        float start_y = commands[2];
        commands.push_back(start_y);

        return Path(commands, dx);
    } else {
        // Not a valid polygon, just create an empty one
        return Path(std::vector<float>(), dx);
    }
}

Path ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    char *path = (char *) node->attribute("d").value();
    auto commands = std::vector<float>();

    Vec2 pos = Vec2(0.0f, 0.0f);
    Vec2 sub_path_start = pos;

    while (*path) {
        while(!IsAlphabetical(*path)) path++;

        switch (*path) {
            case 'M': {
                ParseCommandLetter(kPathCommandMove, &commands, viewport, &path, 1, &pos, false);
                sub_path_start = pos;
                break;
            }

            case 'm': {
                ParseCommandLetter(kPathCommandMove, &commands, viewport, &path, 1, &pos, true);
                sub_path_start = pos;
                break;
            }

            case 'c': {
                ParseCommandLetter(kPathCommandCubic, &commands, viewport, &path, 3, &pos, true);
                break;
            }

            case 'l': {
                ParseCommandLetter(kPathCommandLine, &commands, viewport, &path, 1, &pos, true);
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

    return Path(commands, dx);
}

Path ParseTagCircle(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x = RoundFloatingInput(node->attribute("cx").as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("cy").as_float() / viewport->uupiy);
    float r = RoundFloatingInput(node->attribute("r" ).as_float() / viewport->uupix);

    return Path::CreateCircle(Vec2(x, y), r, dx);
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

bool IsWhitespace(char c) {
    return c == ' '  ||
           c == '\t' ||
           c == '\r' ||
           c == '\n';
}

float ParseFloat(char **path, float uupi) {
    return RoundFloatingInput(std::strtof(*path, path)) / uupi;
}

// One command letter can precede one or more commands. We iterate over the points
// that follow a command letter and add them to the commands vector until we hit the
// next command or the end of the path. For example, a line command may look like:
// "L 100 100 200 200" which would be two line commands. One to (100, 100) and a following
// one to (200, 200)
void ParseCommandLetter(float command, std::vector<float> *commands, ViewPort *viewport, char **path, uint8_t n, Vec2 *pos, bool relative) {
    (*path)++;

    while (**path && !IsAlphabetical(**path)) {
        commands->push_back(command);
        EatWhitespace(path);
        ParseCommandPoints(commands, viewport, path, n, pos, relative);
        EatWhitespace(path);
    }
}

void ParseCommandPoints(std::vector<float> *commands, ViewPort *viewport, char **iter, uint8_t n, Vec2 *pos, bool relative) {
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
    while (**iter && IsWhitespace(**iter)) (*iter)++;
}
