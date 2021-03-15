#include <iostream>
#include <vector>
#include "OSD.hpp"

int OSD::freetype_init() {
    int error;

    error = FT_Init_FreeType(&freetype);
    if (error) {
        return error;
    }

    error = FT_New_Face(
        freetype,
        "/usr/share/fonts/NotoSansMono-Regular.ttf",
        0,
        &fontface
    );
    if (error) {
        return error;
    }

    FT_Stroker_New(freetype, &stroker);

    FT_Set_Char_Size(
        fontface,
        0,
        72*64,
        96,
        96
    );
    return 0;
}

void OSD::draw_glyph(OSDTextItem *ti, FT_BitmapGlyph bmg,
                     int *pen_x, int *pen_y,
                     int item_height, int item_width,
                     uint32_t color) {
    int rel_pen_x = *pen_x + bmg->left;
    int rel_pen_y = *pen_y + bmg->top;
    unsigned int row = 0;
    while (row < bmg->bitmap.rows) {
        rel_pen_x = *pen_x + bmg->left;
        for (unsigned int x = 0; x < bmg->bitmap.width; ++x, ++rel_pen_x) {
            if (rel_pen_x < 0 || rel_pen_y < 0 || rel_pen_x > item_width - 1 || rel_pen_y > item_height - 1)
                continue;
            int data_index = (item_height-rel_pen_y)*item_width*4 + rel_pen_x*4;
            if (data_index+3 >= item_height*item_width*4)
                continue;
            int glyph_index = row*bmg->bitmap.pitch + x;

            uint8_t red, green, blue, alpha;
            uint8_t alpha_a = bmg->bitmap.buffer[glyph_index];
            uint8_t alpha_b = ti->data[data_index+3];
            //Exit early if alpha is zero.
            if (alpha_a == 0)
                continue;

            if (alpha_a == 0xFF || alpha_b == 0) {
                red = (color >> 16) & 0xFF;
                green = (color >> 8) & 0xFF;
                blue = (color >> 0) & 0xFF;
                alpha = alpha_a;
            }
            else {
                //Alpha composite
                float faa = alpha_a / 256.0;
                float fab = alpha_b / 256.0;
                float fra = ((color >> 16) & 0xFF) / 256.0;
                float frb = ti->data[data_index+2] / 256.0;
                float fga = ((color >> 8) & 0xFF) / 256.0;
                float fgb = ti->data[data_index+1] / 256.0;
                float fba = ((color >> 0) & 0xFF) / 256.0;
                float fbb = ti->data[data_index+0] / 256.0;

                float alpha_o = faa + fab*(1.0 - faa);
                fra = (fra*faa + frb*fab*(1.0 - faa)) / alpha_o;
                red = (uint8_t)(fra * 256.0);
                fga = (fga*faa + fgb*fab*(1.0 - faa)) / alpha_o;
                green = (uint8_t)(fra * 256.0);
                fba = (fba*faa + fbb*fab*(1.0 - faa)) / alpha_o;
                blue = (uint8_t)(fba * 256.0);
                alpha = 0xFF;
            }
            ti->data[data_index+0] = blue;
            ti->data[data_index+1] = green;
            ti->data[data_index+2] = red;
            ti->data[data_index+3] = alpha;
        }
        ++row;
        --rel_pen_y;
    }

    *pen_x += ((FT_Glyph)bmg)->advance.x >> 16;
    *pen_y += ((FT_Glyph)bmg)->advance.y >> 16;
}

void OSD::set_text(OSDTextItem *ti, std::string text) {
    ti->text = text;
    FT_Stroker_Set(
        stroker,
        ti->stroke*64,
        FT_STROKER_LINECAP_SQUARE,
        FT_STROKER_LINEJOIN_ROUND,
        0
    );
    FT_Set_Char_Size(fontface, 0, ti->point_size*64, 96, 96);

    //First, calculate the size of the bitmap surface we need
    int item_width = 0, item_height = 0;
    FT_BBox total_bbox = {0,0,0,0};
    for (unsigned int i = 0; i < ti->text.length(); ++i) {
        FT_UInt gindex = FT_Get_Char_Index(fontface, ti->text[i]);
        FT_Load_Glyph(fontface, gindex, FT_LOAD_DEFAULT);
        FT_Glyph glyph;
        FT_Get_Glyph(fontface->glyph, &glyph);
        FT_Glyph_StrokeBorder(&glyph, stroker, false, true);

        FT_BBox bbox;
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &bbox);
        FT_Done_Glyph(glyph);

        if (i+1 == ti->text.length())
            item_width += bbox.xMax - bbox.xMin;
        else
            item_width += glyph->advance.x >> 16;

        if (bbox.yMin < total_bbox.yMin)
            total_bbox.yMin = bbox.yMin;
        if (bbox.yMax > total_bbox.yMax)
            total_bbox.yMax = bbox.yMax;
        if (bbox.xMin < total_bbox.xMin)
            total_bbox.xMin = bbox.xMin;
        if (bbox.xMax > total_bbox.xMax)
            total_bbox.xMax = bbox.xMax;
    }
    item_height = total_bbox.yMax - total_bbox.yMin + 1;

    int pen_y = -total_bbox.yMin;
    int pen_x = -total_bbox.xMin;

    //OSD Quirk: The item width must be divisible by 2.
    //If not, the OSD item shifts rapidly side-to-side.
    //Bad timings?
    if (item_width % 2 != 0)
        ++item_width;

    ti->imp_attr.rect.p1.x = ti->imp_attr.rect.p0.x + item_width - 1;
    ti->imp_attr.rect.p1.y = ti->imp_attr.rect.p0.y + item_height - 1;

    free(ti->data);
    ti->data = (uint8_t*)malloc(item_width * item_height * 4);
    ti->imp_attr.data.picData.pData = ti->data;
    memset(ti->data, 0, item_width * item_height * 4);

    //Then, render the stroke & text
    for (unsigned int i = 0; i < ti->text.length(); ++i) {
        FT_UInt gindex = FT_Get_Char_Index(fontface, ti->text[i]);
        FT_Load_Glyph(fontface, gindex, FT_LOAD_DEFAULT);
        FT_Glyph glyph;
        FT_Get_Glyph(fontface->glyph, &glyph);
        FT_Glyph stroked;
        FT_Get_Glyph(fontface->glyph, &stroked);

        FT_Glyph_StrokeBorder(&stroked, stroker, false, true);

        int cpx = pen_x;
        int cpy = pen_y;

        FT_Glyph_To_Bitmap(&stroked, FT_RENDER_MODE_NORMAL, NULL, true);
        FT_BitmapGlyph strokeBMG = (FT_BitmapGlyph)stroked;
        draw_glyph(ti, strokeBMG, &cpx, &cpy, item_height, item_width, 0x0);
        FT_Done_Glyph(stroked);

        FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, true);
        FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;
        draw_glyph(ti, bitmapGlyph, &pen_x, &pen_y, item_height, item_width, 0xFFFFFFFF);
        FT_Done_Glyph(glyph);
    }
}

bool OSD::init() {
    int ret;
    std::cout << "OSDINIT" << std::endl;

    if (freetype_init()) {
        std::cout << "FREETYPE init failed." << std::endl;
        return true;
    }

    ret = IMP_OSD_CreateGroup(0);
    if (ret < 0) {
        std::cout << "IMP_OSD_CreateGroup() == " << ret << std::endl;
        return true;
    }

    timestamp.stroke = 3;
    timestamp.point_size = 32;
    timestamp.color = 0xFFFFFFFF;
    timestamp.stroke_color = 0x000000FF;
    timestamp.imp_rgn = IMP_OSD_CreateRgn(NULL);
    timestamp.imp_attr.type = OSD_REG_PIC;
    timestamp.imp_attr.rect.p0.x = 5;
    timestamp.imp_attr.rect.p0.y = 5;
    timestamp.imp_attr.fmt = PIX_FMT_BGRA;
    timestamp.data = NULL;
    timestamp.imp_attr.data.picData.pData = timestamp.data;
    set_text(&timestamp, " ");
    IMP_OSD_RegisterRgn(timestamp.imp_rgn, 0, NULL);
    IMP_OSD_SetRgnAttr(timestamp.imp_rgn, &timestamp.imp_attr);

    IMP_OSD_GetGrpRgnAttr(timestamp.imp_rgn, 0, &timestamp.imp_grp_attr);
    timestamp.imp_grp_attr.show = 1;
    timestamp.imp_grp_attr.layer = 1;
    timestamp.imp_grp_attr.scalex = 1;
    timestamp.imp_grp_attr.scaley = 1;
    IMP_OSD_SetGrpRgnAttr(timestamp.imp_rgn, 0, &timestamp.imp_grp_attr);

    ret = IMP_OSD_Start(0);
    if (ret < 0) {
        std::cout << "IMP_OSD_Start() == " << ret << std::endl;
        return true;
    }

    std::cout << "OSDINIT DONE" << std::endl;
    return false;
}

void OSD::update() {
    //Called every frame by the encoder.

    //Update timestamp time
    time_t time = time(NULL);
    if (last_ts_time != time) {
        last_ts_time = time;
        struct tm *ltime = localtime(&time);
        char formatted[256];
        strftime(formatted, 256, "%I:%M:%S %p %m/%d/%Y", ltime);
        formatted[255] = '\0';
        set_text(&timestamp, std::string(formatted));
        IMP_OSD_SetRgnAttr(timestamp.imp_rgn, &timestamp.imp_attr);
    }
}
