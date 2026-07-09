.. zephyr:code-sample:: reel-board-ble-color-sync
   :name: reel_board BLE color sync + sensor dashboard
   :relevant-api: bt_gap bluetooth sensor_interface character_framebuffer_interface

   Synchronize an RGB color across reel boards over connectionless BLE and show
   sensor data on the e-paper display.

Overview
********

A demo for the PHYTEC **reel board** (nRF52840) that combines connectionless
Bluetooth Low Energy synchronization with an e-paper sensor dashboard. Every
board runs the *same* firmware -- there is no master/slave: each board is at
once a broadcaster and an observer.

- **Press the user button** -- the on-board RGB LED advances to the next color
  in the palette, and that color is broadcast over BLE so every other board
  mirrors it.
- **The e-paper display** shows the latest temperature, humidity, acceleration
  (X/Y/Z), ambient light, proximity and the currently selected color, refreshed
  every 2 seconds.

Flash two or more boards, press the button on any one of them, and watch the
rest converge to the same color.

How it works
************

Color synchronization (``src/ble_sync.c``)
==========================================

Synchronization is fully connectionless -- no GATT, no pairing, no connections.

- Each board sends **non-connectable, identity-address advertising**
  (``BT_LE_ADV_NCONN_IDENTITY``) carrying a small manufacturer-specific payload,
  while at the same time running a **continuous passive scan**.
- The 5-byte payload is:

  ===== ============= ==============================================
  Bytes Field         Notes
  ===== ============= ==============================================
  0..1  Company ID    ``0xFFFF``, reserved for testing/local use (LE)
  2     Magic marker  ``0x52``, so unrelated advertisers are ignored
  3     Sequence no.  Monotonic, wraps; provides ordering
  4     Color index   Palette index, mirrored by every board
  ===== ============= ==============================================

- Ordering is **"latest press wins"**: the sequence number is compared with
  signed wrap-around arithmetic. When a board sees a *newer* sequence number it
  adopts the color, **re-broadcasts it** (so late joiners and boards out of
  radio range still converge), and applies it to the local LED.
- Scanning runs with **duplicate filtering disabled** -- because every board
  keeps a stable identity address, the controller would otherwise report a peer
  only once and later color changes would be missed.

Color palette (``src/led.c``)
=============================

The RGB LED is driven through the ``led0``/``led1``/``led2`` GPIO aliases
(active-low, handled by the devicetree flags). The palette cycles through:

``Off -> Red -> Green -> Blue -> Yellow -> Cyan -> Magenta -> White -> Off ...``

Sensors (``src/sensors.c``)
===========================

Read over I²C through the Zephyr sensor API:

- **HDC1010** -- ambient temperature and humidity
- **MMA8652FC** -- 3-axis acceleration
- **APDS9960** -- ambient light and proximity

Missing or unresponsive sensors are reported but do not stop the application;
the dashboard shows ``---`` for any sensor that is unavailable.


Display (``src/display.c``)
===========================

The SSD16xx e-paper panel (250x122) is driven through the Character Framebuffer
(CFB) subsystem using the built-in 10x16 font. A full refresh takes roughly a
second, so it is always rendered from the **system workqueue**, never inline in
an input or Bluetooth callback.

E-paper SPI to SPIM overlay (``boards/reel_board.overlay``)
===========================================================

The board default drives the e-paper over the interrupt-per-byte
``nordic,nrf-spi``. The overlay switches ``spi1`` to the EasyDMA-based
``nordic,nrf-spim`` so the whole frame transfers with a single completion
interrupt. This means an e-paper refresh no longer hogs the CPU and can run
concurrently with BLE advertising/scanning without delaying the controller's
radio-event prepare -- keeping the radio always on for low-latency color sync.

Building and Running
********************

Build and flash to a reel board:

.. zephyr-app-commands::
   :zephyr-app: app/reel_board_ble_broadcast
   :board: reel_board
   :goals: build flash
   :compact:

For revision 2 hardware, use the revision suffix ``reel_board@2``. Repeat for
each board you want to synchronize.

Sample Output
=============

On the serial console (output comes from the logging subsystem, so each line is
prefixed with a timestamp, level and module name):

.. code-block:: console

   [00:00:00.123,000] <inf> app: reel_board BLE color sync + sensor dashboard
   [00:00:00.456,000] <inf> ble_sync: Bluetooth initialized
   [00:00:00.457,000] <inf> ble_sync: BLE color sync active (advertising + scanning)
   [00:00:05.789,000] <inf> ble_sync: TX color=1 seq=1
   [00:00:07.012,000] <inf> ble_sync: RX color=2 seq=2

- ``TX color=... seq=...`` -- this board broadcast a locally chosen color.
- ``RX color=... seq=...`` -- this board adopted a newer color from a peer.

Requirements
************

- Two or more PHYTEC reel boards (a single board still builds, runs, displays
  sensor data and cycles its own LED -- there is just nothing to sync with).
