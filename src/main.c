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
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adhd_main, LOG_LEVEL_DBG);

/* Constants. */
static const k_timeout_t BAS_UPDATE_INTERVAL = K_SECONDS(10);

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
		LOG_INF("Connected.");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
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

/* Initialize the BLE stack and start advertising. */
static void bt_ready(void)
{
	int err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d).", err);
		return;
	}
	LOG_INF("Bluetooth initialized.");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_INF("Advertising failed to start (err %d).", err);
		return;
	}

	LOG_INF("Advertising successfully started.");
}

/* Notify the client about battery level via BAS (Battery Service). */
static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	/* Fake battery status, ie a simulation: decrement from 100 down to 0 and reset. 
	   In actual practice, you would have some kind of a battery_measure()
	   function call here which queries your battery status by reading an ADC
	   and doing calculations with reference to your battery's discharge curve!
	   Refer to https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/boards/nrf/battery
	*/
	battery_level--;
	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

int main(void)
{
	/* Run BLE initialization routines. */
	bt_ready();

	/* Register SMP callbacks for pairing. */
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Periodically update battery status on the GATT server via BAS. */
	while (true)
	{
		k_sleep(BAS_UPDATE_INTERVAL);
		bas_notify();
	}
	return 0;
}
