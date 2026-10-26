#include "EdgeLightWrap.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
void* openPlugin(const char* path) { return reinterpret_cast<void*>(LoadLibraryA(path)); }
void* findSymbol(void* module,const char* name) { return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module),name)); }
void closePlugin(void* module) { FreeLibrary(static_cast<HMODULE>(module)); }
std::string loadError() { return "LoadLibrary failed with error "+std::to_string(GetLastError()); }
#else
#include <dlfcn.h>
void* openPlugin(const char* path) { return dlopen(path,RTLD_NOW|RTLD_LOCAL); }
void* findSymbol(void* module,const char* name) { return dlsym(module,name); }
void closePlugin(void* module) { dlclose(module); }
std::string loadError() { return dlerror(); }
#endif

using EffectEntry = decltype(&EffectMain);
EffectEntry invoke = nullptr;

void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
struct World {
    PF_EffectWorld w{};
    std::vector<unsigned char> bytes;
    World(int width,int height,PF_PixelFormat format,int ox=0,int oy=0) {
        int pixelSize=format==PF_PixelFormat_ARGB32?4:format==PF_PixelFormat_ARGB64?8:16;
        w.width=width; w.height=height; w.rowbytes=width*pixelSize+32;
        w.origin_x=ox; w.origin_y=oy; w.reserved_long1=format;
        bytes.resize(std::size_t(w.rowbytes)*height,0xA5);
        w.data=reinterpret_cast<PF_PixelPtr>(bytes.data());
        for(int y=0;y<height;++y) for(int x=0;x<width;++x) set(x,y,{});
    }
    void set(int x,int y,elw::Pixel p) {
        auto* row=bytes.data()+std::size_t(y)*w.rowbytes;
        if(w.reserved_long1==PF_PixelFormat_ARGB32) {
            auto q=[](float v){return static_cast<A_u_char>(std::round(v*255));};
            reinterpret_cast<PF_Pixel8*>(row)[x]={q(p.a),q(p.r),q(p.g),q(p.b)};
        } else if(w.reserved_long1==PF_PixelFormat_ARGB64) {
            auto q=[](float v){return static_cast<A_u_short>(std::round(v*32768));};
            reinterpret_cast<PF_Pixel16*>(row)[x]={q(p.a),q(p.r),q(p.g),q(p.b)};
        } else reinterpret_cast<PF_PixelFloat*>(row)[x]={p.a,p.r,p.g,p.b};
    }
    elw::Pixel get(int x,int y) const {
        const auto* row=bytes.data()+std::size_t(y)*w.rowbytes;
        if(w.reserved_long1==PF_PixelFormat_ARGB32) {auto p=reinterpret_cast<const PF_Pixel8*>(row)[x];return {p.alpha/255.f,p.red/255.f,p.green/255.f,p.blue/255.f};}
        if(w.reserved_long1==PF_PixelFormat_ARGB64) {auto p=reinterpret_cast<const PF_Pixel16*>(row)[x];return {p.alpha/32768.f,p.red/32768.f,p.green/32768.f,p.blue/32768.f};}
        auto p=reinterpret_cast<const PF_PixelFloat*>(row)[x];return {p.alpha,p.red,p.green,p.blue};
    }
    bool paddingIntact() const {
        int stride=w.reserved_long1==PF_PixelFormat_ARGB32?4:w.reserved_long1==PF_PixelFormat_ARGB64?8:16;
        for(int y=0;y<w.height;++y) for(int x=w.width*stride;x<w.rowbytes;++x)
            if(bytes[std::size_t(y)*w.rowbytes+x]!=0xA5) return false;
        return true;
    }
};
struct Host {
    std::array<PF_ParamDef,NumParams> params{};
    World* source=nullptr; World* background=nullptr; World* output=nullptr;
    bool abort=false;
    int checkins=0;
};
PF_Err addParam(PF_ProgPtr ref,PF_ParamIndex,PF_ParamDefPtr def) {
    auto& h=*reinterpret_cast<Host*>(ref); h.params[def->uu.id]=*def; return 0;
}
PF_Err checkout(PF_ProgPtr ref,PF_ParamIndex p,A_long,A_long,A_u_long,PF_ParamDef* out) {
    auto& h=*reinterpret_cast<Host*>(ref); *out=h.params[p];
    if(p==Background) out->u.ld=h.background?h.background->w:PF_LayerDef{};
    return 0;
}
PF_Err checkin(PF_ProgPtr ref,PF_ParamDef*) { ++reinterpret_cast<Host*>(ref)->checkins; return 0; }
PF_Err abortRender(PF_ProgPtr ref) { return reinterpret_cast<Host*>(ref)->abort?PF_Interrupt_CANCEL:0; }
PF_Err getFormat(const PF_EffectWorld* w,PF_PixelFormat* f) { *f=PF_PixelFormat(w->reserved_long1); return 0; }
PF_WorldSuite2 worldSuite{nullptr,nullptr,getFormat};
SPErr acquire(const char* name,int32 version,const void** suite) {
    if(std::strcmp(name,kPFWorldSuite)||version!=2) return 1;
    *suite=&worldSuite;return 0;
}
SPErr release(const char*,int32) {return 0;}
PF_Err checkoutLayer(PF_ProgPtr ref,PF_ParamIndex p,A_long,const PF_RenderRequest* req,A_long,A_long,A_u_long,PF_CheckoutResult* out) {
    auto& h=*reinterpret_cast<Host*>(ref);
    World* w=p==Input?h.source:h.background;
    require(req->rect.left<=0&&req->rect.right>=64,"Full-frame SmartFX dependency is required.");
    *out={};
    if(w) {
        out->result_rect=out->max_result_rect={w->w.origin_x,w->w.origin_y,w->w.origin_x+w->w.width,w->w.origin_y+w->w.height};
        out->ref_width=w->w.width;out->ref_height=w->w.height;
    }
    return 0;
}
PF_Err checkoutPixels(PF_ProgPtr ref,A_long p,PF_EffectWorld** out) {
    auto& h=*reinterpret_cast<Host*>(ref); World* w=p==Input?h.source:h.background; *out=w?&w->w:nullptr;return 0;
}
PF_Err checkinPixels(PF_ProgPtr,A_long) {return 0;}
PF_Err checkoutOutput(PF_ProgPtr ref,PF_EffectWorld** out) { *out=&reinterpret_cast<Host*>(ref)->output->w;return 0; }
PF_Err renderSmart(PF_InData& in,PF_OutData& out,Host& h) {
    PF_PreRenderInput pi{}; pi.output_request.rect={0,0,h.output->w.width,h.output->w.height};
    PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{checkoutLayer,nullptr};PF_PreRenderExtra pe{&pi,&po,&pc};
    PF_Err err=invoke(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe);
    if(err)return err;
    PF_SmartRenderInput si{};si.pre_render_data=po.pre_render_data;si.output_request=pi.output_request;
    PF_SmartRenderCallbacks sc{checkoutPixels,checkinPixels,checkoutOutput};PF_SmartRenderExtra se{&si,&sc};
    err=invoke(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se);
    po.delete_pre_render_data_func(po.pre_render_data);
    return err;
}
int main(int argc,char** argv) {
    try {
        require(argc==2,"Pass the actual compiled plug-in executable.");
        void* bundle=openPlugin(argv[1]);
        if(!bundle) throw std::runtime_error(loadError());
        invoke=reinterpret_cast<EffectEntry>(findSymbol(bundle,"EffectMain"));
        require(invoke!=nullptr,"The plug-in does not export EffectMain.");
        SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
        for(auto format:{PF_PixelFormat_ARGB32,PF_PixelFormat_ARGB64,PF_PixelFormat_ARGB128}) {
            Host h;PF_InData in{};PF_OutData out{};
            in.effect_ref=reinterpret_cast<PF_ProgPtr>(&h);in.pica_basicP=&basic;
            in.inter.add_param=addParam;in.inter.checkout_param=checkout;in.inter.checkin_param=checkin;in.inter.abort=abortRender;
            in.downsample_x={1,1};in.downsample_y={1,1};in.width=64;in.height=64;in.time_scale=24;in.time_step=1;
            require(!invoke(PF_Cmd_GLOBAL_SETUP,&in,&out,nullptr,nullptr,nullptr),"Global setup failed.");
            require(out.out_flags==ELW_FLAGS&&out.out_flags2==ELW_FLAGS2,"PiPL flags must match runtime flags.");
            require(!invoke(PF_Cmd_PARAMS_SETUP,&in,&out,nullptr,nullptr,nullptr),"Parameter setup failed.");
            require(out.num_params==NumParams,"Wrong parameter count.");
            require(h.params[Background].param_type==PF_Param_LAYER&&h.params[OutputView].u.pd.num_choices==3,"Missing effect controls.");
            World source(64,64,format),bg(64,64,format),output(64,64,format);
            h.source=&source;h.background=&bg;h.output=&output;
            for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
                float a=std::clamp(16.0f-std::hypot(float(x-32),float(y-32)),0.0f,1.0f);
                source.set(x,y,{a,a*0.2f,a*0.3f,a*0.4f}); bg.set(x,y,{1,0.9f,0.2f,0.1f});
            }
            h.params[Strength].u.fs_d.value=0;
            require(!renderSmart(in,out,h),"SmartFX bypass failed.");
            for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
                auto a=source.get(x,y),b=output.get(x,y);
                require(!std::memcmp(&a,&b,sizeof(a)),"8/16/32-bit bypass changed pixels.");
            }
            h.params[Strength].u.fs_d.value=70;
            require(!renderSmart(in,out,h),"SmartFX render failed.");
            require(output.get(46,32).r>source.get(46,32).r,"Native ABI render did not light edges.");
            require(output.get(32,32).r==source.get(32,32).r,"Native ABI render changed the center.");
            World roi(13,11,format,39,27);h.output=&roi;
            require(!renderSmart(in,out,h),"SmartFX ROI render failed.");
            for(int y=0;y<11;++y) for(int x=0;x<13;++x) {
                auto a=roi.get(x,y),b=output.get(x+39,y+27);
                require(!std::memcmp(&a,&b,sizeof(a)),"ROI output differs from the corresponding full-frame pixels.");
            }
            require(source.paddingIntact()&&bg.paddingIntact()&&output.paddingIntact()&&roi.paddingIntact(),"Row padding was overwritten.");
            h.output=&output;h.background=nullptr;
            require(!renderSmart(in,out,h),"Background=None caused an error.");
            require(output.get(46,32).r==source.get(46,32).r,"Background=None must bypass.");
            h.background=&bg;h.abort=true;
            require(renderSmart(in,out,h)==PF_Interrupt_CANCEL,"Host cancellation was not propagated.");
            h.abort=false;
            std::array<PF_ParamDef*,NumParams> ptrs{};
            h.params[Input].u.ld=source.w;
            for(int i=0;i<NumParams;++i)ptrs[i]=&h.params[i];
            require(!invoke(PF_Cmd_RENDER,&in,&out,ptrs.data(),&output.w,nullptr),"Legacy render failed.");
            require(h.checkins==1,"Legacy background checkout was not checked in.");
            std::cout<<"PASS: AE host callbacks, params, "<<(format==PF_PixelFormat_ARGB32?8:format==PF_PixelFormat_ARGB64?16:32)<<" bpc, padded rows, ROI, missing background, cancellation, legacy render\n";
        }
        closePlugin(bundle);
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n';return 1; }
}
