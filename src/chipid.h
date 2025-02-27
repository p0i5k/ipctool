#ifndef CHIPID_H
#define CHIPID_H

#include <stdbool.h>

extern char system_id[128];
extern char system_manufacturer[128];
extern char board_id[128];
extern char board_ver[128];
extern char board_manufacturer[128];
extern char board_specific[1024];
// vendor specific data
extern int chip_generation;
extern char chip_name[128];
extern char chip_manufacturer[128];
extern char short_manufacturer[128];
extern char control[128];
extern char sensor_id[128];
extern char sensor_manufacturer[128];
extern char nor_chip[128];

const char *getchipname();

#endif /* CHIPID_H */
