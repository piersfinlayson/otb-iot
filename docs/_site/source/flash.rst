..
 OTB-IOT - Out of The Box Internet Of Things
 Copyright (C) 2017 Piers Finlayson

Flash
=====

Map
---

The flash map for otb-iot is provided below.  Note that this article may get out of date from time to time.  The master version of this map can be found in `otb_flash.h`_.

.. _otb_flash.h: https://github.com/piersfinlayson/otb-iot/blob/master/include/otb_flash.h

========== ========= =====================
 Location   Length    Contents
========== ========= =====================
0x0         0x6000    Bootloader
0x6000      0x2000    Bootloader Config
0x8000      0xF8000   Application slot 0 (upgradeable, default), only 0xf4000 may be used
0x100000    0x1000    Logs (only 0x400 bytes are used)
0x101000    0x1000    Last reboot reason (only 0x200 bytes are used)
0x102000    0xFE000   Unused
0x200000    0x1000    otb-iot application configuration
0x201000    0x7000    Reserved
0x208000    0xF8000   Application slot 1 (upgradeable), only 0xf4000 may be used
0x300000    0x1000    Reserved
0x301000    0x7000    Reserved
0x308000    0xF4000   Factory application image (treated as read only)
0x3FC000    0x4000    Used by ESP8266 SDK
========== ========= =====================

Notes
-----

1 As can be seen, despite there being 0xF8000 bytes available for application images in slot 0 and 1, the factory image only has 0xF4000 available.

2 The last reboot reason is only written by otb-iot if it differs from the value already contained within this location.  This avoids circular reboots for the same reason causing serious degredation of the flash chip (by too many erases taking place).

3 Logs are only stored in flash upon a reboot (assuming the device is able to store in flash before the reboot takes place - if power is removed this is not possible).

4 There is no separate SPIFFS filesystem in otb-iot.  This is because there is very little information required to be served up as files (from the captive portal) - the filesystem for the HTTP server is linked into the application image.  This has the added benefit of upgrading the HTTP files on OTA upgrade.

5 Neither bootloader nor otb-iot configuration are written to the device as part of the flash_initial process.  The bootloader and application software test the configuration on flash, and if it is invalid a new version is provided.

6 It is important that the application images and factory image are at the same offset from the beginning of the MB they are stored within, as the bootloader knows this offset, loads the correct 1MB of flash (aligned on a 1MB boundary) and jumps to a location in the application image based on this offset.

