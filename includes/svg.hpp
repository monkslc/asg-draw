#ifndef SVG_H
#define SVG_H

#include "pugixml.hpp"

#include "ds.hpp"
#include "sviggy.hpp"

class ViewPort {
    public:
    float uupix, uupiy;
    ViewPort(pugi::xml_node *node);
};

// Parsing SVG Tag Methods
ShapeData ParseTagCircle(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
ShapeData ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
ShapeData ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
ShapeData ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
ShapeData ParseTagRect(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);

// Generic Parsing Methods
bool IsAlphabetical(char c);
bool IsDigit(char c);
bool IsFloatingPointChar(char c);
bool IsWhitespace(char c);
float ParseFloat(char **path, float uupi);
Vec2 ParsePoint(char **iter, ViewPort *viewport, Vec2 *pos, bool relative);
char* FindChar(char *str, char ch);
char* SkipChar(char *str, char ch);
float RoundFloatingInput(float x);

void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc, DXState *dx);
void LoadSVGFile(char *file, Document *doc, DXState *dx);

// Parsing Path Command Methods
void ParsePathCmdMove(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdLine(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdCubic(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdClose(PathBuilder *builder, char **path, Vec2 *pos, Vec2 sub_path_start);

#endif