/* Demo BLE Battery GATT Service. */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adhd_main, LOG_LEVEL_DBG);

#include "app_version.h"
#include "battery.h"
#include "haptics.h"

/* Connection state management. */
static volatile bool ble_connected	= false;
static volatile bool alarm_started	= false;

/* Battery service periodic update interval. */
static const k_timeout_t BAS_UPDATE_INTERVAL = K_SECONDS(1);

/* BLE advertisement params (for GAP). */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL))
};

/* Callbacks for connection/disconnection status (GAP). */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err 0x%02x).", err);
	}
	else {
		ble_connected = true;
		LOG_INF("Connected.");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ble_connected = false;
	LOG_INF("Disconnected (reason 0x%02x).", reason);
	bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY); /* Clear pairing info for next time pairing. */
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Callbacks for pairing (SMP). */
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
	.passkey_confirm = NULL
};

/* Runtime settings override. */
static void settings_runtime_load(void)
{
#if defined(CONFIG_BT_DIS_FW_REV)
	settings_runtime_set("bt/dis/fw", APP_VERSION_STRING, sizeof(APP_VERSION_STRING));
#endif
}

/* Initialize the BLE stack and start advertising. */
static int bt_ready(void)
{
	/* Register SMP callbacks for pairing. */
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Initialize BLE stack and signal ready when done. */
	int err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d).", err);
		return 1;
	}
	LOG_INF("Bluetooth initialized.");

	/* Load BLE settings. */
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}
	settings_runtime_load();

	/* Start BLE advertising. */
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_INF("Advertising failed to start (err %d).", err);
		return 2;
	}
	LOG_INF("Advertising successfully started.");
	
	return 0; 
}

int main(void)
{
	/* Initialize peripherals. */
	gpio_init_haptics();

	/* Run BLE initialization routines. */
	if (bt_ready())
		return 1;

	/* Start periodic battery service function. */
	bas_start(BAS_UPDATE_INTERVAL);

	/* Application super loop. */
	while (true)
	{
		if (!ble_connected && !alarm_started) {
			alarm_started = true;
			haptic_response(alarm_started);
			
		}
		else if (alarm_started && ble_connected) {
			alarm_started = false;
			haptic_response(alarm_started);
		}
		k_msleep(10);
	}
	return 0;
}
