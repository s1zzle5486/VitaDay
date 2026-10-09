int main(){assert(loadCjk());std::ifstream f("work/cjk-codepoints.txt");unsigned cp;std::set<unsigned> points;while(f>>cp)points.insert(cp);
 for(unsigned value:points)for(int size:{12,18,28}){assert(cjkFace.has(value));CjkGlyph g;assert(cjkFace.glyph(size,value,g));assert(g.advance>0);assert(g.width>0&&g.height>0);assert(std::any_of(g.alpha.begin(),g.alpha.end(),[](unsigned char a){return a>0;}));assert(cjkWidth(size,"日本語")>0);}
 for(unsigned value:points){auto* entry=cjkEntry(18,value);assert(entry&&entry->page>=0);auto* atlas=cjkAtlases[entry->page].texture;bool ink=false;for(int y=0;y<entry->h;++y)for(int x=0;x<entry->w;++x)ink|=atlas->pixels[((entry->y+y)*512+entry->x+x)*4+3]>0;assert(ink);}
 assert(cjkEntry(18,0x65e5)!=cjkEntry(28,0x65e5));assert(drawCjk(0,30,18,"日本語",0xffffffff)==cjkWidth(18,"日本語"));
 for(unsigned value=0x4e00;value<0x9fff&&!cjkReset;++value)cjkEntry(28,value);assert(cjkReset&&cjkAtlases.size()==8);clearCjkAtlas();assert(cjkGlyphs.empty()&&cjkAtlases.empty()&&!cjkReset);assert(cjkEntry(18,0x65e5));
 std::cout<<points.size()<<" CJK characters from all 20 locales decoded and resampled at 3 sizes; native atlas uploads, mixed sizes, advance, capacity, reset passed\n";clearCjkAtlas();
 drawCjk(24,46,28,"日本語 · 天気とカレンダー",0xff3e2a1e);
 drawCjk(24,98,28,"한국어 · 날씨와 달력",0xff3e2a1e);
 drawCjk(24,150,28,"简体中文 · 天气与日历",0xff3e2a1e);
 drawCjk(24,202,28,"繁體中文 · 天氣與日曆",0xff3e2a1e);
 std::ofstream image("work/cjk-glyph-render.rgba",std::ios::binary);image.write((char*)canvas.data(),canvas.size());
 clearCjkAtlas();cjkFace.close();}
