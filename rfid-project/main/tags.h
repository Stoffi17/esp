#ifndef TAGS_H
#define TAGS_H

#include <stdint.h>
#include "rc522_types.h"

typedef struct {
    uint8_t uid[RC522_PICC_UID_SIZE_MAX];
    uint8_t length;
    char name[32];
} valid_tag_t;

extern valid_tag_t *valid_tags;
extern size_t valid_tags_count;
extern volatile bool door_locked;

esp_err_t add_tag(const char *tag_name);
esp_err_t remove_tag(const uint8_t *uid, uint8_t uid_length);

void servo_open(void);
void servo_close(void);
void toggle_lock(void);

#endif // TAGS_H
