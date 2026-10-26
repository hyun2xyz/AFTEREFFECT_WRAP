#include "WrapCore.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace elw;
void require(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
bool identical(const Image& a, const Image& b) {
    return a.width == b.width && a.height == b.height &&
           std::memcmp(a.pixels.data(), b.pixels.data(), a.pixels.size() * sizeof(Pixel)) == 0;
}
Image subject(int w = 128, int h = 128) {
    Image out(w,h);
    for (int y=0;y<h;++y) for(int x=0;x<w;++x) {
        float a=std::clamp(30.0f-std::hypot(float(x-w/2),float(y-h/2)),0.0f,1.0f);
        out.at(x,y)={a,a*0.18f,a*0.2f,a*0.24f};
    }
    return out;
}
Image background(int w = 128, int h = 128) {
    Image out(w,h);
    for(int y=0;y<h;++y) for(int x=0;x<w;++x)
        out.at(x,y)={1, x<w/2 ? 0.1f : 0.95f, 0.25f, x<w/2 ? 0.95f : 0.1f};
    return out;
}
void ppm(const std::filesystem::path& path, const Image& image, const Image* bg = nullptr) {
    std::ofstream out(path,std::ios::binary);
    out << "P6\n" << image.width << ' ' << image.height << "\n255\n";
    for(std::size_t i=0;i<image.pixels.size();++i) {
        Pixel p=image.pixels[i];
        if(bg) { p.r+=bg->pixels[i].r*(1-p.a); p.g+=bg->pixels[i].g*(1-p.a); p.b+=bg->pixels[i].b*(1-p.a); }
        unsigned char c[3]={static_cast<unsigned char>(std::round(std::clamp(p.r,0.0f,1.0f)*255)),
                            static_cast<unsigned char>(std::round(std::clamp(p.g,0.0f,1.0f)*255)),
                            static_cast<unsigned char>(std::round(std::clamp(p.b,0.0f,1.0f)*255))};
        out.write(reinterpret_cast<char*>(c),3);
    }
}
int main(int argc,char** argv) {
    try {
        auto fg=subject(), bg=background(); Settings s;
        s.strength=0;
        require(identical(fg,render(fg,&bg,s)),"Strength=0 must be an exact bypass.");
        s.strength=70;
        require(identical(fg,render(fg,nullptr,s)),"Background=None must bypass.");
        Image solid(128,128); for(auto& p:solid.pixels) p={1,0.2f,0.3f,0.4f};
        require(identical(solid,render(solid,&bg,s)),"Opaque source has no alpha edge.");
        Image empty(128,128);
        require(identical(empty,render(empty,&bg,s)),"Transparent source must not generate a full-frame light.");
        auto result=render(fg,&bg,s);
        require(result.at(64,64).r==fg.at(64,64).r,"Subject center must retain detail.");
        require(result.at(92,64).r>fg.at(92,64).r,"Red background must light the right edge.");
        require(result.at(36,64).b>fg.at(36,64).b,"Blue background must light the left edge.");
        require(result.at(97,64).a>0,"Outer spread must produce a local halo.");
        require(result.at(0,0).a==0,"Halo must not reach distant empty pixels.");
        s.preserveAlpha=true;
        auto preserved=render(fg,&bg,s);
        for(std::size_t i=0;i<fg.pixels.size();++i) require(fg.pixels[i].a==preserved.pixels[i].a,"Preserve Alpha changed alpha.");
        s.preserveAlpha=false;
        s.view=View::WrapOnly;
        auto only=render(fg,&bg,s);
        require(only.at(64,64).a==0 && only.at(92,64).a>0,"Wrap Only must isolate the edge.");
        s.view=View::EdgeMatte;
        auto matte=render(fg,&bg,s);
        require(matte.at(64,64).r==0 && matte.at(92,64).r>0,"Edge matte must isolate the edge.");
        require(matte.at(92,64).r==matte.at(92,64).g && matte.at(92,64).a==1,"Matte preview must be grayscale and opaque.");
        s.view=View::Composite;
        Image transparentBg(128,128);
        require(identical(fg,render(fg,&transparentBg,s)),"Transparent background must not introduce halos.");
        for(auto& p:transparentBg.pixels) p={0,1,0.5f,2};
        require(identical(fg,render(fg,&transparentBg,s)),"RGB hidden under zero background alpha must not leak.");
        auto hdr=fg; hdr.at(92,64)={1,3.0f,2.0f,1.2f};
        auto hdrOut=render(hdr,&bg,s);
        require(hdrOut.at(92,64).r>=3.0f && std::isfinite(hdrOut.at(92,64).r),"HDR must not be clipped or darkened.");
        s.edgeWidth=2.5f; s.wrapSpread=1.5f; s.backgroundBlur=0; s.edgeFeather=0; s.wrapSoftness=0;
        auto sharp=render(fg,&bg,s);
        require(sharp.at(64,64).r==fg.at(64,64).r,"Zero blur must preserve the interior.");
        for(int w:{1,2,3}) for(int h:{1,2,3}) {
            Image a(w,h),b(w,h); a.at(0,0)={1,0.2f,0.3f,0.4f}; b.at(0,0)={1,1,0,0};
            s.edgeWidth=100; s.wrapSpread=100; s.backgroundBlur=500;
            auto tiny=render(a,&b,s);
            require(tiny.pixels.size()==a.pixels.size(),"Large radii on tiny images must stay in bounds.");
            for(auto p:tiny.pixels) require(std::isfinite(p.r)&&p.a>=0&&p.a<=1,"Tiny image result must be finite.");
        }
        bool cancelled=false;
        try { render(fg,&bg,s,[]{throw std::runtime_error("abort");}); } catch(const std::runtime_error&) {cancelled=true;}
        require(cancelled,"Rendering must honor cancellation.");
        if(argc==2) {
            std::filesystem::create_directories(argv[1]);
            ppm(std::filesystem::path(argv[1])/"before.ppm",fg,&bg);
            ppm(std::filesystem::path(argv[1])/"after.ppm",result,&bg);
            ppm(std::filesystem::path(argv[1])/"matte.ppm",matte);
        }
        std::cout<<"PASS: native core bypass, matte, colored edges, halo, alpha, HDR, small frames and cancellation\n";
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
