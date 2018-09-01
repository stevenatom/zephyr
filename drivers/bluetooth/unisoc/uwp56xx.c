#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include "uwp56xx.h"
#include "uki_utlis.h"

#include "host/hci_core.h"

#include <fs.h>
#include <ff.h>
#include <bluetooth/crypto.h>

#define BT_CONFIG_PSKEY_FILE "/NAND:/BT_CON~1.INI"
#define BT_CONFIG_RF_FILE "/NAND:/BT_CON~2.INI"

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t#"
#define CONF_VALUES_PARTITION " ,=\n\r\t#"
#define CONF_MAX_LINE_LEN 255

#define FATFS_MNTP "/NAND:"
/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
    .type = FS_FATFS,
    .mnt_point = FATFS_MNTP,
    .fs_data = &fat_fs,
};

// pskey file structure default value
static pskey_config_t marlin3_pskey = {
    .device_class = 0x001F00,
    .feature_set = {0xFF, 0xFF, 0x8D, 0xFE, 0xDB, 0x7D, 0x7B, 0x83, 0xFF, 0xA7, 0xFF, 0x7F, 0x00, 0xE0, 0xF7, 0x3E},
    .device_addr = {0x88, 0x88, 0x66, 0xDA, 0x45, 0x40},
    .comp_id = 0x01EC,
    .g_sys_uart0_communication_supported = 1,
    .cp2_log_mode = 1,
    .LogLevel = 0xFF,
    .g_central_or_perpheral = 0,
    .Log_BitMask = 0xFFFF,
    .super_ssp_enable = 0,
    .common_rfu_b3 = 0,
    .common_rfu_w = {0x00000000, 0x00000000},
    .le_rfu_w = {0x00000000, 0x000000000},
    .lmp_rfu_w = {0x00000000, 0x000000000},
    .lc_rfu_w = {0x00000000, 0x000000000},
    .g_wbs_nv_117 = 0x004D,
    .g_wbs_nv_118 = 0x0076,
    .g_nbv_nv_117 = 0x009B,
    .g_nbv_nv_118 = 0x0A55,
    .g_sys_sco_transmit_mode = 0,
    .audio_rfu_b1 = 0,
    .audio_rfu_b2 = 0,
    .audio_rfu_b3 = 0,
    .audio_rfu_w = {0x00000000, 0x00000000},
    .g_sys_sleep_in_standby_supported = 1,
    .g_sys_sleep_master_supported = 1,
    .g_sys_sleep_slave_supported = 1,
    .power_rfu_b1 = 0,
    .power_rfu_w = {0x00000000, 0x00000000},
    .win_ext = 40,
    .edr_tx_edr_delay = 6,
    .edr_rx_edr_delay = 8,
    .tx_delay = 12,
    .rx_delay = 34,
    .bb_rfu_w = {0x00000000, 0x00000000},
    .agc_mode = 0,
    .diff_or_eq = 0,
    .ramp_mode = 0,
    .modem_rfu_b1 = 0,
    .modem_rfu_w = {0x00000000, 0x00000000},
    .BQB_BitMask_1 = 0x00000000,
    .BQB_BitMask_2 = 0x00000000,
    .bt_coex_threshold = {0x04E2, 0x1F40, 0x0020, 0x00C8, 0x0006, 0x0000, 0x0000, 0x0000},
    .other_rfu_w = {0x00000000, 0x00000000},
};

static const conf_entry_t marlin3_pksey_table[] = {
    CONF_ITEM_TABLE(device_class, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(feature_set, 0, marlin3_pskey, 16),
    CONF_ITEM_TABLE(device_addr, 0, marlin3_pskey, 6),
    CONF_ITEM_TABLE(comp_id, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_sys_uart0_communication_supported, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(cp2_log_mode, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(LogLevel, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_central_or_perpheral, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(Log_BitMask, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(super_ssp_enable, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(common_rfu_b3, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(common_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(le_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(lmp_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(lc_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(g_wbs_nv_117, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_wbs_nv_118, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_nbv_nv_117, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_nbv_nv_118, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sco_transmit_mode, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(audio_rfu_b1, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(audio_rfu_b2, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(audio_rfu_b3, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(audio_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(g_sys_sleep_in_standby_supported, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sleep_master_supported, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(g_sys_sleep_slave_supported, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(power_rfu_b1, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(power_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(win_ext, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(edr_tx_edr_delay, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(edr_rx_edr_delay, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(tx_delay, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(rx_delay, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(bb_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(agc_mode, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(diff_or_eq, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(ramp_mode, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(modem_rfu_b1, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(modem_rfu_w, 0, marlin3_pskey, 2),
    CONF_ITEM_TABLE(BQB_BitMask_1, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(BQB_BitMask_2, 0, marlin3_pskey, 1),
    CONF_ITEM_TABLE(bt_coex_threshold, 0, marlin3_pskey, 8),
    CONF_ITEM_TABLE(other_rfu_w, 0, marlin3_pskey, 2),

    {0, 0, 0, 0, 0}
};

static rf_config_t marlin3_rf_config = {
    .g_GainValue_A = {0xE000, 0xE000, 0xE000, 0xE000, 0xE000, 0xE000},
    .g_ClassicPowerValue_A = {0x3203, 0x2C03, 0x2603, 0x2003, 0x1A03, 0x1403, 0x0E03, 0x0803, 0x0803, 0x0803},
    .g_LEPowerValue_A = {0x3703, 0x3703, 0x3703, 0x3703, 0x3603, 0x3503, 0x3403, 0x3303, 0x3203, 0x2C03, 0x2603, 0x2003, 0x1A03, 0x1403, 0x0E03, 0x1603},
    .g_BRChannelpwrvalue_A = {0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803},
    .g_EDRChannelpwrvalue_A = {0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803, 0x0803},
    .g_LEChannelpwrvalue_A = {0x1603, 0x1603, 0x1603, 0x1603, 0x1603, 0x1603, 0x1603, 0x1603},
    .g_GainValue_B = {0xE000, 0xE000, 0xE000, 0xE000, 0xE000, 0xE000},
    .g_ClassicPowerValue_B = {0x5CC4, 0x7C8C, 0x7CBC, 0x9C84, 0x9CAC, 0x9CEC, 0xBC9C, 0xBCCC, 0xBCCC, 0xBCCC},
    .g_LEPowerValue_B = {0x3703, 0x3703, 0x3703, 0x3703, 0x3703, 0x3703, 0x3703, 0x3703, 0x3603, 0x2F03, 0x2903, 0x2303, 0x1E03, 0x1903, 0x1303, 0x0D03},
    .g_BRChannelpwrvalue_B = {0xBD14, 0xBCEC, 0xBCDC, 0xBCCC, 0xBCCC, 0xBCBC, 0xBCCC, 0xBCCC},
    .g_EDRChannelpwrvalue_B = {0xBCEC, 0xBCDC, 0xBCCC, 0xBCBC, 0xBCBC,0xBCBC, 0xBCBC, 0xBCBC},
    .g_LEChannelpwrvalue_B = {0xBC7C, 0x9CDC, 0x9CCC, 0x9CB4, 0x9CCC, 0x9CCC, 0x9CCC, 0x9CBC},
    .LE_fix_powerword = 0xBC7C,
    .Classic_pc_by_channel = 0xFF,
    .LE_pc_by_channel = 0xFF,
    .RF_switch_mode = 0x02,
    .Data_Capture_Mode = 0x00,
    .Analog_IQ_Debug_Mode = 0x00,
    .RF_common_rfu_b3 = 0x55,
    .RF_common_rfu_w = {0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555},
};

static const conf_entry_t marlin3_rf_table[] = {
    CONF_ITEM_TABLE(g_GainValue_A, 0, marlin3_rf_config, 6),
    CONF_ITEM_TABLE(g_ClassicPowerValue_A, 0, marlin3_rf_config, 10),
    CONF_ITEM_TABLE(g_LEPowerValue_A, 0, marlin3_rf_config, 16),
    CONF_ITEM_TABLE(g_BRChannelpwrvalue_A, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(g_EDRChannelpwrvalue_A, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(g_LEChannelpwrvalue_A, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(g_GainValue_B, 0, marlin3_rf_config, 6),
    CONF_ITEM_TABLE(g_ClassicPowerValue_B, 0, marlin3_rf_config, 10),
    CONF_ITEM_TABLE(g_LEPowerValue_B, 0, marlin3_rf_config, 16),
    CONF_ITEM_TABLE(g_BRChannelpwrvalue_B, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(g_EDRChannelpwrvalue_B, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(g_LEChannelpwrvalue_B, 0, marlin3_rf_config, 8),
    CONF_ITEM_TABLE(LE_fix_powerword, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(Classic_pc_by_channel, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(LE_pc_by_channel, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(RF_switch_mode, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(Data_Capture_Mode, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(Analog_IQ_Debug_Mode, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(RF_common_rfu_b3, 0, marlin3_rf_config, 1),
    CONF_ITEM_TABLE(RF_common_rfu_w, 0, marlin3_rf_config, 5),

    {0, 0, 0, 0, 0}

};

int get_enable_buf(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int size;

    p = msg_req;

    UINT16_TO_STREAM(p, DUAL_MODE);
    UINT8_TO_STREAM(p, ENABLE_BT);
    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

int marlin3_rf_preload(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int i, size;

    //BTD("%s", __FUNCTION__);
    p = msg_req;

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_GainValue_A[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_ClassicPowerValue_A[i]);
    }

    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEPowerValue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_BRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_EDRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_GainValue_B[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_ClassicPowerValue_B[i]);
    }


    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEPowerValue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_BRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_EDRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEChannelpwrvalue_B[i]);
    }

    UINT16_TO_STREAM(p, marlin3_rf_config.LE_fix_powerword);

    UINT8_TO_STREAM(p, marlin3_rf_config.Classic_pc_by_channel);
    UINT8_TO_STREAM(p, marlin3_rf_config.LE_pc_by_channel);
    UINT8_TO_STREAM(p, marlin3_rf_config.RF_switch_mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.Data_Capture_Mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.Analog_IQ_Debug_Mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.RF_common_rfu_b3);

    for (i = 0; i < 5; i++) {
        UINT32_TO_STREAM(p, marlin3_rf_config.RF_common_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}


int get_pskey_buf(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int i, size;

    p = msg_req;
    UINT32_TO_STREAM(p, marlin3_pskey.device_class);

    for (i = 0; i < 16; i++) {
        UINT8_TO_STREAM(p, marlin3_pskey.feature_set[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT8_TO_STREAM(p, marlin3_pskey.device_addr[i]);
    }

    UINT16_TO_STREAM(p, marlin3_pskey.comp_id);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_uart0_communication_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.cp2_log_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.LogLevel);
    UINT8_TO_STREAM(p, marlin3_pskey.g_central_or_perpheral);

    UINT16_TO_STREAM(p, marlin3_pskey.Log_BitMask);
    UINT8_TO_STREAM(p, marlin3_pskey.super_ssp_enable);
    UINT8_TO_STREAM(p, marlin3_pskey.common_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.common_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.le_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.lmp_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.lc_rfu_w[i]);
    }

    UINT16_TO_STREAM(p, marlin3_pskey.g_wbs_nv_117);
    UINT16_TO_STREAM(p, marlin3_pskey.g_wbs_nv_118);
    UINT16_TO_STREAM(p, marlin3_pskey.g_nbv_nv_117);
    UINT16_TO_STREAM(p, marlin3_pskey.g_nbv_nv_118);

    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sco_transmit_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b1);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b2);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.audio_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_in_standby_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_master_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_slave_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.power_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.power_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, marlin3_pskey.win_ext);

    UINT8_TO_STREAM(p, marlin3_pskey.edr_tx_edr_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.edr_rx_edr_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.tx_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.rx_delay);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.bb_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, marlin3_pskey.agc_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.diff_or_eq);
    UINT8_TO_STREAM(p, marlin3_pskey.ramp_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.modem_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.modem_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, marlin3_pskey.BQB_BitMask_1);
    UINT32_TO_STREAM(p, marlin3_pskey.BQB_BitMask_2);

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_pskey.bt_coex_threshold[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.other_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

void set_mac_address(uint8_t *addr)
{
    uint8_t addr_t[6] = {0x01, 0x88, 0x66, 0xDA, 0x45, 0x40};
    u32_t random;

    random =sys_rand32_get();
    memcpy(addr_t, &random, 3);
    memcpy(addr, addr_t, sizeof(addr_t));
}

char *uki_strtok_r(char *str, const char *delim, char **saveptr)
{
  char *pbegin;
  char *pend = NULL;

  /* Decide if we are starting a new string or continuing from
   * the point we left off.
   */

  if (str)
    {
      pbegin = str;
    }
  else if (saveptr && *saveptr)
    {
      pbegin = *saveptr;
    }
  else
    {
      return NULL;
    }

  /* Find the beginning of the next token */

  for (;
       *pbegin && strchr(delim, *pbegin) != NULL;
       pbegin++);

  /* If we are at the end of the string with nothing
   * but delimiters found, then return NULL.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  /* Find the end of the token */

  for (pend = pbegin + 1;
       *pend && strchr(delim, *pend) == NULL;
       pend++);


  /* pend either points to the end of the string or to
   * the first delimiter after the string.
   */

  if (*pend)
    {
      /* Turn the delimiter into a null terminator */

      *pend++ = '\0';
    }

  /* Save the pointer where we left off and return the
   * beginning of the token.
   */

  if (saveptr)
    {
      *saveptr = pend;
    }
  return pbegin;
}

char *uki_fgets(char *dst, int max, struct fs_file_t *file)
{
    char *p;
    char  ch;
    int len = 0;
    /* get max bytes or upto a newline */

    for (p = dst, max--; max > 0; max--) {
        len = fs_read(file, &ch, 1);
        if (len <= 0)
            break;
        *p++ = ch;
        if (ch == '\n')
            break;
    }
    *p = 0;
    if (p == dst || len <= 0)
        return NULL;
    return (p);
}

static void parse_number(char *p_conf_name, char *p_conf_value, void *buf,
                         int len, int size)
{
    uint8_t *dest = (uint8_t *)buf;
    char *sub_value, *p;
    uint32_t value;
    ARG_UNUSED(p_conf_name);
    sub_value = uki_strtok_r(p_conf_value, CONF_VALUES_PARTITION, &p);
    do {
        if (sub_value == NULL) {
            break;
        }

        if (sub_value[0] == '0' && (sub_value[1] == 'x' || sub_value[1] == 'X')) {
            value = strtoul(sub_value, 0, 16) & 0xFFFFFFFF;
        } else {
            value = strtoul(sub_value, 0, 10) & 0xFFFFFFFF;
        }

        switch (size) {
        case sizeof(uint8_t):
            *dest = value & 0xFF;
            dest += size;
            break;

        case sizeof(uint16_t):
            *((uint16_t *)dest) = value & 0xFFFF;
            dest += size;
            break;

        case sizeof(uint32_t):
            *((uint32_t *)dest) = value & 0xFFFFFFFF;
            dest += size;
            break;

        default:
            break;
        }
        sub_value = uki_strtok_r(NULL, CONF_VALUES_PARTITION, &p);
    } while (--len);
}

static void vnd_load_configure(const char *p_path, const conf_entry_t *entry)
{
    struct fs_file_t p_file;
    char *p_name, *p_value, *p;
    conf_entry_t *p_entry;
    char line[CONF_MAX_LINE_LEN + 1]; /* add 1 for \0 char */

    BTD("%s, Attempt to load conf from %s\n", __func__, p_path);

    if (0 == fs_open(&p_file, p_path)) {
        /* read line by line */
        while (uki_fgets(line, CONF_MAX_LINE_LEN + 1, &p_file) != NULL) {
            if (line[0] == CONF_COMMENT) continue;

            p_name = uki_strtok_r(line, CONF_DELIMITERS, &p);

            if (NULL == p_name) {
                continue;
            }

            p_value = uki_strtok_r(NULL, CONF_VALUES_DELIMITERS, &p);

            if (NULL == p_value) {
                 BTD("%s, vnd_load_conf: missing value for name: %s\n", __func__, p_name);
                continue;
            }

            p_entry = (conf_entry_t*)entry;

            while (p_entry->conf_entry != NULL) {
                if (strcmp(p_entry->conf_entry, (const char *)p_name) == 0) {
                    if (p_entry->p_action) {
                        p_entry->p_action(p_name, p_value, p_entry->buf, p_entry->len,
                                          p_entry->size);
                    } else {
                        BTD("%s, %s -> %s\n", __func__, p_name, p_value);
                        parse_number(p_name, p_value, p_entry->buf, p_entry->len,
                                    p_entry->size);
                    }
                    break;
                }

                p_entry++;
            }
        }

        fs_close(&p_file);
    } else {
        BTD("%s, vnd_load_conf file >%s< not found\n", __func__, p_path);
    }
}

int marlin3_init(void) {
    BTD("%s\n", __func__);
    int ret;

    memset(&marlin3_pskey, 0, sizeof(marlin3_pskey));
    memset(&marlin3_rf_config, 0, sizeof(marlin3_rf_config));

    ret = fs_mount(&fatfs_mnt);
    if (ret < 0 && ret!=-EBUSY) {
        BTE("Error mounting fs [%d]\n", ret);
        return -1;
    }

    vnd_load_configure(BT_CONFIG_PSKEY_FILE, &marlin3_pksey_table[0]);
    vnd_load_configure(BT_CONFIG_RF_FILE, &marlin3_rf_table[0]);
    set_mac_address(marlin3_pskey.device_addr);

    return 0;
}

void uwp56xx_vendor_init()
{
    struct net_buf *rsp, *buf;
    int size;
    char data[256] = {0};
    marlin3_init();

    BTD("send pskey\n");
    size = get_pskey_buf(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_PSKEY, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_PSKEY, buf, &rsp);
    net_buf_unref(rsp);

    BTD("send rfkey\n");
    size = marlin3_rf_preload(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_RF, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_RF, buf, &rsp);
    net_buf_unref(rsp);

    BTD("send enable\n");
    size = get_enable_buf(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_ENABLE_CMD, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_ENABLE_CMD, buf, &rsp);
    net_buf_unref(rsp);

}
