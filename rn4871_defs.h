#ifndef RN4871_DEFS_H
#define RN4871_DEFS_H

#define RN4871_DELIMITER_STATUS ('%')

enum flags_events_ren4871_e {
  FLAG_RN4871_TX = 0b000001,
  FLAG_RN4871_RX = 0b000010,
};

enum rn4871_cmd_e {
  CMD_NONE,
  CMD_MODE_ENTER, /* $$$ */
  CMD_MODE_QUIT, /* --- */
  CMD_REBOOT, /* R,1 */
  CMD_RESET_FACTORY, /* SF */
  CMD_SET_BT_NAME, /* S- */
  CMD_SET_DEVICE_NAME, /* SN */
  CMD_GET_DEVICE_NAME, /* GN */
  CMD_SET_SERVICES, /* SS */
  CMD_DUMP_INFOS, /* D */
  CMD_GET_VERSION, /* V */
  CMD_CLEAR_ALL_SERVICES, /* PZ */
  CMD_CREATE_PRIVATE_SERVICE, /* PS */
  CMD_CREATE_PRIVATE_CHARACTERISTIC, /* PC */
  CMD_SERVER_WRITE_CHARACTERISTIC, /* SHW */
  CMD_SERVER_READ_CHARACTERISTIC, /* SHR */
};

const char TABLE_COMMAND[][10] = {
  "",
  "$$$",
  "---",
  "R,1",
  "SF",
  "S-",
  "SN",
  "GN",
  "SS",
  "D",
  "V",
  "PZ",
  "PS",
  "PC",
  "SHW",
  "SHR",
};

#endif /* RN4871_DEFS_H */