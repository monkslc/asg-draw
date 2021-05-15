#ifndef SVG_H
#define SVG_H

#include "pugixml.hpp"

#include "ds.hpp"
#include "sviggy.hpp"
#include "utils.hpp"

class ViewPort {
    public:
    float uupix, uupiy;
    ViewPort(pugi::xml_node *node) {
       // Assumes the width and height are specified in inches
       float width_inches  = std::stof(node->attribute("width").value());
       float height_inches = std::stof(node->attribute("height").value());

       // Viewbox comes in in the format "x y width height"
       char *view_box = (char *)node->attribute("viewBox").value();

       view_box = SkipChar(view_box, ' ');
       view_box = SkipChar(view_box, ' ');
       float width_user_units = std::stof(view_box);

       view_box = SkipChar(view_box, ' ');
       float height_user_units = std::stof(view_box);

       this->uupix = width_user_units / width_inches;
       this->uupiy = height_user_units / height_inches;
    }
};

// Parsing SVG Tag Methods
Path ParseTagCircle(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Path ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Path ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Path ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Path ParseTagRect(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);
Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport, DXState *dx);

// Generic Parsing Methods
bool IsAlphabetical(char c);
bool IsDigit(char c);
bool IsFloatingPointChar(char c);
bool IsWhitespace(char c);
float ParseFloat(char **path, float uupi);
Vec2 ParsePoint(char **iter, ViewPort *viewport, Vec2 *pos, bool relative);

void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc, DXState *dx);
void LoadSVGFile(char *file, Document *doc, DXState *dx);

// Parsing Path Command Methods
void ParsePathCmdMove(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdLine(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdCubic(PathBuilder *builder, ViewPort *viewport, char **path, Vec2 *pos, bool relative);
void ParsePathCmdClose(PathBuilder *builder, char **path, Vec2 *pos, Vec2 sub_path_start);

#endif