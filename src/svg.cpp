#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

#include "ds.hpp"

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
            Text text = ParseTagText(node, viewport, dx);
            doc->texts.Push(text);
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

    return Path::CreateRect(Vec2(x, y), Vec2(w, h), dx);
}

Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x = RoundFloatingInput(node->attribute("x").as_float() / viewport->uupix);
    float y = RoundFloatingInput(node->attribute("y").as_float() / viewport->uupiy);

    return Text(Vec2(x, y), String((char *)node->text().get()), dx);
}

Path ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    float x1 = RoundFloatingInput(node->attribute("x1").as_float() / viewport->uupix);
    float y1 = RoundFloatingInput(node->attribute("y1").as_float() / viewport->uupiy);
    float x2 = RoundFloatingInput(node->attribute("x2").as_float() / viewport->uupix);
    float y2 = RoundFloatingInput(node->attribute("y2").as_float() / viewport->uupiy);

    return Path::CreateLine(Vec2(x1, y1), Vec2(x2, y2), dx);
}

Path ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    PathBuilder builder = PathBuilder(dx);
    char *iter = (char *)node->attribute("points").value();

    float x = ParseFloat(&iter, viewport->uupix); iter++;
    float y = ParseFloat(&iter, viewport->uupiy); iter++;
    Vec2 start = Vec2(x, y);
    builder.Move(start);

    while (*iter) {
       float x = ParseFloat(&iter, viewport->uupix); iter++;
       float y = ParseFloat(&iter, viewport->uupiy); iter++;
       builder.Line(Vec2(x, y));
    }

    builder.Close();
    return builder.Build();
}

Path ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx) {
    PathBuilder builder = PathBuilder(dx);

    char *path = (char *) node->attribute("d").value();

    Vec2 pos = Vec2(0.0f, 0.0f);
    // A 'z' command closes a sub path and an 'm' command opens a new one
    Vec2 sub_path_start = pos;

    while (*path) {
        while(!IsAlphabetical(*path)) path++;

        switch (*path) {
            case 'M': {
                ParsePathCmdMove(&builder, viewport, &path, &pos, false);
                sub_path_start = pos;
                break;
            }

            case 'm': {
                ParsePathCmdMove(&builder, viewport, &path, &pos, true);
                sub_path_start = pos;
                break;
            }

            case 'c': {
                ParsePathCmdCubic(&builder, viewport, &path, &pos, true);
                break;
            }

            case 'l': {
                ParsePathCmdLine(&builder, viewport, &path, &pos, true);
                break;
            }

            case 'z': {
                ParsePathCmdClose(&builder, &path, &pos, sub_path_start);
                break;
            }

            default:
                printf("Unrecognized svg command: %c\n", *path);
                path++;
                break;
        }

    }

    return builder.Build();
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

Vec2 ParsePoint(char **iter, ViewPort *viewport, Vec2 *pos, bool relative) {
    float x = ParseFloat(iter, viewport->uupix);
    (*iter)++;
    float y = ParseFloat(iter, viewport->uupiy);

    Vec2 point = Vec2(x, y);
    if (relative) {
        point += *pos;
    }

    return point;
}

/* ParsePathCmd */
// All of the path command parse methods assume that "path" variable points to the letter that starts the command.
// For example the line command may come in as 'l 25,25'. Commands can also be repeated without respecifying the
// starting indicating letter like so: 'l 10,10 20,20 30,30' which explains the while loop in each parse command
// that goes until it finds the next indicating letter. The current pos should be updated at the end of each command.

void ParsePathCmdMove(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative) {
    (*path)++;

    while (**path && !IsAlphabetical(**path)) {
        Vec2 to = ParsePoint(path, viewport, pos, relative);
        builder->Move(to);

        *pos = to;
    }
}

void ParsePathCmdLine(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative) {
    (*path)++;

    while (**path && !IsAlphabetical(**path)) {
        Vec2 to = ParsePoint(path, viewport, pos, relative);
        builder->Line(to);

        *pos = to;
    }
}

void ParsePathCmdCubic(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative) {
    (*path)++;

    while (**path && !IsAlphabetical(**path)) {
        Vec2 c1  = ParsePoint(path, viewport, pos, relative);
        Vec2 c2  = ParsePoint(path, viewport, pos, relative);
        Vec2 end = ParsePoint(path, viewport, pos, relative);
        builder->Cubic(c1, c2, end);

        *pos = end;
    }
}

void ParsePathCmdClose(PathBuilder *builder, char **path, Vec2 *pos, Vec2 sub_path_start) {
    (*path)++;
    builder->Close();

    *pos = sub_path_start;
}