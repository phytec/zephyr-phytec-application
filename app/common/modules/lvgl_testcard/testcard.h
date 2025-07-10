#include "testcard_720x1280.h"

#define IMAGE_META_SIZE		16

const lv_image_dsc_t img_testcard_rgb = {
    .header = {
        .cf = LV_COLOR_FORMAT_RGB888,
        .w = 720,
        .h = 1280,
        .stride = 720 * 3,
    },
    .data_size = sizeof(MagickImage) - IMAGE_META_SIZE,
    .data = &MagickImage[IMAGE_META_SIZE],
};
