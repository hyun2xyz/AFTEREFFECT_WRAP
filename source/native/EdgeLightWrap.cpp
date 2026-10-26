#include "EdgeLightWrap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

namespace {
struct PreData {
    elw::Settings settings;
    PF_LRect bounds{};
    int backgroundWidth = 0, backgroundHeight = 0;
    int sourceWidth = 0, sourceHeight = 0;
};
void deletePreData(void* data) { delete static_cast<PreData*>(data); }
void checked(PF_Err err) { if (err) throw err; }
float scale(const PF_RationalScale& r) { return r.den ? float(r.num) / r.den : 1.0f; }
void cancel(PF_InData* in_data) { checked(PF_ABORT(in_data)); }

class WorldSuite {
    SPBasicSuite* basic;
public:
    const PF_WorldSuite2* suite = nullptr;
    explicit WorldSuite(PF_InData* in) : basic(in->pica_basicP) {
        if (!basic) throw static_cast<PF_Err>(PF_Err_INVALID_CALLBACK);
        checked(basic->AcquireSuite(kPFWorldSuite, kPFWorldSuiteVersion2, reinterpret_cast<const void**>(&suite)));
    }
    ~WorldSuite() { basic->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2); }
    PF_PixelFormat format(const PF_EffectWorld* w) const {
        PF_PixelFormat f{};
        checked(suite->PF_GetPixelFormat(w, &f));
        if (f != PF_PixelFormat_ARGB32 && f != PF_PixelFormat_ARGB64 && f != PF_PixelFormat_ARGB128)
            throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
        return f;
    }
};

elw::Pixel readPixel(const PF_EffectWorld* w, PF_PixelFormat f, int x, int y) {
    if (!w || !w->data || x < 0 || y < 0 || x >= w->width || y >= w->height) return {};
    const char* row = reinterpret_cast<const char*>(w->data) + std::ptrdiff_t(y) * w->rowbytes;
    if (f == PF_PixelFormat_ARGB128) {
        const auto& p = reinterpret_cast<const PF_PixelFloat*>(row)[x];
        return {p.alpha, p.red, p.green, p.blue};
    }
    if (f == PF_PixelFormat_ARGB64) {
        const auto& p = reinterpret_cast<const PF_Pixel16*>(row)[x];
        return {p.alpha / 32768.0f, p.red / 32768.0f, p.green / 32768.0f, p.blue / 32768.0f};
    }
    const auto& p = reinterpret_cast<const PF_Pixel8*>(row)[x];
    return {p.alpha / 255.0f, p.red / 255.0f, p.green / 255.0f, p.blue / 255.0f};
}
void writePixel(PF_EffectWorld* w, PF_PixelFormat f, int x, int y, const elw::Pixel& p) {
    char* row = reinterpret_cast<char*>(w->data) + std::ptrdiff_t(y) * w->rowbytes;
    if (f == PF_PixelFormat_ARGB128) {
        reinterpret_cast<PF_PixelFloat*>(row)[x] = {p.a, p.r, p.g, p.b};
    } else if (f == PF_PixelFormat_ARGB64) {
        auto q = [](float v) { return static_cast<A_u_short>(std::round(std::clamp(v, 0.0f, 1.0f) * 32768)); };
        reinterpret_cast<PF_Pixel16*>(row)[x] = {q(p.a), q(p.r), q(p.g), q(p.b)};
    } else {
        auto q = [](float v) { return static_cast<A_u_char>(std::round(std::clamp(v, 0.0f, 1.0f) * 255)); };
        reinterpret_cast<PF_Pixel8*>(row)[x] = {q(p.a), q(p.r), q(p.g), q(p.b)};
    }
}

// Layer parameters use source/layer coordinates. Different dimensions are fit to
// the subject's source rectangle; comp-sized precomps give an exact 1:1 mapping.
elw::Pixel sampleBackground(const PF_EffectWorld* w, PF_PixelFormat f, double x, double y) {
    if (!w || !w->data || !w->width || !w->height) return {};
    x = std::clamp(x - w->origin_x, 0.0, double(w->width - 1));
    y = std::clamp(y - w->origin_y, 0.0, double(w->height - 1));
    int x0 = int(x), y0 = int(y), x1 = std::min(x0 + 1, int(w->width - 1)), y1 = std::min(y0 + 1, int(w->height - 1));
    float fx = float(x - x0), fy = float(y - y0);
    auto a = readPixel(w, f, x0, y0), b = readPixel(w, f, x1, y0);
    auto c = readPixel(w, f, x0, y1), d = readPixel(w, f, x1, y1);
    auto mix = [=](float av, float bv, float cv, float dv) { return (av * (1-fx) + bv * fx) * (1-fy) + (cv * (1-fx) + dv * fx) * fy; };
    return {mix(a.a,b.a,c.a,d.a), mix(a.r,b.r,c.r,d.r), mix(a.g,b.g,c.g,d.g), mix(a.b,b.b,c.b,d.b)};
}

PF_Err paramsSetup(PF_InData* in_data, PF_OutData* out_data) {
    PF_ParamDef def{};
    PF_ADD_LAYER("Background Layer", PF_LayerDefault_NONE, Background);
    PF_ADD_FLOAT_SLIDERX("Edge Width", 0, 100, 0, 25, 3, PF_Precision_TENTHS, 0, 0, EdgeWidth);
    PF_ADD_FLOAT_SLIDERX("Edge Feather", 0, 100, 0, 25, 1.5, PF_Precision_TENTHS, 0, 0, EdgeFeather);
    PF_ADD_FLOAT_SLIDERX("Wrap Spread", 0, 100, 0, 25, 4, PF_Precision_TENTHS, 0, 0, WrapSpread);
    PF_ADD_FLOAT_SLIDERX("Wrap Softness", 0, 100, 0, 25, 6, PF_Precision_TENTHS, 0, 0, WrapSoftness);
    PF_ADD_FLOAT_SLIDERX("Background Blur", 0, 500, 0, 100, 25, PF_Precision_TENTHS, 0, 0, BackgroundBlur);
    PF_ADD_FLOAT_SLIDERX("Strength", 0, 100, 0, 100, 40, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, Strength);
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("Preserve Subject Alpha", FALSE, 0, PreserveAlpha);
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("View", 3, 1, "Composite|Wrap Only|Edge Matte", OutputView);
    out_data->num_params = NumParams;
    return PF_Err_NONE;
}

elw::Settings readSettings(PF_InData* in_data, PF_ParamDef* params[]) {
    elw::Settings s;
    s.edgeWidth = float(params[EdgeWidth]->u.fs_d.value);
    s.edgeFeather = float(params[EdgeFeather]->u.fs_d.value);
    s.wrapSpread = float(params[WrapSpread]->u.fs_d.value);
    s.wrapSoftness = float(params[WrapSoftness]->u.fs_d.value);
    s.backgroundBlur = float(params[BackgroundBlur]->u.fs_d.value);
    s.strength = float(params[Strength]->u.fs_d.value);
    s.preserveAlpha = params[PreserveAlpha]->u.bd.value != 0;
    s.view = static_cast<elw::View>(params[OutputView]->u.pd.value);
    s.scaleX = scale(in_data->downsample_x);
    s.scaleY = scale(in_data->downsample_y);
    return s;
}

PF_Err preRender(PF_InData* in_data, PF_PreRenderExtra* extra) {
    auto data = std::make_unique<PreData>();
    PF_ParamDef values[NumParams]{};
    PF_ParamDef* pointers[NumParams]{};
    for (int p = EdgeWidth; p < NumParams; ++p) {
        // The host automatically checks these in at the end of SMART_PRE_RENDER.
        checked(PF_CHECKOUT_PARAM(in_data, p, in_data->current_time, in_data->time_step, in_data->time_scale, &values[p]));
        pointers[p] = &values[p];
    }
    data->settings = readSettings(in_data, pointers);
    data->sourceWidth = std::max(1, int(std::ceil(in_data->width * data->settings.scaleX)));
    data->sourceHeight = std::max(1, int(std::ceil(in_data->height * data->settings.scaleY)));
    PF_RenderRequest req = extra->input->output_request;
    // The full source is needed for erosion, blur and stable results across ROI tiles.
    req.rect = {-100000, -100000, 100000, 100000};
    req.channel_mask = PF_ChannelMask_ARGB;
    req.preserve_rgb_of_zero_alpha = TRUE;
    PF_CheckoutResult subject{}, bg{};
    checked(extra->cb->checkout_layer(in_data->effect_ref, Input, Input, &req,
            in_data->current_time, in_data->time_step, in_data->time_scale, &subject));
    checked(extra->cb->checkout_layer(in_data->effect_ref, Background, Background, &req,
            in_data->current_time, in_data->time_step, in_data->time_scale, &bg));
    data->backgroundWidth = int(std::ceil(bg.ref_width * data->settings.scaleX));
    data->backgroundHeight = int(std::ceil(bg.ref_height * data->settings.scaleY));
    data->bounds = subject.max_result_rect;
    data->bounds.left = std::min<A_long>(0, data->bounds.left);
    data->bounds.top = std::min<A_long>(0, data->bounds.top);
    data->bounds.right = std::max<A_long>(data->sourceWidth, data->bounds.right);
    data->bounds.bottom = std::max<A_long>(data->sourceHeight, data->bounds.bottom);
    extra->output->result_rect = extra->output->max_result_rect = data->bounds;
    extra->output->solid = FALSE;
    extra->output->flags |= PF_RenderOutputFlag_RETURNS_EXTRA_PIXELS;
    extra->output->delete_pre_render_data_func = deletePreData;
    extra->output->pre_render_data = data.release();
    return PF_Err_NONE;
}

PF_Err process(PF_InData* in_data, const PreData& data, const PF_EffectWorld* subject,
               const PF_EffectWorld* background, PF_EffectWorld* output, bool smart) {
    if (!output || !output->data) return PF_Err_BAD_CALLBACK_PARAM;
    WorldSuite suite(in_data);
    auto outFormat = suite.format(output);
    auto sourceFormat = subject && subject->data ? suite.format(subject) : outFormat;
    auto bgFormat = background && background->data ? suite.format(background) : outFormat;
    int w = data.bounds.right - data.bounds.left, h = data.bounds.bottom - data.bounds.top;
    elw::Image source(w, h);
    bool hasBackground = background && background->data && background->width > 0 && background->height > 0;
    elw::Image bg = hasBackground ? elw::Image(w, h) : elw::Image();
    int sx = smart && subject ? subject->origin_x : 0;
    int sy = smart && subject ? subject->origin_y : 0;
    for (int y = 0; y < h; ++y) {
        if (!(y & 31)) cancel(in_data);
        for (int x = 0; x < w; ++x) {
            int gx = x + data.bounds.left, gy = y + data.bounds.top;
            source.at(x,y) = readPixel(subject, sourceFormat, gx-sx, gy-sy);
            if (hasBackground) {
                double bx = (gx + 0.5) * std::max(1, data.backgroundWidth) / double(data.sourceWidth) - 0.5;
                double by = (gy + 0.5) * std::max(1, data.backgroundHeight) / double(data.sourceHeight) - 0.5;
                bg.at(x,y) = sampleBackground(background, bgFormat, bx, by);
            }
        }
    }
    auto result = elw::render(source, hasBackground ? &bg : nullptr, data.settings, [&] { cancel(in_data); });
    int ox = smart ? output->origin_x : 0, oy = smart ? output->origin_y : 0;
    for (int y = 0; y < output->height; ++y) {
        if (!(y & 31)) cancel(in_data);
        for (int x = 0; x < output->width; ++x) {
            int rx = x + ox - data.bounds.left, ry = y + oy - data.bounds.top;
            elw::Pixel p{};
            if (rx >= 0 && ry >= 0 && rx < w && ry < h) p = result.at(rx,ry);
            writePixel(output, outFormat, x, y, p);
        }
    }
    return PF_Err_NONE;
}

PF_Err smartRender(PF_InData* in_data, PF_SmartRenderExtra* extra) {
    auto* data = static_cast<const PreData*>(extra->input->pre_render_data);
    if (!data) return PF_Err_BAD_CALLBACK_PARAM;
    PF_EffectWorld *source = nullptr, *bg = nullptr, *output = nullptr;
    checked(extra->cb->checkout_layer_pixels(in_data->effect_ref, Input, &source));
    checked(extra->cb->checkout_layer_pixels(in_data->effect_ref, Background, &bg));
    checked(extra->cb->checkout_output(in_data->effect_ref, &output));
    return process(in_data, *data, source, bg, output, true);
}
PF_Err legacyRender(PF_InData* in_data, PF_ParamDef* params[], PF_LayerDef* output) {
    PreData data;
    data.settings = readSettings(in_data, params);
    data.bounds = {0, 0, output->width, output->height};
    data.sourceWidth = output->width; data.sourceHeight = output->height;
    PF_ParamDef bg{};
    checked(PF_CHECKOUT_PARAM(in_data, Background, in_data->current_time, in_data->time_step, in_data->time_scale, &bg));
    data.backgroundWidth = bg.u.ld.width; data.backgroundHeight = bg.u.ld.height;
    PF_Err err = PF_Err_NONE;
    try { err = process(in_data, data, &params[Input]->u.ld, &bg.u.ld, output, false); }
    catch (...) { PF_CHECKIN_PARAM(in_data, &bg); throw; }
    auto checkin = PF_CHECKIN_PARAM(in_data, &bg);
    return err ? err : checkin;
}
} // namespace

extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr ptr, PF_PluginDataCB2 cb,
        SPBasicSuite*, const char*, const char*) {
    PF_Err result = PF_Err_NONE;
    PF_REGISTER_EFFECT_EXT2(ptr, cb, ELW_NAME, ELW_MATCH_NAME, ELW_CATEGORY, AE_RESERVED_INFO,
            "EffectMain", "https://github.com/hyun2xyz/edge-light-wrap");
    return result;
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data,
        PF_ParamDef* params[], PF_LayerDef* output, void* extra) {
    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                    "Edge Light Wrap 2.0.0\rNative edge light wrap. Apply to an isolated subject, then select Background Layer.");
                return PF_Err_NONE;
            case PF_Cmd_GLOBAL_SETUP:
                out_data->my_version = PF_VERSION(2,0,0,PF_Stage_RELEASE,1);
                out_data->out_flags = ELW_FLAGS; out_data->out_flags2 = ELW_FLAGS2;
                return PF_Err_NONE;
            case PF_Cmd_PARAMS_SETUP: return paramsSetup(in_data, out_data);
            case PF_Cmd_SMART_PRE_RENDER: return preRender(in_data, static_cast<PF_PreRenderExtra*>(extra));
            case PF_Cmd_SMART_RENDER: return smartRender(in_data, static_cast<PF_SmartRenderExtra*>(extra));
            case PF_Cmd_RENDER: return legacyRender(in_data, params, output);
            default: return PF_Err_NONE;
        }
    } catch (PF_Err err) { return err; }
    catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (const std::exception& e) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Edge Light Wrap: %s", e.what());
        out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    } catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}
