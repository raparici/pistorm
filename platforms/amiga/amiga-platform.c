#include "../platforms.h"
#include "amiga-autoconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int handle_register_read_amiga(unsigned int addr, unsigned char type, unsigned int *val);
int handle_register_write_amiga(unsigned int addr, unsigned int value, unsigned char type);

extern int ac_done;
extern int ac_pic_count;
extern int ac_type[AC_PIC_LIMIT];
extern int ac_index[AC_PIC_LIMIT];

const char *z2_autoconf_id = "z2_autoconf_fast";
const char *z2_autoconf_zap_id = "^2_autoconf_fast";

int custom_read_amiga(struct emulator_config *cfg, unsigned int addr, unsigned int *val, unsigned char type) {
    if (!ac_done && addr >= AC_BASE && addr < AC_BASE + AC_SIZE) {
        if (ac_pic_count == 0) {
            ac_done = 1;
            return -1;
        }

        if (type == OP_TYPE_BYTE) {
            //printf("Read from autoconf area.\n");
            *val = autoconfig_read_memory_8(cfg, addr);
            return 1;
        }
    }

    return -1;
}

int custom_write_amiga(struct emulator_config *cfg, unsigned int addr, unsigned int val, unsigned char type) {
    if (cfg || addr || val || type) {}
    if (!ac_done && addr >= AC_BASE && addr < AC_BASE + AC_SIZE) {
        if (type == OP_TYPE_BYTE) {
            if (ac_pic_count == 0) {
                ac_done = 1;
                return -1;
            }

            //printf("Write to autoconf area.\n");
            autoconfig_write_memory_8(cfg, addr, val);
            return 1;
        }
    }

    return -1;
}

int setup_platform_amiga(struct emulator_config *cfg) {
    if (cfg) {}
    printf("Performing setup for Amiga platform.\n");
    // Look for Z2 autoconf Fast RAM by id
    int index = get_named_mapped_item(cfg, "z2_autoconf_fast");
    more_z2_fast:;
    if (index != -1) {
        cfg->map_id[index][0] = '^';
        int resize_data = 0;
        if (cfg->map_size[index] > 8 * SIZE_MEGA) {
            printf("Attempted to configure more than 8MB of Z2 Fast RAM, downsizng to 8MB.\n");
            resize_data = 8 * SIZE_MEGA;
        }
        else if(cfg->map_size[index] != 2 * SIZE_MEGA && cfg->map_size[index] != 4 * SIZE_MEGA && cfg->map_size[index] != 8 * SIZE_MEGA) {
            printf("Z2 Fast RAM may only provision 2, 4 or 8MB of memory, resizing to ");
            if (cfg->map_size[index] > 8 * SIZE_MEGA)
                resize_data = 8 * SIZE_MEGA;
            else if (cfg->map_size[index] > 4 * SIZE_MEGA)
                resize_data = 4 * SIZE_MEGA;
            else
                resize_data = 2 * SIZE_MEGA;
            printf("%dMB.\n", resize_data / SIZE_MEGA);
        }
        if (resize_data) {
            free(cfg->map_data[index]);
            cfg->map_size[index] = resize_data;
            cfg->map_data[index] = (unsigned char *)malloc(cfg->map_size[index]);
        }
        printf("%dMB of Z2 Fast RAM configured at $%lx\n", cfg->map_size[index] / SIZE_MEGA, cfg->map_offset[index]);
        ac_type[ac_pic_count] = ACTYPE_MAPFAST_Z2;
        ac_index[ac_pic_count] = index;
        ac_pic_count++;
        printf("AAAAHH!\n");
    }
    else
        printf("No Z2 Fast RAM configured.\n");
    index = get_named_mapped_item(cfg, "z2_autoconf_fast");
    if (index != -1)
        goto more_z2_fast;
    
    for (int i = 0; i < MAX_NUM_MAPPED_ITEMS; i ++) {
        if (cfg->map_id[i] && strcmp(cfg->map_id[i], z2_autoconf_zap_id) == 0) {
            cfg->map_id[i][0] = 'z';
        }
    }
    
    return 0;
}

void setvar_amiga(char *var, char *val) {
    if (var || val) {}
}

void create_platform_amiga(struct platform_config *cfg, char *subsys) {
    cfg->register_read = handle_register_read_amiga;
    cfg->register_write = handle_register_write_amiga;
    cfg->custom_read = custom_read_amiga;
    cfg->custom_write = custom_write_amiga;
    cfg->platform_initial_setup = setup_platform_amiga;

    cfg->setvar = setvar_amiga;

    if (subsys) {
        cfg->subsys = malloc(strlen(subsys) + 1);
        strcpy(cfg->subsys, subsys);
    }
}
