/*
 * Stuff data inside probe management frames.
 */

#include "headers.h"

/*
 * Function implementing probe stuffing.
 * 
 * It starts the scan and stuffs data as vendor 
 * specific IEs inside probe request packets.
 */
int do_probe_stuffing(struct nl_sock *socket, int interface_index,
        int driver_id, int ies_len, unsigned char *ies_data) {
    struct trigger_results results = {
        .done = 0,
        .aborted = 0,
    };
    struct nl_msg *msg;

struct nl_msg *freqs;

    struct nl_cb *cb;
    struct nl_msg *ssids_to_scan;
    int err;
    int ret;
    int mcid = nl_get_multicast_id(socket, "nl80211", "scan");
    // Without this, callback_trigger() won't be called.
    nl_socket_add_membership(socket, mcid);

    // Allocate the messages and callback handler.
    msg = nlmsg_alloc();
    if (!msg) {
        printf("ERROR: Failed to allocate netlink message for msg.\n");
        return -ENOMEM;
    }

    ssids_to_scan = nlmsg_alloc();
    if (!ssids_to_scan) {
        printf("ERROR: Failed to allocate netlink message for ssids_to_scan.\n");
        nlmsg_free(msg);
        return -ENOMEM;
    }
    
    freqs = nlmsg_alloc();
if(!freqs){
fprintf(stderr,"failed to allocate netlink message");

if(freqs)
	nlmsg_free(freqs);
return -ENOMEM;

}

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        printf("ERROR: Failed to allocate netlink callbacks.\n");
        nlmsg_free(msg);
        nlmsg_free(ssids_to_scan);
        return -ENOMEM;
    }

    /*
     * Setup the messages and callback handler.
     */

    // Setup which command to run.
    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);
    // Add message attribute, which interface to use.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, interface_index);
    // Stuff IEs.
    nla_put(msg, NL80211_ATTR_IE, ies_len, ies_data);
    
      //selective broadcast		
     //nla_put_u32(freqs, 1, 5*3 + 2407);
     //nla_put_nested(msg,NL80211_ATTR_SCAN_FREQUENCIES, freqs);
    
    // Scan all SSIDs.
    nla_put(ssids_to_scan, 1, 0, "");
    // Add message attribute, which SSIDs to scan for.
    nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids_to_scan);

    nlmsg_free(ssids_to_scan);

    // Add the callback.
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger, &results);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
    // No sequence checking for multicast messages.
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

    // Send NL80211_CMD_TRIGGER_SCAN to start the scan.
    // The kernel may reply with NL80211_CMD_NEW_SCAN_RESULTS on
    // success or NL80211_CMD_SCAN_ABORTED if another
    // scan was started by another process.
    err = 1;
    ret = nl_send_auto(socket, msg);

    printf("\nNL80211_CMD_TRIGGER_SCAN sent %d bytes to the kernel.\n", ret);
    printf("Broadcasting stuffed probe request frames.\n");

    while (err > 0)
        // First wait for ack_handler(). This helps with basic errors.
        ret = nl_recvmsgs(socket, cb);

    if (err < 0)
        printf("WARNING: err has a value of %d.\n", err);

    if (ret < 0) {
        printf("ERROR: nl_recvmsgs() returned %d (%s).\n", ret, nl_geterror(-ret));
        nl_geterror(-ret);
        return ret;
    }

    while (!results.done)
        // Now wait until the scan is done or aborted.
        nl_recvmsgs(socket, cb);

    if (results.aborted) {
        printf("ERROR: Kernel aborted scan.\n");
        return 1;
    }

    // Cleanup.
    nlmsg_free(msg);
    nl_cb_put(cb);
    nl_socket_drop_membership(socket, mcid);

    return 0;
}
