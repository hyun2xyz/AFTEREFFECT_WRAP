#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace elw {
struct Pixel { float a = 0, r = 0, g = 0, b = 0; }; // Premultiplied RGBA.
struct Image {
    int width = 0, height = 0;
    std::vector<Pixel> pixels;
    Image() = default;
    Image(int w, int h);
    Pixel& at(int x, int y) { return pixels[std::size_t(y) * width + x]; }
    const Pixel& at(int x, int y) const { return pixels[std::size_t(y) * width + x]; }
};
enum class View { Composite = 1, WrapOnly = 2, EdgeMatte = 3 };
struct Settings {
    float edgeWidth = 3, edgeFeather = 1.5f, wrapSpread = 4, wrapSoftness = 6;
    float backgroundBlur = 25, strength = 40;
    float scaleX = 1, scaleY = 1;
    bool preserveAlpha = false;
    View view = View::Composite;
};
using Cancel = std::function<void()>; // May throw; the AE entry point catches it.
Image render(const Image& subject, const Image* background, const Settings&, const Cancel& cancel = {});
} // namespace elw
