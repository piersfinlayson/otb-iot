/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2018 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define OTB_EEPROM_C
#include "otb.h"
#include "brzo_i2c.h"

#ifdef OTB_RBOOT_BOOTLOADER
#undef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

void ICACHE_FLASH_ATTR otb_eeprom_read(void)
{
#define TEMP_BUF_LEN 256
  unsigned char buf[TEMP_BUF_LEN];
  char fn_rc;
  brzo_i2c_info *bus;
  uint8_t ii;
  otb_eeprom_main_module *mod;
  otb_eeprom_info *eeprom_info_v;
  uint32_t rpi_rc;

  DEBUG("EEPROM: otb_eeprom_read entry");

  DETAIL("EEPROM: Searching internal bus for EEPROMs");

  bus = &otb_i2c_bus_internal;
  fn_rc = otb_eeprom_init(otb_eeprom_main_board_addr, bus);
  if (!fn_rc)
  {
    // Failed - bail
    INFO("EEPROM: Failed");
    goto EXIT_LABEL;
  }

#ifndef OTB_RBOOT_BOOTLOADER

  DETAIL("EEPROM: Attempt to read main eeprom at address 0x%02x", otb_eeprom_main_board_addr);
  otb_eeprom_read_all(otb_eeprom_main_board_addr, bus);
  os_memset(otb_eeprom_main_module_info_g, 0, sizeof(otb_eeprom_main_module_info) * OTB_EEPROM_MAX_MODULES);

  // Read the modules.
  // - If an non-espi device then use the standard format
  // - If an espi device then use the raspberry pi format 

  if (otb_eeprom_main_board_g != NULL)
  {
    for (ii = 0; ii < otb_eeprom_main_board_g->num_modules; ii++) 
    {
      if (ii >= OTB_EEPROM_MAX_MODULES)
      {
        WARN("EEPROM: Too many modules");
        break;
      }

      if (otb_eeprom_main_board_module_g[ii] != NULL)
      {
        DETAIL("EEPROM: Look for module %d eeprom at address 0x%02x", ii, otb_eeprom_main_board_module_g[ii]->address);
        
        eeprom_info_v = NULL;
        if (otb_eeprom_main_board_module_g[ii]->socket_type != OTB_EEPROM_MODULE_TYPE_RPI_HAT_ESPI)
        {
          eeprom_info_v = otb_eeprom_load_main_comp(otb_eeprom_main_board_module_g[ii]->address, bus, eeprom_info_v, OTB_EEPROM_INFO_TYPE_INFO, 1, NULL, 0);
          if (eeprom_info_v != NULL)
          {
            mod = (otb_eeprom_main_module *)otb_eeprom_load_main_comp(otb_eeprom_main_board_module_g[ii]->address, bus, eeprom_info_v, OTB_EEPROM_INFO_TYPE_MAIN_MODULE, 1, NULL, 0);
            if (mod != NULL)
            {
              // No errors - so store this module info off
              DETAIL("EEPROM: Found information for module %d", ii);
              otb_eeprom_main_module_info_g[ii].eeprom_info = eeprom_info_v;
              otb_eeprom_main_module_info_g[ii].module = mod;
              otb_eeprom_main_module_info_g[ii].main_board_mod = otb_eeprom_main_board_module_g[ii];
            }
          }
        }
        else
        {
          // Try reading an RPi EEPROM
          otb_eeprom_rpi_header rpi_hdr;
          rpi_rc = otb_eeprom_load_rpi_eeprom(otb_eeprom_main_board_module_g[ii]->address, bus, eeprom_info_v, OTB_EEPROM_INFO_RPI_TYPE_HEADER, &rpi_hdr);
        }
        
      }
    }
  }

#endif // OTB_RBOOT_BOOTLOADER

EXIT_LABEL:

  // Now do something with the info read from the eeprom
  otb_eeprom_process_all();

  DEBUG("EEPROM: otb_eeprom_read exit");

  return;
}

#ifndef OTB_RBOOT_BOOTLOADER

// Logic for RPi header processing:
// - Only look for RPi EEPROM if otb_eeprom_main_board_module is of appropriate type (and has appropriate address)
// - If module type is OTB_EEPROM_MODULE_TYPE_RPI_HAT_ESPI I know I should find a RPi header
// - Read in the RPI header and check
//   - signature
//   - version
//   - numatoms
//   - eeplen
// - Read in the vendor info atom
//   - atom header
//     - type
//     - count
//     - dlen (inc 2 byte crc)
//     - data
//       - uuid
//       - pid
//       - pver
//       - vslen/vs
//       - pslen/ps
//   - ignore other atoms for now
//   - but at some point want to process the custom header
//     (have my own serial number in - and need to figure out how to flash as
//     well)
// - Then do something with all of this info - like detect DS18B20 board, programming board, M-Bus Master Hat!

uint32_t ICACHE_FLASH_ATTR otb_eeprom_load_rpi_eeprom(uint8_t addr,
                                                      brzo_i2c_info *i2c_info,
                                                      otb_eeprom_info *eeprom_info,
                                                      uint32_t type,
                                                      otb_eeprom_rpi_header *hdr)
{
  uint32_t fn_rc;
  uint32_t rc = OTB_EEPROM_ERR;
  uint32_t signature;
  uint32_t read_len;
  uint32_t read_pos;
  int32_t remain_len;
  int ii, jj, kk;
  otb_eeprom_rpi_atom atom;
  uint8_t data[256];
  uint8_t uuid[(16*2)+4+1]; // 16 bytes (=32 chars), 4 -s, 1 NULL
  uint16_t pid;
  uint16_t pver;
  uint8_t pslen;
  uint8_t vstr[128];
  uint8_t vslen;
  uint8_t pstr[128];

  DEBUG("EEPROM: otb_eeprom_load_rpi_eeprom entry");

  OTB_ASSERT(type == OTB_EEPROM_INFO_RPI_TYPE_HEADER);
  read_pos = 0;
  read_len = sizeof(otb_eeprom_rpi_header);
  OTB_ASSERT(read_len < OTB_EEPROM_MAX_MAIN_COMP_LENGTH);

  fn_rc = otb_i2c_24xx128_read_data(addr,
                                    read_pos,
                                    read_len,
                                    (uint8_t *)hdr,
                                    i2c_info);
  if (!fn_rc)
  {
    // Error reading or reading to I2C bus
    WARN("EEPROM: Failed to read data from I2C bus otb_eeprom_rpi_header");
    rc |= OTB_EEPROM_ERR_I2C;
    goto EXIT_LABEL;
  }

  DETAIL("EEPROM: otb_eeprom_rpi_header:");
  DETAIL("EEPROM:   hdr.signature: 0x%02x%02x%02x%02x", hdr->signature[0], hdr->signature[1], hdr->signature[2], hdr->signature[3]);
  DETAIL("EEPROM:   hdr.version:   0x%02x", hdr->version);
  DETAIL("EEPROM:   hdr.reserved:  0x%02x", hdr->reserved);
  DETAIL("EEPROM:   hdr.numatoms:  0x%04x", hdr->numatoms);
  DETAIL("EEPROM:   hdr.eeplen:    0x%08x", hdr->eeplen);

  signature = *(uint32_t*)(&hdr->signature);
  if (signature == OTB_EEPROM_RPI_EEPROM_SIGNATURE)
  {
    DETAIL("EEPROM: Valid RPi Hat EEPROM found");
  }
  else
  {
    WARN("EEPROM: Invalid RPi Hat EEPROM signature found - should be 0x%08x was 0x%08x", OTB_EEPROM_RPI_EEPROM_SIGNATURE, signature);
    rc |= OTB_EEPROM_ERR_MAGIC;
    goto EXIT_LABEL;
  }

  remain_len = hdr->eeplen;
  if (remain_len < 0)
  {
    WARN("EEPROM: Eeprom data length too long 0x%08x", hdr->eeplen);
    rc |= OTB_EEPROM_ERR_LENGTH;
    goto EXIT_LABEL;
  }

  // Move to the end of the header
  read_pos += read_len;
  remain_len -= read_len;

  // Read in the atoms
  for (ii = 0; ii < hdr->numatoms; ii++)
  {
    read_len = sizeof(otb_eeprom_rpi_atom);
    DEBUG("EEPROM: Read atom header size %d", read_len);
    if (read_len > remain_len)
    {
      WARN("EEPROM: Not enough data on eeprom %d to read next atom %d", remain_len, read_len);
      rc |= OTB_EEPROM_ERR_LENGTH;
      goto EXIT_LABEL;
    }

    fn_rc = otb_i2c_24xx128_read_data(addr,
                                      read_pos,
                                      read_len,
                                      (uint8_t *)&atom,
                                      i2c_info);
    if (!fn_rc)
    {
      // Error reading or reading to I2C bus
      WARN("EEPROM: Failed to read data from I2C bus otb_eeprom_rpi_header");
      rc |= OTB_EEPROM_ERR_I2C;
      goto EXIT_LABEL;
    }

    // Move to the end of the atom structure (but not any data)
    read_pos += read_len;
    remain_len -= read_len;

    DETAIL("EEPROM: otb_eeprom_rpi_atom %d:", ii);
    DETAIL("EEPROM:   atom.type:     0x%04x", atom.type);
    DETAIL("EEPROM:   atom.count:    0x%04x", atom.count);
    DETAIL("EEPROM:   atom.dlen:     0x%08x", atom.dlen);

    read_len = atom.dlen;
    if (read_len > 256)
    {
      WARN("EEPROM: Too much atom data %d to read %d", read_len, 256);
      rc |= OTB_EEPROM_ERR_LENGTH;
      goto EXIT_LABEL;
    }
    fn_rc = otb_i2c_24xx128_read_data(addr,
                                      read_pos,
                                      read_len,
                                      data,
                                      i2c_info);
    if (!fn_rc)
    {
      // Error reading or reading to I2C bus
      WARN("EEPROM: Failed to read data from I2C bus otb_eeprom_rpi_header");
      rc |= OTB_EEPROM_ERR_I2C;
      goto EXIT_LABEL;
    }

    if (atom.type == OTB_EEPROM_RPI_ATOM_TYPE_VENDOR_INFO)
    {
      DEBUG("EEPROM: Reading RPi vendor info");
      // 22 comes from uuid, pid, pver, vslen, pslen
      if (read_len < 22)
      {
        WARN("EEPROM: Atom data %d not long enough %d", read_len, 22);
        rc |= OTB_EEPROM_ERR_LENGTH;
        goto EXIT_LABEL;
      }
 
      // Build UUID string
      for (jj = 0, kk = 0; jj < 16; jj++)
      {
        os_sprintf(uuid+kk, "%02x", data[jj]);
        kk += 2;
        if ((jj == 3) || (jj == 5) || (jj == 7) || (jj == 9))
        {
          os_sprintf(uuid+kk, "-");
          kk++;
        }
        OTB_ASSERT(kk < (16*2)+4+1);
      }
      uuid[jj*2] = 0; // NULL terminate

      // get pid and pver
      pid = (data[17] << 8) | data[16];
      pver = (data[19] << 8) | data[18];

      // Get vendor and product strings
      vslen = data[20];
      pslen = data[21];
      if ((vslen > 127) || (pslen > 128))
      {
        WARN("EEPROM: Product or version string length too long %d", 128);
        rc |= OTB_EEPROM_ERR_LENGTH;
        goto EXIT_LABEL;
      }
      if ((vslen + pslen) > (read_len - 22))
      {
        WARN("EEPROM: Not enough space in atom %d for vendor and product strings %d", read_len-22, vslen+pslen);
        rc |= OTB_EEPROM_ERR_LENGTH;
        goto EXIT_LABEL;
      }
      os_memset(vstr, 0, 128);
      os_memset(pstr, 0, 128);
      os_memcpy(vstr, data+22, vslen);
      os_memcpy(pstr, data+22+vslen, pslen);

      if (!os_memcmp(vstr,
                     OTB_EEPROM_RPI_HAT_VENDOR_PACKOM,
                     os_strlen(OTB_EEPROM_RPI_HAT_VENDOR_PACKOM)))
      {
        INFO("OTB: RPi Hat found: %s %s", vstr, pstr) ;
        if (!os_memcmp(pstr,
                       OTB_EEPROM_RPI_HAT_PRODUCT_MBUS_MASTER,
                       os_strlen(OTB_EEPROM_RPI_HAT_PRODUCT_MBUS_MASTER)))
        {
          otb_mbus_hat_init();
        }
        if (!os_memcmp(pstr,
                       OTB_EEPROM_RPI_HAT_PRODUCT_ESPI_PROG,
                       os_strlen(OTB_EEPROM_RPI_HAT_PRODUCT_ESPI_PROG)))
        {
          // XXX Initialize ESPi Programmer (no-op)
        }
      }

    }

    // Skip to the end of the atom
    read_pos += atom.dlen;
    remain_len -= atom.dlen;
  }

  rc = otb_eeprom_rpi_hat_store_info(uuid, pid, pver, vstr, pstr);
  if (!rc)
  {
    WARN("EEPROM: Failed to store RPi Hat info");
    goto EXIT_LABEL;
  }
  otb_eeprom_rpi_hat_log_info();

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_load_rpi_eeprom exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_eeprom_rpi_hat_log_info(void)
{
  DEBUG("EEPROM: otb_eeprom_rpi_hat_log_info entry");

  if (otb_eeprom_rpi_hat_info_g != NULL)
  {
    DETAIL("EEPROM: RPi Hat Info");
    DETAIL("EEPROM:   UUID:           %s",
           otb_eeprom_rpi_hat_info_g->uuid);
    DETAIL("EEPROM:   Product ID:     0x%04x",
           otb_eeprom_rpi_hat_info_g->pid);
    DETAIL("EEPROM:   Product Ver:    0x%04x",
           otb_eeprom_rpi_hat_info_g->pver);
    DETAIL("EEPROM:   Vendor String:  %s",
           otb_eeprom_rpi_hat_info_g->vstr);
    DETAIL("EEPROM:   Product String: %s",
           otb_eeprom_rpi_hat_info_g->pstr);
  }

  DEBUG("EEPROM: otb_eeprom_rpi_hat_log_info exit");

  return;
}

bool ICACHE_FLASH_ATTR otb_eeprom_rpi_hat_store_info(unsigned char *uuid, uint16_t pid, uint16_t pver, unsigned char *vstr, unsigned char *pstr)
{
  bool rc = FALSE;
  int max_len = 256;
  int alloc_len;
  DEBUG("EEPROM: otb_eeprom_rpi_hat_store_info entry");

  alloc_len = sizeof(otb_eeprom_rpi_hat_info);
  otb_eeprom_rpi_hat_info_g = (otb_eeprom_rpi_hat_info *)os_malloc(alloc_len);
  if (otb_eeprom_rpi_hat_info_g == NULL)
  {
    WARN("EEEPROM: alloc failed %d", alloc_len);
    goto EXIT_LABEL;
  }

  otb_eeprom_rpi_hat_info_g->pid = pid;
  otb_eeprom_rpi_hat_info_g->pver = pver;

  rc = otb_eeprom_alloc_and_copy_str(&(otb_eeprom_rpi_hat_info_g->uuid), uuid, max_len);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  DEBUG("EEPROM: UUID location 0x%08x", otb_eeprom_rpi_hat_info_g->uuid);

  rc = otb_eeprom_alloc_and_copy_str(&(otb_eeprom_rpi_hat_info_g->pstr), pstr, max_len);
  if (!rc)
  {
    goto EXIT_LABEL;
  }

  rc = otb_eeprom_alloc_and_copy_str(&(otb_eeprom_rpi_hat_info_g->vstr), vstr, max_len);
  if (!rc)
  {
    goto EXIT_LABEL;
  }
  
  rc = TRUE;

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_rpi_hat_store_info exit");

  return rc;
}

bool ICACHE_FLASH_ATTR otb_eeprom_alloc_and_copy_str(unsigned char **dest, unsigned char *src, int max_len)
{
  int alloc_len;
  bool rc = FALSE;

  DEBUG("EEPROM: otb_eeprom_alloc_and_copy_str entry");

  alloc_len = os_strnlen(src, max_len);
  alloc_len++;
  DEBUG("EEPROM: Storing %s len %d", src, alloc_len);
  if (alloc_len > max_len)
  {
    WARN("EEPROM: String too long %d to store %d", alloc_len, max_len);
    goto EXIT_LABEL;
  }
  *dest = (unsigned char*)os_malloc(alloc_len);
  if (*dest == NULL)
  {
    WARN("EEEPROM: alloc failed %d", alloc_len);
    goto EXIT_LABEL;
  }
  DEBUG("EEPROM: Copying to 0x%08x", *dest)
  os_memcpy(*dest, src, alloc_len);
  (*dest)[alloc_len - 1] = 0;

  rc = TRUE;

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_alloc_and_copy_str exit");

  return rc;
}

#endif // OTB_RBOOT_BOOTLOADER

#ifndef OTB_RBOOT_BOOTLOADER

bool ICACHE_FLASH_ATTR otb_eeprom_module_present()
{
  bool rc = FALSE;
  int ii;

  DEBUG("EEPROM: otb_eeprom_module_present entry");

  if (otb_eeprom_main_board_g != NULL)
  {
    for (ii = 0; ii < OTB_EEPROM_MAX_MODULES; ii++) 
    {
      if (otb_eeprom_main_module_info_g[ii].eeprom_info != NULL)
      { 
        DEBUG("EEPROM: Have found at one set of module info");
        rc = TRUE;
        break;
      }
    }
  }

  DEBUG("EEPROM: otb_eeprom_module_present exit");

  return rc;
}

void ICACHE_FLASH_ATTR otb_eeprom_init_modules()
{
  int ii;
  os_timer_func_t *timer_func = otb_init_ds18b20;
  void *timer_param = NULL;
  int jj;
  int kk;
  uint32 gpios[2];
  uint32 gpios_munged;

  DEBUG("OTB: otb_eeprom_init_modules entry");

  //
  // XXX
  // 
  // This function only currently supported initialising the last mezzanine board found (only starts one timer function)
  //
  os_timer_disarm((os_timer_t*)&init_timer);

  // If there aren't any modules present
  // - set up stored GPIO status
  if (!otb_eeprom_module_present())
  {
    DETAIL("OTB: No modules present, initialise GPIOs and DS18B20");
    otb_gpio_apply_boot_state();
    goto EXIT_LABEL;
  }

  if (otb_eeprom_main_board_g != NULL)
  {
    for (ii = 0; ii < OTB_EEPROM_MAX_MODULES; ii++) 
    {
      if (otb_eeprom_main_module_info_g[ii].eeprom_info != NULL)
      { 
        DEBUG("EEPROM: Have found at one set of module info socket_type 0x%x", otb_eeprom_main_module_info_g[ii].main_board_mod->socket_type);
        OTB_ASSERT(otb_eeprom_main_module_info_g[ii].module != NULL);
        OTB_ASSERT(otb_eeprom_main_module_info_g[ii].main_board_mod != NULL);
        // Only interested in mezzanine boards (other sort is programming board, or mezz+prog combo)
        if ((otb_eeprom_main_module_info_g[ii].main_board_mod->socket_type == OTB_EEPROM_MODULE_TYPE_MEZZ) ||
            (otb_eeprom_main_module_info_g[ii].main_board_mod->socket_type == OTB_EEPROM_MODULE_TYPE_DOUBLE_MEZZ))
        {
          // Check out the GPIOs for this module
          // XXX Note relying on the num_pins actually being correct and not sending us off into hyperspace
          //     Should really check length as we do through
          for (jj = 0, kk = 0; jj < otb_eeprom_main_module_info_g[ii].main_board_mod->num_pins; jj++)
          {
            if (otb_eeprom_main_module_info_g[ii].main_board_mod->pin_info[jj].use == OTB_EEPROM_PIN_USE_GPIO)
            {
              // Bottom type of further_info is GPIO number
              gpios[kk] = (otb_eeprom_main_module_info_g[ii].main_board_mod->pin_info[jj].further_info & 0xff);
              kk++;
              if (kk >= 2)
              {
                // Only want 2 GPIOs
                break;
              }
            }
          }

          if (kk >= 2)
          {
            gpios_munged = (gpios[1] << 8) | gpios[0];
            timer_param = (void*)gpios_munged;
            switch (otb_eeprom_main_module_info_g[ii].module->module_type)
            {
              case OTB_EEPROM_MODULE_TYPE_TEMP_V0_2:
                timer_func = otb_init_ds18b20;
                break;

              case OTB_EEPROM_MODULE_TYPE_ADC_V0_1:
                timer_func = otb_init_ads;
                break;

              case OTB_EEPROM_MODULE_TYPE_RELAY_V0_2:
                timer_func = otb_relay_init_mezz;
                break;

              case OTB_EEPROM_MODULE_TYPE_MBUS_V0_1:
                timer_func = otb_serial_init_mbus_mezz;
                timer_param = (void*)otb_eeprom_main_module_info_g[ii].module->module_type;
                break;

              case OTB_EEPROM_MODULE_TYPE_LL_V0_1:
              case OTB_EEPROM_MODULE_TYPE_NIXIE_V0_2:
              default:
                DETAIL("EEPROM: No initialisation for module type: 0x%08x", otb_eeprom_main_module_info_g[ii].module->module_type);
                break;
            }
          }
          else 
          {
            WARN("EEPROM: Have mezzanine board 0x%02x but module has fewer than 2 GPIOs", otb_eeprom_main_module_info_g[ii].main_board_mod->address);
          }
        }
      }
    }
  }

EXIT_LABEL:

  if (timer_func != NULL)
  {
    os_timer_setfn((os_timer_t*)&init_timer, timer_func, timer_param);
    os_timer_arm((os_timer_t*)&init_timer, 500, 0);  
  }

  DEBUG("OTB: otb_eeprom_init_modules exit");

  return;
}

#endif // OTB_RBOOT_BOOTLOADER

//
// otb_eeprom_init
//
// Tries to connect to the eeprom at the address and bus requested
//
char ICACHE_FLASH_ATTR otb_eeprom_init(uint8_t addr, brzo_i2c_info *i2c_info)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_init entry");

  // Must have already initialized the internal I2C bus (otb_main.c or rboot.c)

  // Connect to the eeprom
  rc = otb_i2c_24xxyy_init(addr, i2c_info);
  if (rc)
  {
    DEBUG("EEPROM: Connected to eeprom at 0x%02x successfully", addr);
  }
  else
  {
    WARN("EEPROM: Failed to connect to eeprom at 0x%02x %d", addr, rc);
    goto EXIT_LABEL;
  }
  
  rc = 1;
  
EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_init exit");

  return rc;
}

//
// Reads the main components from eeprom
//
// Behaves differently when invoked from bootloader.
// Places pointers to comps in locations pointed to by otb_eeprom_main_comp_types.
//
void ICACHE_FLASH_ATTR otb_eeprom_read_all(uint8_t addr,
                                           brzo_i2c_info *i2c_info)
{
#ifndef OTB_RBOOT_BOOTLOADER // Not within the bootloader
  uint32_t types[] = {OTB_EEPROM_INFO_TYPE_INFO,
                      OTB_EEPROM_INFO_TYPE_MAIN_BOARD,
                      OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE,
                      OTB_EEPROM_INFO_TYPE_GPIO_PINS};
  uint32_t types_num = 4;
#else // OTB_RBOOT_BOOTLOADER // Within the bootloader
  uint32_t types[] = {OTB_EEPROM_INFO_TYPE_INFO,
                      OTB_EEPROM_INFO_TYPE_GPIO_PINS,
                      OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA};
  uint32_t types_num = 3;
#endif // OTB_RBOOT_BOOTLOADER
  
  DEBUG("EEPROM: otb_eeprom_read_all entry");

  otb_eeprom_read_main_types(addr,
                             i2c_info,
                             NULL,
                             types,
                             types_num);

  DEBUG("EEPROM: otb_eeprom_read_all exit");

  return;
}

void ICACHE_FLASH_ATTR otb_eeprom_process_all(void)
{
  unsigned char *prefix;

  DEBUG("EEPROM: otb_eeprom_process_all entry");

#ifndef OTB_RBOOT_BOOTLOADER
  // Note this function needs to be able to handle the information _not_
  // having been read successfully

  // Get the chip ID
  otb_util_get_chip_id();

  // Get the prefix - used for AP, and for MQTT
  prefix = OTB_MAIN_OTBIOT_PREFIX;
  if ((otb_eeprom_main_board_g != NULL) && 
      (otb_eeprom_main_board_g->common.code == OTB_EEPROM_HW_CODE_ESPI_BOARD))
  {
    prefix = OTB_MAIN_ESPI_PREFIX;
  }
  otb_mqtt_root = prefix;
  OTB_ASSERT((os_strlen(prefix) +
              os_strlen(OTB_MAIN_CHIPID)) <
             sizeof(OTB_MAIN_DEVICE_ID));
  os_sprintf(OTB_MAIN_DEVICE_ID, "%s.%s", prefix, OTB_MAIN_CHIPID);

  INFO("OTB: Hardware info: %s", otb_hw_info);

#endif // OTB_RBOOT_BOOTLOADER

  DEBUG("EEPROM: otb_eeprom_process_all exit");

  return;
}

//
// Reads a list of eeprom main comp types
//
// Array of types must start with OTB_EEPROM_INFO_TYPE_INFO, or 
// eeprom_info must be passed in.  If OTB_EEPROM_INFO_TYPE_INFO is the first
// type, and eeprom_info is non-NULL, eeprom_info is ignored (most recently
// read version if used).
void ICACHE_FLASH_ATTR otb_eeprom_read_main_types(uint8_t addr,
                                                  brzo_i2c_info *i2c_info,
                                                  otb_eeprom_info *eeprom_info,
                                                  uint32_t types[],
                                                  uint32_t types_num)
{
  otb_eeprom_info *eeprom_info_v = NULL;
  int ii, jj;

  DEBUG("EEPROM: otb_eeprom_read_main_types entry");

  OTB_ASSERT(types_num > 0);
  OTB_ASSERT((types[0] == OTB_EEPROM_INFO_TYPE_INFO) ||
            (eeprom_info != NULL));

  for (ii = 0; ii < types_num; ii++)
  {
    DEBUG("EEPROM: Reading %s", otb_eeprom_main_comp_types[types[ii]].name);

    // Support reading multiple if the type warrants it
    jj = 0;
    do
    {
      *(otb_eeprom_main_comp_types[types[ii]].global+jj) = 
                            otb_eeprom_load_main_comp(addr,
                                                      i2c_info,
                                                      eeprom_info_v,
                                                      types[ii],
                                                      jj,
                                                      NULL,
                                                      0);
      jj++;
    } while ((otb_eeprom_main_comp_types[types[ii]].max_num != OTB_EEPROM_COMP_1) &&
             (*(otb_eeprom_main_comp_types[types[ii]].global+jj-1) != NULL) &&
             ((otb_eeprom_main_comp_types[types[ii]].quantity > jj)));
    
    // Check we actually got a comp of the right type
    if (((*(otb_eeprom_main_comp_types[types[ii]].global)) == NULL) &&
        (otb_eeprom_main_comp_types[types[ii]].max_num != OTB_EEPROM_COMP_0_OR_MORE))
    {
      goto EXIT_LABEL;
    }
    
    // If we have an otb_eeprom_info store it off in eeprom_info_v for use in 
    // subsequent queries - should be first element queried
    if ((ii == 0) && (types[ii] == OTB_EEPROM_INFO_TYPE_INFO))
    {
      eeprom_info_v = *(otb_eeprom_main_comp_types[types[ii]].global);
    }
  }

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_read_main_types exit");

  return;
}

void ICACHE_FLASH_ATTR otb_eeprom_output_pin_info(uint32_t num_pins, otb_eeprom_pin_info *pin_info)
{
  int ii;

  DEBUG("EEPROM: otb_eeprom_output_pin_info entry");

  for (ii = 0; ii < num_pins; ii++)
  {
    DETAIL("EEPROM:   pin #%d:", ii);
    DETAIL("EEPROM:     num:           %d", pin_info[ii].num);
    DETAIL("EEPROM:     header_num:    %d", pin_info[ii].header_num);
    DETAIL("EEPROM:     use:           %d", pin_info[ii].use);
    DETAIL("EEPROM:     module:        0x%08x", pin_info[ii].module);
    DETAIL("EEPROM:     further_info:  0x%08x", pin_info[ii].further_info);
    DETAIL("EEPROM:     pulled:        %d", pin_info[ii].pulled);
  }
    
  DEBUG("EEPROM: otb_eeprom_output_pin_info exit");

  return;
}

//
// Read a main comp of the appropriate type.
//
// If successful either:
// - copy it into the buf (if buf_len long enough)
// - allocate a buffer and copy into that (if otb_eeprom_main_comp_types contains
//   an address to store pointer to buffer in, and if can malloc memory)
//
// Return NULL on failure, pointer to buffer info read to if successful
void *ICACHE_FLASH_ATTR otb_eeprom_load_main_comp(uint8_t addr,
                                                  brzo_i2c_info *i2c_info,
                                                  otb_eeprom_info *eeprom_info,
                                                  uint32_t type,
                                                  uint32_t num,
                                                  void *buf,
                                                  uint32_t buf_len)
{
  unsigned char *temp_buf[OTB_EEPROM_MAX_MAIN_COMP_LENGTH];
  void *local_buf = NULL;
  char *struct_name;
  uint32_t fn_rc;
  otb_eeprom_hdr *hdr;
  int ii;
  unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+2];

  DEBUG("EEPROM: otb_eeprom_load_main_comp entry");

  // Check valid type
  OTB_ASSERT(type < OTB_EEPROM_INFO_TYPE_NUM);

  // Read in the eeprom info to a variable on the stack, to get the length required
  // for the overal buffer, 
  struct_name = otb_eeprom_main_comp_types[type].name;
  fn_rc = otb_eeprom_read_main_comp(addr,
                                    i2c_info,
                                    eeprom_info,
                                    type,
                                    num,
                                    temp_buf,
                                    OTB_EEPROM_MAX_MAIN_COMP_LENGTH);

  // Can continue for now even if main comp was too long
  if ((fn_rc != OTB_EEPROM_ERR_OK) &&
      (fn_rc != (OTB_EEPROM_ERR | OTB_EEPROM_ERR_BUF_LEN_COMP)))
  {
    // Errors have already been logged
    DETAIL("EEPROM: Reading main comp %s at 0x%x returned error 0x%x", struct_name, addr, fn_rc);
    goto EXIT_LABEL;
  }

  // Log interesting (common) information
  hdr = (otb_eeprom_hdr *)temp_buf;
  DEBUG("EEPROM: %s:", struct_name, hdr);
  DEBUG("EEPROM:   hdr.magic:       0x%08x", hdr->magic);
  DEBUG("EEPROM:   hdr.type:        %d", hdr->type);
  DEBUG("EEPROM:   hdr.struct_size: 0x%08x", hdr->struct_size);
  DEBUG("EEPROM:   hdr.version:     0x%08x", hdr->version);
  DEBUG("EEPROM:   hdr.length:      0x%08x", hdr->length);
  DEBUG("EEPROM:   hdr.checksum:    0x%08x", hdr->checksum);

  if (fn_rc != OTB_EEPROM_ERR_OK)
  {
    // Now have to bail out - if main comp was too long
    goto EXIT_LABEL;
  }

  if (buf != NULL)
  {
    if (buf_len >= hdr->length)
    {
      local_buf = buf;
    }
  }
  else
  {
    // Can assert this as read_main_comp checks length not too long
    OTB_ASSERT(hdr->length <= OTB_EEPROM_MAX_MAIN_COMP_LENGTH);
#ifndef OTB_RBOOT_BOOTLOADER    
    local_buf = os_malloc(hdr->length);
#else // OTB_RBOOT_BOOTLOADER
    OTB_ASSERT((type == OTB_EEPROM_INFO_TYPE_INFO) ||
               (type == OTB_EEPROM_INFO_TYPE_GPIO_PINS) ||
               (type == OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA));
    local_buf = *(otb_eeprom_main_comp_types[type].global);
#endif // OTB_RBOOT_BOOTLOADER
    if (local_buf == NULL)
    {
      WARN("EEPROM: Failed to allocate memory for otb_eeprom_info_g %d",
           hdr->length);
      goto EXIT_LABEL;
    }
  }


  OTB_ASSERT(local_buf != NULL);
  os_memcpy(local_buf, temp_buf, hdr->length);

  hdr = (otb_eeprom_hdr *)local_buf;
  switch (type)
  {
    case OTB_EEPROM_INFO_TYPE_INFO:
      ;
      otb_eeprom_info *eeprom_info = (otb_eeprom_info *)local_buf;
      DETAIL("EEPROM:   eeprom_size:     0x%08x", eeprom_info->eeprom_size);
      DETAIL("EEPROM:   comp_num:        %d", eeprom_info->comp_num);
      DETAIL("EEPROM:   write_date:      0x%08x", eeprom_info->write_date);
      break;

    case OTB_EEPROM_INFO_TYPE_MAIN_BOARD:
      ;
      otb_eeprom_main_board *main_board = (otb_eeprom_main_board *)local_buf;
      // Guard against non NULL terminated serial!
      os_memcpy(serial, main_board->common.serial, OTB_EEPROM_HW_SERIAL_LEN+1);
      serial[OTB_EEPROM_HW_SERIAL_LEN+1] = 0;
      DETAIL("EEPROM:   common.serial:   %s", serial);
      DETAIL("EEPROM:   common.code:     0x%08x", main_board->common.code);
      DETAIL("EEPROM:   common.subcode:  0x%08x", main_board->common.subcode);
#ifndef OTB_RBOOT_BOOTLOADER
      os_sprintf(otb_hw_info,
                "%04x:%04x",
                main_board->common.code,
                main_board->common.subcode);
#endif // OTB_RBOOT_BOOTLOADER
      DETAIL("EEPROM:   chipid:          %02x%02x%02x", main_board->chipid[0], main_board->chipid[1], main_board->chipid[2]);
      DETAIL("EEPROM:   mac1:            %02x%02x%02x%02x%02x%02x",
           main_board->mac1[0],
           main_board->mac1[1],
           main_board->mac1[2],
           main_board->mac1[3],
           main_board->mac1[4],
           main_board->mac1[5]);
      DETAIL("EEPROM:   mac2:            %02x%02x%02x%02x%02x%02x",
           main_board->mac2[0],
           main_board->mac2[1],
           main_board->mac2[2],
           main_board->mac2[3],
           main_board->mac2[4],
           main_board->mac2[5]);
      DETAIL("EEPROM:   esp_module:      %d", main_board->esp_module);
      DETAIL("EEPROM:   flash_size:      0x%08x bytes", main_board->flash_size_bytes);
      DETAIL("EEPROM:   i2c_adc:         %d", main_board->i2c_adc);
      DETAIL("EEPROM:   internal_adc_type: %d", main_board->internal_adc_type);
      DETAIL("EEPROM:   num_modules:     %d", main_board->num_modules);
      break;

    case OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE:
      ;
      otb_eeprom_main_board_module *module = (otb_eeprom_main_board_module *)local_buf;
      DETAIL("EEPROM:   num:             %d", module->num);
      DETAIL("EEPROM:   socket_type:     %d", module->socket_type);
      DETAIL("EEPROM:   num_headers:     %d", module->num_headers);
      DETAIL("EEPROM:   num_pins:        %d", module->num_pins);
      DETAIL("EEPROM:   address:         0x%02x", module->address);
#ifdef OTB_DEBUG      
      otb_eeprom_output_pin_info(module->num_pins, module->pin_info);
#endif // OTB_DEBUG      
      break;

    case OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA:
      ;
      otb_eeprom_main_board_sdk_init_data *sdk_init_data = (otb_eeprom_main_board_sdk_init_data *)local_buf;
      DETAIL("EEPROM:   data_len:        %d bytes", sdk_init_data->data_len);
      break;

    case OTB_EEPROM_INFO_TYPE_GPIO_PINS:
      ;
      otb_eeprom_main_board_gpio_pins *gpio_pins = (otb_eeprom_main_board_gpio_pins *)local_buf;
      DETAIL("EEPROM:   num_pins:        %d", gpio_pins->num_pins);
#ifdef OTB_DEBUG      
      otb_eeprom_output_pin_info(gpio_pins->num_pins, gpio_pins->pin_info);
#endif // OTB_DEBUG      
      break;

    case OTB_EEPROM_INFO_TYPE_MAIN_MODULE:
      ;
      otb_eeprom_main_module *mod = (otb_eeprom_main_module *)local_buf;
      // Guard against non NULL terminated serial!
      os_memcpy(serial, mod->common.serial, OTB_EEPROM_HW_SERIAL_LEN+1);
      serial[OTB_EEPROM_HW_SERIAL_LEN+1] = 0;
      DETAIL("EEPROM:   common.serial:   %s", serial);
      DETAIL("EEPROM:   common.code:     0x%08x", mod->common.code);
      DETAIL("EEPROM:   common.subcode:  0x%08x", mod->common.subcode);
      DETAIL("EEPROM:   module_type:     0x%08x", mod->module_type);
      DETAIL("EEPROM:   socket_type:     0x%08x", mod->socket_type);
      DETAIL("EEPROM:   jack_used:       0x%08x", mod->jack_used);
      break;

    default:
      WARN("EEPROM: Unknown type - can't crack out further");
      break;
  }

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_load_main_comp exit");

  return (local_buf);
  
}
//
// otb_eeprom_find_main_comp
//
// Given an otb_eeprom_info structure, finds other components.
// If otb_eeprom_info is NULL can only "find" OTB_EEPROM_INFO_TYPE_INFO (at 0x0)
//
// Use num to indicate which instance should be found (0, 1, ...) - ignored if
// max_num is OTB_EEPROM_COMP_1
// 
//

bool ICACHE_FLASH_ATTR otb_eeprom_find_main_comp(otb_eeprom_info *eeprom_info,
                                                 uint32_t type, 
                                                 uint32_t num,
                                                 uint32_t *loc,
                                                 uint32_t *length)
{
  int ii;
  bool found = FALSE;
  int jj = 0;

  DEBUG("EEPROM: otb_eeprom_find_comp entry");

  // Unless reading otb_eeprom_info, otb_eeprom_info must be passed in
  OTB_ASSERT((eeprom_info != NULL) ||
             (type == OTB_EEPROM_INFO_TYPE_INFO));

  OTB_ASSERT(loc != NULL);

  if (type == OTB_EEPROM_INFO_TYPE_INFO)
  {
    // Note length will be larger than this - but need to read the struct off the
    // eeprom to know the exact length
    *loc = 0;
    *length = sizeof(otb_eeprom_info);
    DEBUG("EEPROM: \"Found\" comp type %d at 0x%x length 0x%x", type, loc, len);
    found = TRUE;
    goto EXIT_LABEL;
  }

  // Find the component
  for (ii = 0; ii < eeprom_info->comp_num; ii++)
  {
    if (eeprom_info->comp[ii].type == type)
    {
      *loc = eeprom_info->comp[ii].location;
      *length = eeprom_info->comp[ii].length;
      DEBUG("EEPROM: Found comp type %d at 0x%x length 0x%x", type, loc, len);
      jj++;
      if ((otb_eeprom_main_comp_types[type].max_num == OTB_EEPROM_COMP_1) ||
          (num < jj))
      {
        found = TRUE;
        break;
      }
    }
  }

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_find_comp exit");

  return found;
}

//
// otb_eeprom_process_hdr
//
// Process the hdr struct in an main comp struct
// Returns FALSE if any errors, rc clarifies these
//
// If checksum is TRUE, JUST checks checksum otherwise doesn't check it
//
bool ICACHE_FLASH_ATTR otb_eeprom_process_hdr(otb_eeprom_hdr *hdr,
                                              uint32_t type,
                                              uint32_t buf_len,
                                              uint32_t *rc,
                                              bool checksum)
{
  bool fn_rc = TRUE;
  uint32_t magic;
  uint32_t version_min;
  uint32_t version_max;
  uint32_t struct_size_min;
  uint32_t struct_size_max;
  unsigned char *struct_type;

  DEBUG("EEPROM: otb_eeprom_process_hdr entry");

  OTB_ASSERT(hdr != NULL);
  OTB_ASSERT(type < OTB_EEPROM_INFO_TYPE_NUM);
  OTB_ASSERT(buf_len >= sizeof(otb_eeprom_main_comp_types[type].struct_size_min));

  struct_type = otb_eeprom_main_comp_types[type].name;
  magic = otb_eeprom_main_comp_types[type].magic;
  version_min = otb_eeprom_main_comp_types[type].version_min;
  version_max = otb_eeprom_main_comp_types[type].version_max;
  struct_size_min = otb_eeprom_main_comp_types[type].struct_size_min;
  struct_size_max = otb_eeprom_main_comp_types[type].struct_size_max;

  if (!checksum)
  {
    if (hdr->magic != magic)
    {
      // Can keep going
      WARN("EEPROM: Bad magic number %s 0x%08x vs 0x%08x", struct_type, hdr->magic, magic);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_MAGIC;
    }

    if (hdr->type != type)
    {
      // Can keep going
      WARN("EEPROM: Bad type %s %d vs %d", struct_type, hdr->type, type);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_TYPE;
    }

    if ((hdr->version < version_min) ||
        (hdr->version > version_max))
    {
      // Can keep going
      WARN("EEPROM: Bad version %s %d vs %d/%d",
          struct_type,
          hdr->version,
          version_min,
          version_max);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_VERSION;
    }

    // Should also test not smaller than size of smallest version
    if ((hdr->struct_size < struct_size_min) || (hdr->struct_size > struct_size_max))
    {
      // Can keep going
      WARN("EEPROM: Bad struct_size %s %d vs %d/%d",
          struct_type,
          hdr->struct_size,
          struct_size_min,
          struct_size_max);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_MAGIC;
    }

    if (hdr->length > buf_len)
    {
      // We can't check the checksum as we don't have all the data
      WARN("EEPROM: Buffer not large enough %d vs %d", buf_len, hdr->length);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_BUF_LEN_COMP;
      goto EXIT_LABEL;
    }
  }
  else
  {
    fn_rc = otb_eeprom_check_checksum((char*)hdr,
                                      hdr->length,
                                      (char*)&(hdr->checksum) - (char*)hdr,
                                      sizeof(hdr->checksum));
    if (!fn_rc)
    {
      // Can keep going
      WARN("EEPROM: Bad checksum %s 0x%08x", struct_type, hdr->checksum);
      fn_rc = FALSE;
      *rc |= OTB_EEPROM_ERR_CHECKSUM;
    }                 
  }

EXIT_LABEL:                     

  DEBUG("EEPROM: otb_eeprom_process_hdr exit");

  return fn_rc;
}

//
// otb_eeprom_read_main_comp
//
// Reads a main component from the eeprom, one of OTB_EEPROM_INFO_TYPE_...
// Checks the header values are sensible
//
uint32_t ICACHE_FLASH_ATTR otb_eeprom_read_main_comp(uint8_t addr,
                                                     brzo_i2c_info *i2c_info,
                                                     otb_eeprom_info *eeprom_info,
                                                     uint32_t type,
                                                     uint32_t num,
                                                     void *read_buf,
                                                     uint32_t buf_len)
{
  uint32_t rc = OTB_EEPROM_ERR;
  int eeprom_info_len;
  uint32_t loc = 0, len = 0;
  uint32_t read_len, new_read_len;
  bool partial_read = FALSE;
  bool fn_rc;
  unsigned char *struct_type;
  otb_eeprom_hdr *buf = (otb_eeprom_hdr *)read_buf;

  DEBUG("EEPROM: otb_eeprom_read_main_comp entry");

  OTB_ASSERT(type < OTB_EEPROM_INFO_TYPE_NUM);
  OTB_ASSERT(buf_len >= sizeof(otb_eeprom_main_comp_types[type].struct_size_min));

  struct_type = otb_eeprom_main_comp_types[type].name;

  //
  // First of all read in just the struct - will read in the read shortly
  //

  fn_rc = otb_eeprom_find_main_comp(eeprom_info, type, num, &loc, &len);
  if (!fn_rc)
  {
    // Couldn't find any record of this structure on the eeprom - exit
    // Note this may be expected - if we are reading multiple instances
    // we'll hit this after we've read the last
    DETAIL("EEPROM: %s not found on eeprom", struct_type);
    rc |= OTB_EEPROM_ERR_NOT_FOUND;
    goto EXIT_LABEL;
  }

  if (len > buf_len)
  {
    // Not enough space for everything but (based on assert at beginning) enough for
    // main struct, so carry on
    WARN("EEPROM: Buffer not large enough to read %s: %d vs %d",
         struct_type,
         len,
         buf_len);
    rc |= OTB_EEPROM_ERR_BUF_LEN_COMP;
    read_len = buf_len;
    partial_read = TRUE;
  }
  else
  {
    // Don't read more data than is on the eeprom
    read_len = len;
  }

  fn_rc = otb_i2c_24xx128_read_data(addr,
                                    loc,
                                    read_len,
                                    (uint8_t *)buf,
                                    i2c_info);
  if (!fn_rc)
  {
    // Error reading or reading to I2C bus
    WARN("EEPROM: Failed to read data from I2C bus %s", struct_type);
    rc |= OTB_EEPROM_ERR_I2C;
    goto EXIT_LABEL;
  }

  // Update loc (may be used later)
  loc += read_len;

  //
  // Deal with header information - can't check checksum (as haven't read
  // entirety of data)
  //
  fn_rc = otb_eeprom_process_hdr(buf,
                                 type,
                                 buf_len,
                                 &rc,
                                 FALSE);
  if (!fn_rc)
  {
    // That's all we can do if we hit an error - we don't want to continue.  Errors
    // are logged in otb_eeprom_process_hdr.
    WARN("EEPROM: Hit error processing header");
    partial_read = TRUE;
    goto EXIT_LABEL;
  }

  //
  // We now need to read any extra data
  //
  if (buf->length > read_len)
  {
    // Check we can
    if (buf->length > buf_len)
    {
      rc |= OTB_EEPROM_ERR_BUF_LEN_COMP;
      goto EXIT_LABEL;
    }

    // Optimise to avoid rereading what we already read
    new_read_len = buf->length - read_len;
    // Must read into a 4 byte aligned boundary
    OTB_ASSERT(((uint32_t)(((uint8_t *)buf)+read_len) % 4) == 0);
    fn_rc = otb_i2c_24xx128_read_data(addr,
                                      loc,
                                      new_read_len,
                                      ((uint8_t *)buf)+read_len,
                                      i2c_info);
    if (!fn_rc)
    {
      // Error reading or reading to I2C bus
      // Arguably could return something more helpful as did read some informatio
      // but sadly nothing terribly useful - as the interesting stuff are the info
      // comps.  Going to treat the whole eeprom as bad.
      WARN("EEPROM: Failed to read data from I2C bus %s", struct_type);
      rc |= OTB_EEPROM_ERR_I2C;
      goto EXIT_LABEL;
    }
  }

  //
  // Now reprocess header (including checksum)
  //
  fn_rc = otb_eeprom_process_hdr(buf,
                                 type,
                                 buf_len,
                                 &rc,
                                 TRUE);

  //
  // No additional components to read so stop here
  //
  if (rc == OTB_EEPROM_ERR)
  {
    // Haven't hit any errors so set rc to OK
    rc = OTB_EEPROM_ERR_OK; 
  }

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_read_main_comp exit");

  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_check_checksum(char *data, int size, int checksum_loc, int checksum_size)
{
  char rc = 0;
  int ii;
  uint32 *checksum;
  uint32 calc_check;
  
  DEBUG("EEPROM: otb_eeprom_check_checksum entry");

  DEBUG("EEPROM: Size: %d", size);
  DEBUG("EEPROM: Checksum loc: %d", checksum_loc);
  DEBUG("EEPROM: Checksum size: %d", checksum_size);
  
  OTB_ASSERT(checksum_size == sizeof(uint32));
  
  checksum = (uint32*)(data + checksum_loc);
  DEBUG("EEPROM: Stored checksum: 0x%08x", *checksum);
  
  calc_check = OTB_EEPROM_CHECKSUM_INITIAL;
  for (ii = 0; ii < size; ii++)
  {
    if ((ii < checksum_loc) || (ii >= (checksum_loc + checksum_size)))
    {
      DEBUG("EEPROM: Checksum byte: 0x%02x, %d", *(data+ii), (ii%4));
      calc_check += (*(data+ii) << ((ii%4) * 8));
    }
  }  
  
  DEBUG("EEPROM: Calculated checksum: 0x%08x", calc_check);

  rc = (*checksum == calc_check);
  
  DEBUG("EEPROM: otb_eeprom_check_checksum exit");
  
  return rc;
}

#ifndef OTB_RBOOT_BOOTLOADER
bool ICACHE_FLASH_ATTR otb_eeprom_rpi_hat_get(unsigned char *next_cmd, void *arg, unsigned char *prev_cmd)
{
  bool rc = FALSE;
  uint32_t info = (uint32_t)arg;
  
  DEBUG("EEPROM: otb_eeprom_rpi_hat_get entry");

  if (otb_eeprom_rpi_hat_info_g == NULL)
  {
    otb_cmd_rsp_append("no hat installed");
    goto EXIT_LABEL;
  }
  
  rc = TRUE;
  switch (info)
  {
    case OTB_EEPROM_RPI_HAT_INFO_UUID:
      otb_cmd_rsp_append(otb_eeprom_rpi_hat_info_g->uuid);
      break;

    case OTB_EEPROM_RPI_HAT_INFO_PID:
      otb_cmd_rsp_append("0x%04x", otb_eeprom_rpi_hat_info_g->pid);
      break;

    case OTB_EEPROM_RPI_HAT_INFO_PVER:
      otb_cmd_rsp_append("0x%04x", otb_eeprom_rpi_hat_info_g->pver);
      break;

    case OTB_EEPROM_RPI_HAT_INFO_PRODUCT:
      otb_cmd_rsp_append(otb_eeprom_rpi_hat_info_g->pstr);
      break;

    case OTB_EEPROM_RPI_HAT_INFO_VENDOR:
      otb_cmd_rsp_append(otb_eeprom_rpi_hat_info_g->vstr);
      break;

    default:
      rc = FALSE;
      break;

  }
  
EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_rpi_hat_get exit");
  
  return rc;

}
#endif // OTB_RBOOT_BOOTLOADER
