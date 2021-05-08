#ifndef SVG_H
#define SVG_H

#include "pugixml.hpp"

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

Circle ParseTagCircle(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);
Path ParseTagLine(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);
Path ParseTagPath(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);
Path ParseTagPolygon(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);
Rect ParseTagRect(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);
Text ParseTagText(pugi::xml_node_iterator node, ViewPort *viewport, D2State *d2);

bool IsAlphabetical(char c);
bool IsDigit(char c);
bool IsFloatingPointChar(char c);
bool IsWhitespace(char c);
float ParseFloat(char **path, float uupi);
void EatWhitespace(char **iter);

void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc, D2State *d2);
void LoadSVGFile(char *file, Document *doc, D2State *d2);
void ParseCommandLetter(float command, std::vector<float> *commands, ViewPort *viewport, char **iter, uint8_t n, Vec2 *pos, bool relative);
void ParseCommandPoints(std::vector<float> *commands, ViewPort *viewport, char **iter, uint8_t n, Vec2 *last_pos, bool relative);
Vec2 ParsePoint(char **iter, ViewPort *viewport);

#endif