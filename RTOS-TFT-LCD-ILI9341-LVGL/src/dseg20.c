/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#define LV_LVGL_H_INCLUDE_SIMPLE
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef DSEG20
#define DSEG20 1
#endif

#if DSEG20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+002D "-" */
    0x7f, 0xff, 0xe0,

    /* U+002E "." */
    0xf0,

    /* U+0030 "0" */
    0xff, 0xfb, 0xfd, 0xc0, 0x3c, 0x3, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x0, 0x0,
    0x3c, 0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x3,
    0xc0, 0x3c, 0x3, 0xff, 0xdb, 0xff,

    /* U+0031 "1" */
    0x5f, 0xff, 0xcf, 0xff, 0xf4,

    /* U+0032 "2" */
    0xff, 0xf3, 0xfd, 0x0, 0x30, 0x3, 0x0, 0x30,
    0x3, 0x0, 0x30, 0x3, 0x0, 0x33, 0xfc, 0x3f,
    0xcc, 0x0, 0xc0, 0xc, 0x0, 0xc0, 0xc, 0x0,
    0xc0, 0xc, 0x0, 0xff, 0xcb, 0xff,

    /* U+0033 "3" */
    0xff, 0xf7, 0xfd, 0x0, 0x30, 0x3, 0x0, 0x30,
    0x3, 0x0, 0x30, 0x3, 0x0, 0x33, 0xfc, 0x7f,
    0xf0, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0x3f, 0xd7, 0xff,

    /* U+0034 "4" */
    0x0, 0x1c, 0x1, 0xc0, 0x3c, 0x3, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3f, 0xfc, 0x3f,
    0xf0, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0x0, 0x30,

    /* U+0035 "5" */
    0xff, 0xeb, 0xfc, 0xc0, 0xc, 0x0, 0xc0, 0xc,
    0x0, 0xc0, 0xc, 0x0, 0xc0, 0xf, 0xfc, 0x3f,
    0xf0, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0x3f, 0xd3, 0xff,

    /* U+0036 "6" */
    0xff, 0xeb, 0xfc, 0xc0, 0xc, 0x0, 0xc0, 0xc,
    0x0, 0xc0, 0xc, 0x0, 0xc0, 0xf, 0xfc, 0x3f,
    0xfc, 0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x3,
    0xc0, 0x3c, 0x3, 0xff, 0xdb, 0xff,

    /* U+0037 "7" */
    0xff, 0xfb, 0xfd, 0xc0, 0x3c, 0x3, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x1, 0x0,
    0x30, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0x0, 0x30,

    /* U+0038 "8" */
    0xff, 0xfb, 0xfd, 0xc0, 0x3c, 0x3, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3f, 0xfc, 0x3f,
    0xfc, 0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x3,
    0xc0, 0x3c, 0x3, 0xff, 0xdb, 0xff,

    /* U+0039 "9" */
    0xff, 0xfb, 0xfd, 0xc0, 0x3c, 0x3, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3f, 0xfc, 0x3f,
    0xf0, 0x3, 0x0, 0x30, 0x3, 0x0, 0x30, 0x3,
    0x0, 0x30, 0x3, 0x3f, 0xd3, 0xff,

    /* U+003A ":" */
    0xf0, 0x0, 0x3c
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 64, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 261, .box_w = 10, .box_h = 2, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 4, .adv_w = 0, .box_w = 2, .box_h = 2, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 261, .box_w = 2, .box_h = 19, .ofs_x = 12, .ofs_y = 1},
    {.bitmap_index = 40, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 261, .box_w = 12, .box_h = 19, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 129, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 189, .adv_w = 261, .box_w = 12, .box_h = 19, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 218, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 261, .box_w = 12, .box_h = 20, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 64, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xd, 0xe
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 15, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 48, .range_length = 11, .glyph_id_start = 4,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t dseg20 = {
#else
lv_font_t dseg20 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if DSEG20*/

