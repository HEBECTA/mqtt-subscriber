#include "serial_ubus.h"

static const struct blobmsg_policy serial_policy[__GET_MAX] = {
	[SERIAL_DATA] = { .name = "serial", .type = BLOBMSG_TYPE_STRING },
};

static const struct blobmsg_policy get_policy[__MNINFO_MAX] = {
	[MNFINFO_DATA] = { .name = "mnfinfo", .type = BLOBMSG_TYPE_TABLE },
};

void board_cb(struct ubus_request *req, int type, struct blob_attr *msg) {

	char *serial_str = (char *) req->priv;

	struct blob_attr *tb[__MNINFO_MAX];
	struct blob_attr *serial[__GET_MAX];

	blobmsg_parse(get_policy, __MNINFO_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[0]) {
                req->status_msg = true;
		req->status_code = ENODATA;
		return;
	}

	blobmsg_parse(serial_policy, __GET_MAX, serial,
	                blobmsg_data(tb[MNFINFO_DATA]), blobmsg_data_len(tb[MNFINFO_DATA]));

	
	sprintf(serial_str, "%s", blobmsg_get_string(serial[SERIAL_DATA]));
}


int connect_ubus(uint32_t *id, struct ubus_context **ctx){

        *ctx = ubus_connect(NULL);

        if ( !(*ctx) ) 
                return ECONNREFUSED;

        if ( ubus_lookup_id(*ctx, "mnfinfo", id))
                return ECONNREFUSED;
                
        return EXIT_SUCCESS;
}