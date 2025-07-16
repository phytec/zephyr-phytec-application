/*
 * Copyright (c) 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(capture_testpattern, CONFIG_APP_LOG_LEVEL);

#include "check_test_pattern.h"

#define CONFIG_VIDEO_WIDTH 640
#define CONFIG_VIDEO_HEIGHT 480

/* size of stack area used by each thread */
#define CAPTURE_TESTPATTERN_STACKSIZE		8192
/* scheduling priority used by each thread */
#define CAPTURE_TESTPATTERN_PRIORITY		5


int capture_testpattern(void)
{
	struct video_buffer *buffers[2];
	struct video_buffer *vbuf = &(struct video_buffer){};
	const struct device *display_dev;
	struct video_format fmt;
	struct video_caps caps;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	unsigned int frame = 0;
	const struct device *video_dev;
	size_t bsize;
	int i = 0;
	int err;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}



	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s device is not ready", video_dev->name);
		return 0;
	}

	LOG_INF("- Device name: %s", video_dev->name);

	/* Get capabilities */
	if (video_get_caps(video_dev, VIDEO_EP_OUT, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	LOG_INF("- Capabilities:");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* four %c to string */
		LOG_INF("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]",
			(char)fcap->pixelformat, (char)(fcap->pixelformat >> 8),
			(char)(fcap->pixelformat >> 16), (char)(fcap->pixelformat >> 24),
			fcap->width_min, fcap->width_max, fcap->width_step, fcap->height_min,
			fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	if (video_get_format(video_dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	/* Set format */
	fmt.width = CONFIG_VIDEO_WIDTH;
	fmt.height = CONFIG_VIDEO_HEIGHT;
	fmt.pitch = fmt.width * 2;

	if (video_set_format(video_dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to set up video format");
		return 0;
	}

	LOG_INF("- Format: %c%c%c%c %ux%u %u", (char)fmt.pixelformat, (char)(fmt.pixelformat >> 8),
		(char)(fmt.pixelformat >> 16), (char)(fmt.pixelformat >> 24), fmt.width, fmt.height,
		fmt.pitch);

	if (!video_get_frmival(video_dev, VIDEO_EP_OUT, &frmival)) {
		LOG_INF("- Default frame rate : %f fps",
		       1.0 * frmival.denominator / frmival.numerator);
	}

	LOG_INF("- Supported frame intervals for the default format:");
	memset(&fie, 0, sizeof(fie));
	fie.format = &fmt;
	while (video_enum_frmival(video_dev, VIDEO_EP_OUT, &fie) == 0) {
		if (fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
			LOG_INF("   %u/%u ", fie.discrete.numerator, fie.discrete.denominator);
		} else {
			LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
			       fie.stepwise.min.numerator, fie.stepwise.min.denominator,
			       fie.stepwise.max.numerator, fie.stepwise.max.denominator,
			       fie.stepwise.step.numerator, fie.stepwise.step.denominator);
		}
		fie.index++;
	}

	/* Set controls */
	if (IS_ENABLED(CONFIG_VIDEO_CTRL_HFLIP)) {
		video_set_ctrl(video_dev, VIDEO_CID_HFLIP, (void *)1);
	}

	if (IS_ENABLED(CONFIG_VIDEO_CTRL_VFLIP)) {
		video_set_ctrl(video_dev, VIDEO_CID_VFLIP, (void *)1);
	}

	video_set_ctrl(video_dev, VIDEO_CID_TEST_PATTERN, (void *)1);

	/* Size to allocate for each buffer */
	if (caps.min_line_count == LINE_COUNT_HEIGHT) {
		bsize = fmt.pitch * fmt.height;
	} else {
		bsize = fmt.pitch * caps.min_line_count;
	}

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		/*
		 * For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
		 * buffer alignment is needed in order to achieve the best performance
		 */
		buffers[i] = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							K_FOREVER);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}

		video_enqueue(video_dev, VIDEO_EP_OUT, buffers[i]);
	}

	/* Start video capture */
	if (video_stream_start(video_dev)) {
		LOG_ERR("Unable to start capture (interface)");
		return 0;
	}

	display_blanking_off(display_dev);

	const lv_img_dsc_t video_img = {
		.header.w = CONFIG_VIDEO_WIDTH,
		.header.h = CONFIG_VIDEO_HEIGHT,
		.data_size = CONFIG_VIDEO_WIDTH * CONFIG_VIDEO_HEIGHT * sizeof(lv_color_t),
		.header.cf = LV_COLOR_FORMAT_NATIVE,
		.data = (const uint8_t *)buffers[0]->buffer,
	};

	lv_obj_t *screen = lv_img_create(lv_scr_act());

	LOG_INF("- Capture started");

	while (1) {

		err = video_dequeue(video_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		LOG_DBG("Got frame %u! size: %u; timestamp %u ms", frame++, vbuf->bytesused,
		       vbuf->timestamp);

		lv_img_set_src(screen, &video_img);
		lv_obj_align(screen, LV_ALIGN_BOTTOM_LEFT, 0, 0);

		lv_task_handler();

		err = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

K_THREAD_DEFINE(capture_testpattern_tid, CAPTURE_TESTPATTERN_STACKSIZE,
		capture_testpattern, NULL, NULL, NULL,
		CAPTURE_TESTPATTERN_PRIORITY, 0, 0);
