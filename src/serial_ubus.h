#ifndef SERIAL_UBUS_H
#define SERIAL_UBUS_H

#include <libubox/blobmsg_json.h>
#include <libubus.h>


enum {
	SERIAL_DATA,
	__GET_MAX
};

enum {
	MNFINFO_DATA,
	__MNINFO_MAX,
};

static const struct blobmsg_policy serial_policy[__GET_MAX];

static const struct blobmsg_policy get_policy[__MNINFO_MAX];

void board_cb(struct ubus_request *req, int type, struct blob_attr *msg);

int connect_ubus(uint32_t *id, struct ubus_context **ctx);

#endif