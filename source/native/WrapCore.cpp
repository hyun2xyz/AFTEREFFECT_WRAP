#include "WrapCore.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <stdexcept>

namespace elw {
namespace {
float unit(float x) { return std::clamp(x, 0.0f, 1.0f); }
void poll(const Cancel& cancel, int line) { if (cancel && (line & 31) == 0) cancel(); }
using Plane = std::vector<float>;

// Sliding-window extrema: O(image size), including large user-selected widths.
Plane extremum(const Plane& in, int w, int h, int radius, bool horizontal, bool minimum,
               const Cancel& cancel) {
    if (!radius) return in;
    Plane out(in.size());
    int length = horizontal ? w : h, lines = horizontal ? h : w;
    for (int line = 0; line < lines; ++line) {
        poll(cancel, line);
        auto index = [&](int pos) { return horizontal ? std::size_t(line) * w + pos : std::size_t(pos) * w + line; };
        auto value = [&](int pos) { return in[index(std::clamp(pos, 0, length - 1))]; };
        std::deque<int> queue;
        for (int pos = -radius; pos < length + radius; ++pos) {
            while (!queue.empty() && queue.front() < pos - 2 * radius) queue.pop_front();
            float v = value(pos);
            while (!queue.empty() && (minimum ? value(queue.back()) >= v : value(queue.back()) <= v)) queue.pop_back();
            queue.push_back(pos);
            int center = pos - radius;
            if (center >= 0) out[index(center)] = value(queue.front());
        }
    }
    return out;
}
Plane morphAxis(const Plane& in, int w, int h, float radius, bool horizontal, bool minimum,
                const Cancel& cancel) {
    radius = std::clamp(radius, 0.0f, 200.0f);
    int lo = int(std::floor(radius));
    float f = radius - lo;
    Plane a = extremum(in, w, h, lo, horizontal, minimum, cancel);
    if (f > 0.0001f) {
        Plane b = extremum(in, w, h, lo + 1, horizontal, minimum, cancel);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] += f * (b[i] - a[i]);
    }
    return a;
}
void boxAxis(Plane& in, int w, int h, int radius, bool horizontal, const Cancel& cancel) {
    if (radius <= 0) return;
    Plane out(in.size());
    int length = horizontal ? w : h, lines = horizontal ? h : w;
    double divisor = 2 * radius + 1;
    for (int line = 0; line < lines; ++line) {
        poll(cancel, line);
        auto index = [&](int pos) { return horizontal ? std::size_t(line) * w + pos : std::size_t(pos) * w + line; };
        auto value = [&](int pos) { return in[index(std::clamp(pos, 0, length - 1))]; };
        double sum = 0;
        for (int p = -radius; p <= radius; ++p) sum += value(p);
        for (int p = 0; p < length; ++p) {
            out[index(p)] = float(sum / divisor);
            sum += double(value(p + radius + 1)) - value(p - radius);
        }
    }
    in.swap(out);
}
void blur(Plane& p, int w, int h, float amountX, float amountY, const Cancel& cancel) {
    auto radii = [](float amount) {
        double sigma = std::clamp(double(amount), 0.0, 1000.0) / 3.0;
        int low = int(std::floor(std::sqrt(4 * sigma * sigma + 1)));
        if (!(low & 1)) --low;
        low = std::max(1, low);
        double m = (12 * sigma * sigma - 3 * low * low - 12 * low - 9) / (-4 * low - 4);
        int nlow = std::clamp(int(std::round(m)), 0, 3);
        std::array<int, 3> out{};
        for (int i = 0; i < 3; ++i) out[i] = ((i < nlow ? low : low + 2) - 1) / 2;
        return out;
    };
    auto rx = radii(amountX), ry = radii(amountY);
    for (int i = 0; i < 3; ++i) {
        boxAxis(p, w, h, rx[i], true, cancel);
        boxAxis(p, w, h, ry[i], false, cancel);
    }
}
} // namespace

Image::Image(int w, int h) : width(w), height(h) {
    if (w < 0 || h < 0 || (std::uint64_t(w) * std::uint64_t(h)) > 268435456ULL)
        throw std::length_error("Image is too large.");
    pixels.resize(std::size_t(w) * h);
}

Image render(const Image& subject, const Image* background, const Settings& s, const Cancel& cancel) {
    int w = subject.width, h = subject.height;
    if (!w || !h) return subject;
    if (s.view == View::Composite && (s.strength <= 0 || !background || background->pixels.empty())) return subject;
    if (background && (background->width != w || background->height != h))
        throw std::invalid_argument("Background must be sampled into subject coordinates.");
    Plane alpha(subject.pixels.size());
    for (std::size_t i = 0; i < alpha.size(); ++i) alpha[i] = unit(subject.pixels[i].a);
    Plane inner = morphAxis(alpha, w, h, s.edgeWidth * s.scaleX, true, true, cancel);
    inner = morphAxis(inner, w, h, s.edgeWidth * s.scaleY, false, true, cancel);
    blur(inner, w, h, s.edgeFeather * s.scaleX, s.edgeFeather * s.scaleY, cancel);
    Plane outer = morphAxis(alpha, w, h, s.wrapSpread * s.scaleX, true, false, cancel);
    outer = morphAxis(outer, w, h, s.wrapSpread * s.scaleY, false, false, cancel);
    blur(outer, w, h, s.wrapSoftness * s.scaleX, s.wrapSoftness * s.scaleY, cancel);
    Plane matte(alpha.size());
    for (std::size_t i = 0; i < matte.size(); ++i)
        matte[i] = unit(outer[i] * (1 - inner[i])) * (s.preserveAlpha ? alpha[i] : 1.0f);
    alpha.clear(); alpha.shrink_to_fit();
    inner.clear(); inner.shrink_to_fit();
    outer.clear(); outer.shrink_to_fit();
    Image out(w, h);
    if (s.view == View::EdgeMatte) {
        for (std::size_t i = 0; i < matte.size(); ++i) out.pixels[i] = {1, matte[i], matte[i], matte[i]};
        return out;
    }
    Image light = background ? *background : Image(w, h);
    for (auto& p : light.pixels) if (p.a <= 0) p = {};
    // Blur premultiplied channels, including alpha, to prevent transparent-color fringes.
    for (int channel = 0; channel < 4; ++channel) {
        Plane p(light.pixels.size());
        for (std::size_t i = 0; i < p.size(); ++i) {
            const Pixel& px = light.pixels[i];
            p[i] = channel == 0 ? px.a : channel == 1 ? px.r : channel == 2 ? px.g : px.b;
        }
        blur(p, w, h, s.backgroundBlur * s.scaleX, s.backgroundBlur * s.scaleY, cancel);
        for (std::size_t i = 0; i < p.size(); ++i) {
            Pixel& px = light.pixels[i];
            (channel == 0 ? px.a : channel == 1 ? px.r : channel == 2 ? px.g : px.b) = p[i];
        }
    }
    float strength = std::clamp(s.strength, 0.0f, 100.0f) / 100.0f;
    for (int y = 0; y < h; ++y) {
        poll(cancel, y);
        for (int x = 0; x < w; ++x) {
            std::size_t i = std::size_t(y) * w + x;
            const Pixel& f = subject.pixels[i];
            const Pixel& b = light.pixels[i];
            float amount = matte[i] * strength;
            Pixel l{unit(b.a) * amount, b.r * amount, b.g * amount, b.b * amount};
            if (s.view == View::WrapOnly) { out.pixels[i] = l; continue; }
            // Screen in premultiplied space. Positive HDR values are never darkened.
            float ceiling = s.preserveAlpha ? unit(f.a) : 1.0f;
            auto screen = [ceiling](float base, float glow) {
                return base + glow * std::max(0.0f, ceiling - std::clamp(base, 0.0f, ceiling));
            };
            out.pixels[i] = {s.preserveAlpha ? f.a : f.a + l.a * (1 - unit(f.a)),
                             screen(f.r, l.r), screen(f.g, l.g), screen(f.b, l.b)};
        }
    }
    return out;
}
} // namespace elw
