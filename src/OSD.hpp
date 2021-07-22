#ifndef OSD_hpp
#define OSD_hpp

#include <map>

#include <imp/imp_osd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

struct OSDTextItem {
    IMPRgnHandle imp_rgn;
    IMPOSDRgnAttr imp_attr;
    IMPOSDGrpRgnAttr imp_grp_attr;
    std::string text;
    uint8_t *data;
    int point_size;
    int stroke;
    uint32_t color;
    uint32_t stroke_color;
};

class OSD {
public:
    bool init();
    void update();
private:
    int freetype_init();
    void draw_glyph(OSDTextItem *ti, FT_BitmapGlyph bmg,
                     int *pen_x, int *pen_y,
                    int item_height, int item_width,
                   uint32_t color);
    void set_text(OSDTextItem *ti, std::string text);

    FT_Library freetype;
    FT_Face fontface;
    FT_Stroker stroker;

    OSDTextItem timestamp;
    time_t last_ts_time;

    std::map<char,FT_BitmapGlyph> bitmaps;
    std::map<char,FT_BitmapGlyph> stroke_bitmaps;
    std::map<char,FT_BBox> boxes;
    std::map<char,FT_Vector> advances;
};

#endif
