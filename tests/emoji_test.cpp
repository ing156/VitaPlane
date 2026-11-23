#include "emoji.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cassert>
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    assert(argc==2);
    FT_Library library=nullptr;
    FT_Face face=nullptr;
    assert(!FT_Init_FreeType(&library));
    assert(!FT_New_Face(library,argv[1],0,&face));
    assert(!FT_Select_Size(face,0));
    size_t count=0;
    for (const auto& entry : emoji_data::entries) {
        const std::string text(entry.text);
        assert(emoji::next(text,0)==text.size());
        assert(emoji::previous(text,text.size())==0);
        assert(emoji::find(text)==&entry);
        assert(entry.base<std::size(emoji_data::entries));
        assert(entry.group<9);
        assert(!FT_Load_Glyph(face,entry.glyph,FT_LOAD_COLOR|FT_LOAD_RENDER));
        assert(face->glyph->bitmap.pixel_mode==FT_PIXEL_MODE_BGRA);
        assert(face->glyph->bitmap.width && face->glyph->bitmap.rows);
        if (entry.picker) {
            const auto& base=emoji_data::entries[entry.base];
            assert(base.picker && base.base==entry.base && base.group==entry.group);
            ++count;
        }
    }
    for (const char* text : {u8"👍🏽",u8"👩🏽‍💻",u8"👨‍👩‍👧‍👦",u8"🇷🇺",u8"1️⃣",u8"🏴󠁧󠁢󠁥󠁮󠁧󠁿"}) {
        assert(emoji::find(text));
        const std::string joined=std::string("a")+text+"b";
        const size_t end=joined.size()-1;
        assert(emoji::next(joined,1)==end);
        assert(emoji::previous(joined,end)==1);
    }
    assert(emoji::find(u8"☝🏻")->base==emoji::find(u8"☝️")->base);
    assert(emoji::find(u8"🖐🏿")->base==emoji::find(u8"🖐️")->base);
    assert(emoji::find(u8"🏽") && !emoji::find(u8"🏽")->picker);
    assert(!emoji::find(u8"❤︎")); // VS15 explicitly requests text presentation.
    assert(emoji::next(u8"áb",0)==3);
    assert(emoji::next(u8"🇷🇺🇯🇵",0)==8);
    assert(emoji::next("\r\na",0)==2);
    assert(emoji::next(std::string("\0",1),0)==1);
    assert(emoji::next("\xf0\x9f",0)==1);
    assert(emoji::next(u8"x‍y",0)==4); // ZWJ alone must not combine arbitrary letters.
    assert(count>3000);
    FT_Done_Face(face); FT_Done_FreeType(library);
    std::printf("Emoji: %zu picker entries, aliases, glyph rasterization and cluster boundaries passed\n",count);
}
