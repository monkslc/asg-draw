#include <d2d1.h>

#include "sviggy.hpp"

Document::Document() {};

void Document::Click(Vec2 screen_pos) {
    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->paths.size(); i++) {
        Path *path = &this->paths[i];

        BOOL contains_point;
        path->geometry->StrokeContainsPoint(point, kHairline, NULL, path->TransformMatrix() * doc_to_screen, &contains_point);

        if (contains_point) {
            printf("found you :)\n");
            this->active_shape = ActiveShape(ShapeType::Path, i);
            return;
        }
    }

    this->active_shape = ActiveShape();
}

void Document::TranslateView(Vec2 amount) {
    this->view.start += amount;
}

void Document::ScrollZoom(bool in) {
    this->view.ScrollZoom(in);
}

Vec2 Document::MousePos() {
   return  this->view.MousePos();
}