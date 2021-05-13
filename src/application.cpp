#include <d2d1.h>
#include <unordered_set>

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

void Document::AddNewPath(Path p) {
    p.collection = this->NextFreeCollection();
    this->paths.push_back(p);
}

size_t Document::NextFreeCollection() {
    auto collections = std::unordered_set<size_t>();
    for (auto &path : this->paths) {
        collections.insert(path.collection);
    }

    auto collection=0;
    while (true) {
        if (collections.find(collection) == collections.end()) return collection;

        collection++;
    }

    return collection;
}

bool EntirelyContains(D2D1_RECT_F* outer, D2D1_RECT_F* inner) {
    return
        outer->left   <= inner->left  &&
        outer->top    <= inner->top   &&
        outer->right  >= inner->right &&
        outer->bottom >= inner->bottom;
}

void Document::SelectShapes(Vec2 mousedown, Vec2 mouseup) {
    auto new_selection = std::vector<ActiveShape>();

    float minx = std::min<float>(mousedown.x, mouseup.x);
    float miny = std::min<float>(mousedown.y, mouseup.y);
    float maxx = std::max<float>(mousedown.x, mouseup.x);
    float maxy = std::max<float>(mousedown.y, mouseup.y);

    Vec2 start = Vec2(minx, miny);
    Vec2 end   = Vec2(maxx, maxy);

    D2D1_RECT_F selection = D2D1::RectF(start.x, start.y, end.x, end.y);
    for (auto i=0; i<this->paths.size(); i++) {
       Path *path = &this->paths[i];
       D2D1_RECT_F bound = path->Bound();

       if (EntirelyContains(&selection, &bound)) {
           new_selection.emplace_back(ShapeType::Path, i);
       }
    }

    this->active_shapes = new_selection;
}

void Document::SelectShape(Vec2 screen_pos) {
    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->paths.size(); i++) {
        Path *path = &this->paths[i];

        BOOL contains_point;
        path->geometry->StrokeContainsPoint(point, kHairline, NULL, path->TransformMatrix() * doc_to_screen, &contains_point);

        if (contains_point) {
            this->active_shapes = std::vector<ActiveShape>{ActiveShape(ShapeType::Path, i)};
            return;
        }
    }

    this->active_shapes = std::vector<ActiveShape>();
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