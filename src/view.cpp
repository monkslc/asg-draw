#include "sviggy.hpp"

View::View() : start(Vec2(0.0f, 0.0f)), mouse_pos_screen(Vec2(0.0f, 0.0f)) {};
Vec2 View::GetDocumentPosition(Vec2 screen_pos) {
    return this->ScreenToDocumentMat().TransformPoint(screen_pos.D2Point());
}

Vec2 View::MousePos() {
    return this->GetDocumentPosition(this->mouse_pos_screen);
}

void View::Scale(bool in) {
    // We preserve the mouse document position when scaling in and out
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