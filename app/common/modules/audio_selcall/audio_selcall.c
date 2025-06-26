/*
 * Copyright 2025 PHYTEC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/audio/codec.h>

#include "selcall_tones.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(audio, CONFIG_APP_LOG_LEVEL);

#define SLEEP_TIME_S		1

/* size of stack area used by each thread */
#define AUDIO_STACKSIZE		1024
/* scheduling priority used by each thread */
#define AUDIO_PRIORITY		5

#define SAMPLE_FREQUENCY	48000U
#define SAMPLE_BIT_WIDTH	(16U)
#define BYTES_PER_SAMPLE	sizeof(int16_t)
#define NUMBER_OF_CHANNELS	(2U)

#define TIMEOUT			(2000U)

#define NUM_BLOCKS 4
#define BLOCK_SIZE (2 * sizeof(tone_697))

static void *tx_block[NUM_BLOCKS];

/* Fill buffer with sine wave on left and right channel */

static void fill_buf(int16_t *tx_block, int16_t *origin)
{
	for (int i = 0; i < SELCALL_TONE_LEN; i++) {
		/* Left channel is sine wave */
		tx_block[2 * i] = origin[i];
		// /* Right channel is the same sine wave */
		tx_block[2 * i + 1] = origin[i];
	}
}


#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

#define I2S_CODEC_TX		DT_ALIAS(i2s_codec_tx)
#if !DT_NODE_HAS_STATUS_OKAY(I2S_CODEC_TX)
#error "Unsupported board: si2c-codec-tx devicetree alias is not defined"
#endif
#define AUDIO_CODEC		DT_NODELABEL(audio_codec)

static const struct device *const i2s_dev_codec = DEVICE_DT_GET(I2S_CODEC_TX);
static const struct device *const codec_dev = DEVICE_DT_GET(AUDIO_CODEC);


int init_audio(void)
{
	struct audio_codec_cfg audio_cfg;
	audio_property_value_t volume;
	struct i2s_config i2s_cfg;
	int ret;

	if (!device_is_ready(i2s_dev_codec)) {
		LOG_ERR("%s is not ready", i2s_dev_codec->name);
		return -ENODEV;
	}

	if (!device_is_ready(codec_dev)) {
		LOG_ERR("%s is not ready", codec_dev->name);
		return -ENODEV;
	}

	/* Configure the audio codec */
	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels =  2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = &tx_0_mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	ret = audio_codec_configure(codec_dev, &audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure audio coded");
		return ret;
	}

	k_sleep(K_SECONDS(1));

	volume.mute = false;
	volume.vol = -50;
	ret = audio_codec_set_property(codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
				       AUDIO_CHANNEL_ALL, volume);
	if (ret < 0) {
		LOG_ERR("Failed to set output volume");
		return ret;
	}

	/* Configure I2S stream */
	i2s_cfg.word_size = SAMPLE_BIT_WIDTH;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	i2s_cfg.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg.mem_slab = &tx_0_mem_slab;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;
	ret = i2s_configure(i2s_dev_codec, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream");
		return ret;
	}

	return 0;
};


int play_tone(const int16_t *tone)
{
	k_timepoint_t end;
	int idx = 0;
	int ret = 0;

	for (int i = 0; i < NUM_BLOCKS; i++)
		fill_buf((uint16_t *)tx_block[i], (int16_t *)tone);

	end = sys_timepoint_calc(K_MSEC(100));
	while (!sys_timepoint_expired(end)) {
		ret = i2s_write(i2s_dev_codec, tx_block[idx], BLOCK_SIZE);
		if (ret < 0) {
			LOG_INF("Could not write TX buffer");
			return ret;
		}
		idx = (idx + 1) % NUM_BLOCKS;
	}

	k_sleep(K_MSEC(10));
	return ret;
}


void audio(void)
{
	int ret;

	LOG_INF("I2S example application");

	init_audio();

	/* Prepare all TX block */
	for (int i = 0; i < NUM_BLOCKS; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[i], K_FOREVER);
		if (ret < 0) {
			LOG_INF("Failed to allocate TX block");
			return;
		}
		fill_buf((uint16_t *)tx_block[i], (int16_t *)tone_697);
	}

	/* Start I2S stream */
	LOG_INF("Starting I2S stream\n");
	/* Test should enable when testing this interface only */
#ifndef CONFIG_PHYTEC_KIT_TESTING
	audio_codec_start_output(codec_dev);
#endif /* CONFIG_PHYTEC_TESTING */

	/* Write first block */
	ret = i2s_write(i2s_dev_codec, tx_block[0], BLOCK_SIZE);
	if (ret < 0) {
		LOG_INF("Could not write TX buffer");
		return;
	}

	LOG_INF("First I2S block written, triggering I2S");
	/* Trigger the I2S transmission */
	ret = i2s_trigger(i2s_dev_codec, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		LOG_INF("Could not trigger I2S tx");
		return;
	}

	while (1) {
		ret = play_tone(tone_852);
		if (ret < 0) {
			LOG_INF("Failed to play tone 852");
		}
		ret = play_tone(tone_1393);
		if (ret < 0) {
			LOG_INF("Failed to play tone 852");
		}
		ret = play_tone(tone_1020);
		if (ret < 0) {
			LOG_INF("Failed to play tone 852");
		}
		ret = play_tone(tone_2100);
		if (ret < 0) {
			LOG_INF("Failed to play tone 852");
		}
		ret = play_tone(tone_1153);
		if (ret < 0) {
			LOG_INF("Failed to play tone 852");
		}
		k_sleep(K_SECONDS(SLEEP_TIME_S));
	};
}

K_THREAD_DEFINE(audio_tid, AUDIO_STACKSIZE, audio, NULL, NULL,
		NULL, AUDIO_PRIORITY, 0, 0);
