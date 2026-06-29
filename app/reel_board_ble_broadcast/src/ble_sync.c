/*
 * Copyright (c) 2026 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "ble_sync.h"

LOG_MODULE_REGISTER(ble_sync, LOG_LEVEL_INF);

/* Application marker so we ignore unrelated advertisers. */
#define SYNC_MAGIC 0x52

/*
 * Manufacturer-specific advertising payload:
 *   [0..1] company id (0xFFFF, reserved for testing/local use, little-endian)
 *   [2]    magic marker
 *   [3]    sequence number (latest-wins, wraps)
 *   [4]    color palette index
 */
enum {
	OFF_COMPANY_LO = 0,
	OFF_COMPANY_HI = 1,
	OFF_MAGIC = 2,
	OFF_SEQ = 3,
	OFF_COLOR = 4,
	PAYLOAD_LEN = 5,
};

static uint8_t mfg[PAYLOAD_LEN] = {
	[OFF_COMPANY_LO] = 0xFF,
	[OFF_COMPANY_HI] = 0xFF,
	[OFF_MAGIC] = SYNC_MAGIC,
	[OFF_SEQ] = 0,
	[OFF_COLOR] = 0,
};

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg, sizeof(mfg)),
};

static struct k_mutex lock;
static uint8_t highest_seq;
static bool adv_active;
static ble_color_cb_t remote_color_cb;

/* Re-push the advertising payload from the system workqueue (decouples the
 * update from the caller's context, e.g. the BT RX thread).
 */
static void adv_update_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!adv_active) {
		return;
	}

	int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		LOG_ERR("adv update failed (err %d)", err);
	}
}
static K_WORK_DEFINE(adv_update_work, adv_update_work_fn);

/* Store seq/color into the advertised payload. */
static void set_payload(uint8_t seq, uint8_t color)
{
	highest_seq = seq;
	mfg[OFF_SEQ] = seq;
	mfg[OFF_COLOR] = color;

	k_work_submit(&adv_update_work);
}

void ble_sync_broadcast(uint8_t color_idx)
{
	uint8_t next;

	k_mutex_lock(&lock, K_FOREVER);
	next = highest_seq + 1U;

	set_payload(next, color_idx);
	k_mutex_unlock(&lock);
	LOG_INF("TX color=%u seq=%u", color_idx, next);
}

/* --- scanning --- */

struct parse_ctx {
	bool found;
	uint8_t seq;
	uint8_t color;
};

static bool parse_cb(struct bt_data *data, void *user_data)
{
	struct parse_ctx *ctx = user_data;

	if (data->type != BT_DATA_MANUFACTURER_DATA ||
	    data->data_len < PAYLOAD_LEN) {
		return true; /* keep parsing */
	}

	if (data->data[OFF_COMPANY_LO] != 0xFF ||
	    data->data[OFF_COMPANY_HI] != 0xFF ||
	    data->data[OFF_MAGIC] != SYNC_MAGIC) {
		return true;
	}

	ctx->found = true;
	ctx->seq = data->data[OFF_SEQ];
	ctx->color = data->data[OFF_COLOR];
	return false; /* stop parsing */
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *buf)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(rssi);
	ARG_UNUSED(type);

	struct parse_ctx ctx = {0};

	bt_data_parse(buf, parse_cb, &ctx);
	if (!ctx.found) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);
	bool newer = (int8_t)(ctx.seq - highest_seq) > 0;
	k_mutex_unlock(&lock);

	if (!newer) {
		return;
	}

	LOG_INF("RX color=%u seq=%u", ctx.color, ctx.seq);

	/* Adopt: rebroadcast so late joiners converge, then apply locally. */
	set_payload(ctx.seq, ctx.color);

	if (remote_color_cb) {
		remote_color_cb(ctx.color);
	}
}

/* Continuous passive scan with NO duplicate filtering: every board keeps a
 * stable identity address, so the controller would otherwise report a peer only
 * once and we'd miss later color changes. We need every advertising event.
 */
static const struct bt_le_scan_param scan_param = {
	.type = BT_LE_SCAN_TYPE_PASSIVE,
	.options = BT_LE_SCAN_OPT_NONE,
	.interval = BT_GAP_SCAN_FAST_INTERVAL_MIN, /* window == interval */
	.window = BT_GAP_SCAN_FAST_WINDOW,
};

/* Start advertising (current color payload) + scanning. */
static int start_radio(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		LOG_ERR("Advertising start failed (err %d)", err);
		return err;
	}
	adv_active = true;

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		LOG_ERR("Scan start failed (err %d)", err);
		return err;
	}

	return 0;
}

int ble_sync_init(ble_color_cb_t on_remote_color)
{
	int err;

	remote_color_cb = on_remote_color;
	k_mutex_init(&lock);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}
	LOG_INF("Bluetooth initialized");

	err = start_radio();
	if (err) {
		return err;
	}

	LOG_INF("BLE color sync active (advertising + scanning)");
	return 0;
}
