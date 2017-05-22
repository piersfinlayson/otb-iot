/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016-2017 Piers Finlayson
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

  DEBUG("EEPROM: otb_eeprom_read entry");

  fn_rc = otb_eeprom_init(otb_eeprom_main_board_addr, &otb_i2c_bus_internal);
  if (!fn_rc)
  {
    // Failed - bail
    goto EXIT_LABEL;
  }

  otb_eeprom_read_all(otb_eeprom_main_board_addr, &otb_i2c_bus_internal);

EXIT_LABEL:

  // Now do something with the info read from the eeprom
  otb_eeprom_process_all();

  DEBUG("EEPROM: otb_eeprom_read exit");

  return;
}

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
#ifndef OTB_RBOOT_BOOTLOADER
  uint32_t types[] = {OTB_EEPROM_INFO_TYPE_INFO,
                      OTB_EEPROM_INFO_TYPE_MAIN_BOARD,
                      OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE,
                      OTB_EEPROM_INFO_TYPE_GPIO_PINS};
  uint32_t types_num = 4;
#else // OTB_RBOOT_BOOTLOADER
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

  DEBUG("EEPROM: otb_eeprom_process_all entry");

#ifndef OTB_RBOOT_BOOTLOADER
  // Note this function needs to be able to handle the information _not_
  // having been read successfully

  // Get the chip ID
  otb_util_get_chip_id();
  OTB_ASSERT((os_strlen(OTB_MAIN_OTBIOT_PREFIX) +
              os_strlen(OTB_MAIN_CHIPID)) <
             sizeof(OTB_MAIN_DEVICE_ID));
  os_sprintf(OTB_MAIN_DEVICE_ID, "%s.%s", OTB_MAIN_OTBIOT_PREFIX, OTB_MAIN_CHIPID);

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
    INFO("EEPROM: Reading %s", otb_eeprom_main_comp_types[types[ii]].name);

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
    
    // Check we actually got a comp or the right type
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
    INFO("EEPROM:   pin #%d:", ii);
    INFO("EEPROM:     num:           %d", pin_info[ii].num);
    INFO("EEPROM:     header_num:    %d", pin_info[ii].header_num);
    INFO("EEPROM:     use:           %d", pin_info[ii].use);
    INFO("EEPROM:     module:        0x%08x", pin_info[ii].module);
    INFO("EEPROM:     further_info:  0x%08x", pin_info[ii].further_info);
    INFO("EEPROM:     pulled:        %d", pin_info[ii].pulled);
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

  DEBUG("EEPROM: otb_eeprom_read_all_otbiot entry");

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
    WARN("EEPROM: Reading main comp %s returned error 0x%x", struct_name, fn_rc);
    goto EXIT_LABEL;
  }

  // Log interesting (common) information
  hdr = (otb_eeprom_hdr *)temp_buf;
  INFO("EEPROM: %s:", struct_name, hdr);
  INFO("EEPROM:   hdr.magic:       0x%08x", hdr->magic);
  INFO("EEPROM:   hdr.struct_size: 0x%08x", hdr->struct_size);
  INFO("EEPROM:   hdr.version:     0x%08x", hdr->version);
  INFO("EEPROM:   hdr.length:      0x%08x", hdr->length);
  INFO("EEPROM:   hdr.checksum:    0x%08x", hdr->checksum);

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
      INFO("EEPROM:   eeprom_size:     0x%08x", eeprom_info->eeprom_size);
      INFO("EEPROM:   comp_num:        %d", eeprom_info->comp_num);
      INFO("EEPROM:   write_date:      0x%08x", eeprom_info->write_date);
      break;

    case OTB_EEPROM_INFO_TYPE_MAIN_BOARD:
      ;
      otb_eeprom_main_board *main_board = (otb_eeprom_main_board *)local_buf;
      // Guard against non NULL terminated serial!
      unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+2];
      os_memcpy(serial, main_board->common.serial, OTB_EEPROM_HW_SERIAL_LEN+1);
      serial[OTB_EEPROM_HW_SERIAL_LEN+1] = 0;
      INFO("EEPROM:   common.serial:   %s", main_board->common.serial);
      INFO("EEPROM:   common.code:     0x%08x", main_board->common.code);
      INFO("EEPROM:   common.subcode:  0x%08x", main_board->common.subcode);
      INFO("EEPROM:   chipid:          %02x%02x%02x", main_board->chipid[0], main_board->chipid[1], main_board->chipid[2]);
      INFO("EEPROM:   mac1:            %02x%02x%02x%02x%02x%02x",
           main_board->mac1[0],
           main_board->mac1[1],
           main_board->mac1[2],
           main_board->mac1[3],
           main_board->mac1[4],
           main_board->mac1[5]);
      INFO("EEPROM:   mac2:            %02x%02x%02x%02x%02x%02x",
           main_board->mac2[0],
           main_board->mac2[1],
           main_board->mac2[2],
           main_board->mac2[3],
           main_board->mac2[4],
           main_board->mac2[5]);
      INFO("EEPROM:   esp_module:      %d", main_board->esp_module);
      INFO("EEPROM:   flash_size:      0x%08x bytes", main_board->flash_size_bytes);
      INFO("EEPROM:   i2c_adc:         %d", main_board->i2c_adc);
      INFO("EEPROM:   internal_adc_type: %d", main_board->internal_adc_type);
      INFO("EEPROM:   num_modules:     %d", main_board->num_modules);
      break;

    case OTB_EEPROM_INFO_TYPE_MAIN_BOARD_MODULE:
      ;
      otb_eeprom_main_board_module *module = (otb_eeprom_main_board_module *)local_buf;
      INFO("EEPROM:   num:             %d", module->num);
      INFO("EEPROM:   socket_type:     %d", module->socket_type);
      INFO("EEPROM:   num_headers:     %d", module->num_headers);
      INFO("EEPROM:   num_pins:        %d", module->num_pins);
      INFO("EEPROM:   address:         0x%02x", module->address);
//#ifdef OTB_DEBUG      
      otb_eeprom_output_pin_info(module->num_pins, module->pin_info);
//#endif // OTB_DEBUG      
      break;

    case OTB_EEPROM_INFO_TYPE_SDK_INIT_DATA:
      ;
      otb_eeprom_main_board_sdk_init_data *sdk_init_data = (otb_eeprom_main_board_sdk_init_data *)local_buf;
      INFO("EEPROM:   data_len:        %d bytes", sdk_init_data->data_len);
      break;

    case OTB_EEPROM_INFO_TYPE_GPIO_PINS:
      ;
      otb_eeprom_main_board_gpio_pins *gpio_pins = (otb_eeprom_main_board_gpio_pins *)local_buf;
      INFO("EEPROM:   num_pins:        %d", gpio_pins->num_pins);
//#ifdef OTB_DEBUG      
      otb_eeprom_output_pin_info(gpio_pins->num_pins, gpio_pins->pin_info);
//#endif // OTB_DEBUG      
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
    WARN("EEPROM: %s not found on eeprom", struct_type);
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

#if 0

TESTING
- Quick way to test would be a standalone app running the appropriate bits of code

Defensive code for checking info comps!

  for (ii = 0; ii < eeprom_info->comp_num; ii)
  {
    info_comp = eeprom_info->comp[ii];
    next_info_comp = eeprom_info->comp[ii+1];
    if ((next_info_comp - eeprom_info) < 
  }

Check info_comp locs are all in eeprom itself given its size

Need to build functions to log each of main info comps and link to from otb_eeprom_main_comp_types

  // Now we have loaded the entire otb_eeprom_info from eeprom we can log the info
  // comp locations.
  for (ii = 0;  ii < OTB_MAX(OTB_EEPROM_INFO_MAX_COMP, otb_eeprom_info_g.comp_num), ii++)
  {
    INFO("EEPROM:   otb_info_comp %d:", ii);
    INFO("EEPROM:     type = %d", otb_eeprom_info_g.comp[ii].type);
    INFO("EEPROM:     type = %d", otb_eeprom_info_g.comp[ii].location);
    INFO("EEPROM:     type = %d", otb_eeprom_info_g.comp[ii].length);
  }
  if (otb_eeprom_info_g.comp_num > OTB_EEPROM_INFO_MAX_COMP)
  {
    WARN("EEPROM: otb_eeprom_info.comp_num is invalid: %d vs %d",
         otb_eeprom_info_g.comp_num,
         OTB_EEPROM_INFO_MAX_COMP);
  }

char ICACHE_FLASH_ATTR otb_eeprom_read_sdk_init_data(otb_eeprom_glob_conf *glob_conf, unsigned char *buf, uint32 buf_len)
{
  char rc = 0;
  uint32 sdk_end;
  otb_eeprom_sdk_init_data *sdk;
  int ii;

  DEBUG("EEPROM: otb_eeprom_read_sdk_init_data entry");

  // Check we can safely access the sdk init data
  sdk_end = glob_conf->loc_sdk_init_data + glob_conf->loc_sdk_init_data_len;
  if ((sdk_end > OTB_EEPROM_SIZE_128) || (sdk_end > otb_eeprom_size))
  {
    WARN("EEPROM: Invalid SDK init data location on eeprom: %u %u", glob_conf->loc_sdk_init_data, glob_conf->loc_sdk_init_data_len);
    goto EXIT_LABEL;
  }

  if (glob_conf->loc_sdk_init_data_len > buf_len)
  {
    WARN("EEPROM: Buffer not big enough for sdk data: %u %u:", glob_conf->loc_sdk_init_data_len, buf_len);
    goto EXIT_LABEL;
  }
  
  // Read the data
  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, glob_conf->loc_sdk_init_data, glob_conf->loc_sdk_init_data_len, (uint8_t *)buf, &otb_i2c_bus_internal);
if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (sdk init data) from 0x%x", glob_conf->loc_sdk_init_data_len, glob_conf->loc_sdk_init_data);
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (sdk init data) %d", glob_conf->loc_sdk_init_data_len, rc);
    goto EXIT_LABEL;
  }

  sdk = (otb_eeprom_sdk_init_data *)buf;
  // Check magic number - can't continue if this is wrong
  if (sdk->hdr.magic != OTB_EEPROM_SDK_INIT_DATA_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in sdk init data: 0x%08x", sdk->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }

  INFO("EEPROM: SDK Init data format:   V%d", sdk->hdr.version);
  INFO("EEPROM: SDK Init data checksum: 0x%08x", sdk->hdr.checksum);

  rc = otb_eeprom_check_checksum((char*)sdk,
                                 glob_conf->loc_sdk_init_data_len,
                                 (char*)&(sdk->hdr.checksum) - (char*)sdk - 1,
                                 sizeof(sdk->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: SDK Init data checksum: Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: SDK Init data checksum: Valid");
  }

  // We've loaded the SDK init data now - the actual data should succeed this
  // structure. Finally check len is loc_sdk_init_data_len - the structure
  if (sdk->hdr.struct_size != glob_conf->loc_sdk_init_data_len)
  {
    WARN("EEPROM: Invalid SDK Init data len: %u %u", sdk->hdr.struct_size, glob_conf->loc_sdk_init_data_len);
    goto EXIT_LABEL;
  }

  // Done.  Copy the data to the start of buf.  Better do this a byte at a time
  // as memcpy doesn't work on overlapping ranges, and I can't remember if
  // memmove might try and allocate memory.
  for (ii = 0; ii < (glob_conf->loc_sdk_init_data_len); ii++)
  {
    buf[ii] = buf[sizeof(*sdk) + ii];
  } 
  rc = 1;

EXIT_LABEL:

  DEBUG("EEPROM: otb_eeprom_read_sdk_init_data exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_read_glob_conf(otb_eeprom_glob_conf *glob_conf)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_read_glob_conf entry");
  
  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, OTB_EEPROM_GLOB_CONFIG_LOC, sizeof(otb_eeprom_glob_conf), (uint8_t *)glob_conf, &otb_i2c_bus_internal);
  if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (glob conf)", sizeof(otb_eeprom_glob_conf));
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (glob conf) %d", sizeof(otb_eeprom_glob_conf), rc);
    goto EXIT_LABEL;
  }
  
  // Check magic number - can't continue if this is wrong
  if (glob_conf->hdr.magic != OTB_EEPROM_GLOB_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in glob conf: 0x%08x", glob_conf->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }
  
  INFO("EEPROM: Eeprom size:            %d bytes", glob_conf->eeprom_size);
  INFO("EEPROM: Global info format:     V%d", glob_conf->hdr.version);
  INFO("EEPROM: Global checksum:        0x%08x", glob_conf->hdr.checksum);
  rc = otb_eeprom_check_checksum((char*)glob_conf,
                                 sizeof(*glob_conf),
                                 (char*)&(glob_conf->hdr.checksum) - (char*)glob_conf,
                                 sizeof(glob_conf->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: Global checksum:        Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: Global checksum:        Valid");
  }
  
  otb_eeprom_size = glob_conf->eeprom_size;
  
EXIT_LABEL:  

  if (!rc)
  {
    os_memset(glob_conf, 0, sizeof(*glob_conf));
  }
  
  DEBUG("EEPROM: otb_eeprom_read_glob_conf exit");
  
  return rc;
}

char ICACHE_FLASH_ATTR otb_eeprom_read_hw_conf(otb_eeprom_glob_conf *glob_conf, otb_eeprom_hw_conf *hw_conf)
{
  char rc = 0;
  
  DEBUG("EEPROM: otb_eeprom_read_hw_conf entry");
  
  uint32 hw_end;
  unsigned char serial[OTB_EEPROM_HW_SERIAL_LEN+2];
  
  // Check we can actually safely access the hardware information location
  hw_end = glob_conf->loc_hw_struct + glob_conf->loc_hw_struct_len;
  if ((hw_end > OTB_EEPROM_SIZE_128) || (hw_end > otb_eeprom_size))
  {
    // Invalid hardware info location
    WARN("EEPROM: Invalid hardware info location on eeprom: %u %u", glob_conf->loc_hw_struct, glob_conf->loc_hw_struct_len);
    goto EXIT_LABEL;
  }
  
  if (glob_conf->loc_hw_struct_len < sizeof(otb_eeprom_hw_conf))
  {
    // Invalid hardware info length
    WARN("EEPROM: Invalid hardware info location length on eeprom: %u %u", glob_conf->loc_hw_struct, glob_conf->loc_hw_struct_len);
    goto EXIT_LABEL;
  }

  rc = otb_i2c_24xx128_read_data(otb_eeprom_addr, glob_conf->loc_hw_struct, sizeof(otb_eeprom_hw_conf), (uint8_t *)hw_conf, &otb_i2c_bus_internal);
  if (rc)
  {
    DEBUG("EEPROM: Read %d bytes from eeprom successfully (hw structure)", sizeof(otb_eeprom_hw_conf));
  }
  else
  {
    WARN("EEPROM: Failed to read %d bytes from eeprom (hw structure) %d", sizeof(otb_eeprom_hw_conf), rc);
    goto EXIT_LABEL;
  }
  
  // Check magic number - can't continue if this is wrong
  if (hw_conf->hdr.magic != OTB_EEPROM_HW_MAGIC)
  {
    WARN("EEPROM: Invalid magic value in hw conf: 0x%08x", hw_conf->hdr.magic);
    rc = 0;
    goto EXIT_LABEL;
  }
  
  INFO("EEPROM: Hardware info format:   V%d", hw_conf->hdr.version);
  INFO("EEPROM: Hardware checksum:      0x%08x", hw_conf->hdr.checksum);

  rc = otb_eeprom_check_checksum((char*)hw_conf,
                                 sizeof(*hw_conf),
                                 (char*)&(hw_conf->hdr.checksum) - (char*)hw_conf,
                                 sizeof(hw_conf->hdr.checksum));
  if (!rc)
  {
    WARN("EEPROM: Hardware checksum:      Invalid");
    goto EXIT_LABEL;
  }
  else
  {
    INFO("EEPROM: Hardware checksum:      Valid");
  }

  // We copy serial number out in order to ensure it's NULL terminated
  os_memcpy(serial, hw_conf->serial, OTB_EEPROM_HW_SERIAL_LEN+1);
  serial[OTB_EEPROM_HW_SERIAL_LEN+1] = 0;
  // Also null terminate within the struct just in case ...
  hw_conf->serial[OTB_EEPROM_HW_SERIAL_LEN] = 0;
  INFO("EEPROM: Device serial:          %s", serial);
  INFO("EEPROM: Hardware code/sub code: %08x:%08x", hw_conf->code, hw_conf->subcode);
  INFO("EEPROM: Chip ID:                %02x%02x%02x", hw_conf->chipid[0], hw_conf->chipid[1], hw_conf->chipid[2]);
  INFO("EEPROM: MAC 1:                  %02x:%02x:%02x:%02x:%02x:%02x", hw_conf->mac1[0], hw_conf->mac1[1], hw_conf->mac1[2], hw_conf->mac1[3], hw_conf->mac1[4], hw_conf->mac1[5]);
  INFO("EEPROM: MAC 2:                  %02x:%02x:%02x:%02x:%02x:%02x", hw_conf->mac2[0], hw_conf->mac2[1], hw_conf->mac2[2], hw_conf->mac2[3], hw_conf->mac2[4], hw_conf->mac2[5]);
  INFO("EEPROM: ESP module type:        %d", hw_conf->esp_module);
  INFO("EEPROM: Flash Size:             %d bytes", hw_conf->flash_size_bytes);
  INFO("EEPROM: ADC Type(s):            %d", hw_conf->i2c_adc);
  INFO("EEPROM: ADC Config(s):          %d", hw_conf->internal_adc_type);
  INFO("EEPROM: Internal SDA pin:       %d", hw_conf->i2c_int_sda_pin);
  INFO("EEPROM: Internal SCL pin:       %d", hw_conf->i2c_int_scl_pin);
  INFO("EEPROM: External SDA pin:       %d", hw_conf->i2c_ext_sda_pin);
  INFO("EEPROM: External SCL pin:       %d", hw_conf->i2c_ext_scl_pin);
    
  rc = 1;
  
EXIT_LABEL:  

  if (!rc)
  {
    os_memset(hw_conf, 0, sizeof(*hw_conf));
  }
  
  DEBUG("EEPROM: otb_eeprom_read_hw_conf exit");
  
  return rc;
}

#endif


