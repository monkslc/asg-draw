#include <d2d1.h>

#include "sviggy.hpp"

Application::Application() : documents(std::vector<Document> {Document()}), active_doc(0) {};

Document* Application::ActiveDoc() {
    return &this->documents[this->active_doc];
}

View* Application::ActiveView() {
    return &this->documents[this->active_doc].view;
}

void Application::ActivateDoc(size_t index) {
    if (this->documents.size() <= index) {
        auto i = documents.size();
        while (i <= index) {
            this->documents.push_back(Document());
            i++;
        }
    }

    this->active_doc = index;
}

Document::Document() {};

void Document::Click(Vec2 screen_pos) {
    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->paths.size(); i++) {
        Path *path = &this->paths[i];

        BOOL contains_point;
        path->geometry->StrokeContainsPoint(point, kHairline, NULL, path->TransformMatrix() * doc_to_screen, &contains_point);

        if (contains_point) {
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

View::View() : start(Vec2(0.0f, 0.0f)), mouse_pos_screen(Vec2(0.0f, 0.0f)) {};
Vec2 View::GetDocumentPosition(Vec2 screen_pos) {
    return this->ScreenToDocumentMat().TransformPoint(screen_pos.D2Point());
}

Vec2 View::MousePos() {
    return this->GetDocumentPosition(this->mouse_pos_screen);
}

// Scroll zoom adjusts the scale while preservice the mouse position in the document
void View::ScrollZoom(bool in) {
    Vec2 original_mouse_pos = this->MousePos();

    float delta = in ? 1 + kScaleDelta : 1-kScaleDelta;
    this->scale *= delta;

    Vec2 mouse_after_scale = this->MousePos();
    Vec2 mouse_change = original_mouse_pos - mouse_after_scale;

    this->start += mouse_change;
}

D2D1::Matrix3x2F View::ScaleMatrix() {
    return D2D1::Matrix3x2F::Scale(
        this->scale * kPixelsPerInch,
        this->scale * kPixelsPerInch,
        D2D1_POINT_2F()
    );
}

D2D1::Matrix3x2F View::TranslationMatrix() {
    return D2D1::Matrix3x2F::Translation(-this->start.x, -this->start.y);
}

D2D1::Matrix3x2F View::DocumentToScreenMat() {
    D2D1::Matrix3x2F scale_matrix = this->ScaleMatrix();
    D2D1::Matrix3x2F translation_matrix = this->TranslationMatrix();

    return translation_matrix * scale_matrix;
}

D2D1::Matrix3x2F View::ScreenToDocumentMat() {
    D2D1::Matrix3x2F mat = this->DocumentToScreenMat();
    mat.Invert();
    return mat;
}