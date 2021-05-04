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

void AddNodesToDocument(ViewPort *viewport, pugi::xml_object_range<pugi::xml_node_iterator> nodes, Document *doc);
void LoadSVGFile(char *file, Document *doc);
bool IsFloatingPointChar(char c);

#endif