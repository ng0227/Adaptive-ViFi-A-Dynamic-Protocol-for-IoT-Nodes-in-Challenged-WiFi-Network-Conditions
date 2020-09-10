/*
 * NL80211 callback functions.
 */

#include "headers.h"

// Callback for errors.
int error_handler(struct sockaddr_nl *nla,
        struct nlmsgerr *err, void *arg) {
    int *ret = arg;
    *ret = err->error;
    return NL_STOP;
}

// Callback for NL_CB_FINISH.
int finish_handler(struct nl_msg *msg, void *arg) {
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

// Callback for NL_CB_ACK.
int ack_handler(struct nl_msg *msg, void *arg) {
    int *ret = arg;
    *ret = 0;
    return NL_STOP;
}

// Callback for NL_CB_SEQ_CHECK.
int no_seq_check(struct nl_msg *msg, void *arg) {
    return NL_OK;
}

// Callback for NL_CB_VALID within nl_get_multicast_id().
// Source: http://sourcecodebrowser.com/iw/0.9.14/genl_8c.html.
int family_handler(struct nl_msg *msg, void *arg) {
    struct handler_args *grp = arg;
    struct nlattr *tb[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcgrp;
    int rem_mcgrp;

    nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
        genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[CTRL_ATTR_MCAST_GROUPS]) return NL_SKIP;

    nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
        struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

        nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
            nla_data(mcgrp), nla_len(mcgrp), NULL);

        if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] 
                || !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
            continue;

        if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]), grp->group,
                nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
            continue;

        grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
        break;
    }

    return NL_SKIP;
}

// Function called by the kernel when the scan is done or has been aborted.
int callback_trigger(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct trigger_results *results = arg;

    if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED) {
        printf("Got NL80211_CMD_SCAN_ABORTED.\n");
        results->done = 1;
        results->aborted = 1;
    } else if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS) {
        results->done = 1;
        results->aborted = 0;
    }
    return NL_SKIP;
}
