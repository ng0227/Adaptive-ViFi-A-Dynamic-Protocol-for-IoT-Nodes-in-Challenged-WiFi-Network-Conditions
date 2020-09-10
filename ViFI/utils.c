/*
* Utility functions.
*/

#include "headers.h"

// Intialize the netlink socket.
struct nl_sock * init_socket() {
	/*
	 * Open socket to kernel.
	 */

	// Allocate new netlink socket in memory.
	struct nl_sock *socket = nl_socket_alloc();
	// Create file descriptor and bind socket.
	genl_connect(socket);
	// Set socket as a non-blocking socket.
	nl_socket_set_nonblocking(socket);

	return socket;
}

// Source: http://sourcecodebrowser.com/iw/0.9.14/genl_8c.html.
int nl_get_multicast_id(struct nl_sock *sock,
		const char *family, const char *group) {
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct handler_args grp = { .group = group, .id = -ENOENT, };

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0) goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0) nl_recvmsgs(sock, cb);

	if (ret == 0) ret = grp.id;

	nla_put_failure:
		out:
			nl_cb_put(cb);
		out_fail_cb:
			nlmsg_free(msg);
			return ret;
}

// Create a valid vendor specific IE using the data passed by the client.
struct ie_info * create_vendor_ie(char *data) {
	size_t vendor_tag_number_len = 1;
	size_t vendor_oui_len = 3;
	size_t data_size = strlen(data);
	size_t ie_size = vendor_oui_len + data_size;
	struct ie_info * ie = (struct ie_info *)malloc(sizeof(struct ie_info));
	ie->ie_len = vendor_tag_number_len + ie_size + 1;
	ie->ie_data = (unsigned char *)malloc(ie->ie_len);

	// Vendor tag number.
	ie->ie_data[0] = 221;
	// Size of IE.
	ie->ie_data[1] = ie_size;
	// Vendor OUI (= 123). This is also a random value.
	ie->ie_data[2] = 1;
	ie->ie_data[3] = 2;
	ie->ie_data[4] = 3;

	int data_idx = 0;
	for (int i = 5; i < data_size + 5; ++i) {
		ie->ie_data[i] = data[data_idx];
		++data_idx;
	}

	return ie;
}

// Extract ack from Vendor IE inside probe response frame.
char * extract_ack_from_vendor_ie(unsigned char len, unsigned char *data) {
	// OUI of our vendor.
	unsigned char ps_oui[3] = {0x01, 0x02, 0x03};   // 123
	char *ack_seq_num = (char *)malloc((len - 2) * sizeof(char));

	if (len >= 4 && memcmp(data, ps_oui, 3) == 0) {
		int j = 0;
		for (int i = 0; i < len - 3; i++) {
			ack_seq_num[j] = data[i + 3];
			j++;
		}
		ack_seq_num[j] = '\0';
		return ack_seq_num;
	}

	return NULL;
}

// Get ACK for sent stuffed probe request frames.
// The ACK will be an IE in probe response frames.
//
// This function will be called by the kernel with a dump
// of the successful scan's data. Called for each SSID.
int get_ack(struct nl_msg *msg, void *arg) {
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};

	// Parse and error check.
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_BSS]) {
		return NL_SKIP;
	}

	if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
		return NL_SKIP;
	}

	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	if (!bss[NL80211_BSS_INFORMATION_ELEMENTS])
		return NL_SKIP;

	// Extract out the IEs.
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		unsigned char *ies = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		int ies_len = nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]);

		while (ies_len >= 2 && ies_len >= ies[1]) {
			// Vendor Specific IE.
			if (ies[0] == 221) {
				char *ack_seq_num_str = extract_ack_from_vendor_ie(ies[1], ies + 2);
				int *ack_seq_num = arg;
				if (ack_seq_num_str) {
					*ack_seq_num = atoi(ack_seq_num_str);
					free(ack_seq_num_str);
				}
			}
			ies_len -= ies[1] + 2;
			ies += ies[1] + 2;
		}
	}

	return NL_SKIP;
}

// Read the complete contents of file at once.
char * read_file(char *file_path) {
	FILE *file;
	char *buffer;
	long numbytes;

	// Open the file for reading.
	file = fopen(file_path, "r");

	// Return if the file does not exist.
	if(file == NULL)
		return NULL;

	// Get the number of bytes.
	fseek(file, 0L, SEEK_END);
	numbytes = ftell(file);

	// Reset the file position indicator to the beginning of the file.
	fseek(file, 0L, SEEK_SET);

	buffer = (char*)malloc(numbytes * sizeof(char)); 

	// Memory error.
	if(buffer == NULL)
		return NULL;

	// Copy all the text into the buffer.
	fread(buffer, sizeof(char), numbytes, file);
	fclose(file);
  
	return buffer;
}

// Divide data into chunks of 252 bytes.
char ** split_data(char *data, int n_ies) {
	int len = strlen(data);
	int i;
	char **raw_ies_data = (char **)calloc(n_ies, (sizeof(char **)));

	for (i = 0; i < n_ies - 1; ++i) {
		raw_ies_data[i] = (char *)calloc(252, sizeof(char *));
		memcpy(raw_ies_data[i], &data[i*252], 252);
	}

	int len_left = len - ((n_ies - 1) * 252);
	raw_ies_data[i] = (char *)calloc(len_left, sizeof(char *));
	memcpy(raw_ies_data[i], &data[i*252], len_left);

	return raw_ies_data;
}

// Get number of IEs required.
int get_n_ies_reqd(size_t len_data_to_stuff) {
	if (len_data_to_stuff % 252 == 0) {
		return (len_data_to_stuff / 252);
	} else {
		return (len_data_to_stuff / 252) + 1;
	}
}

// Client side logging.
void log_info(char *wifi_interface, unsigned int data_origin_ts, unsigned int data_origin_ts_usec, char *data_origin_ts_str,
	unsigned int data_tx_ts, unsigned int data_tx_ts_usec, int data_size, int attempt, int status) {

	// Log file name = client's wifi interface name + data origin timstamp
	char log_file_path[50] = "logs/client_log.log";
	//strcat(log_file_path, wifi_interface);
	//strcat(log_file_path, "_");
	//strcat(log_file_path, data_origin_ts_str);
	//strcat(log_file_path, ".txt");

	FILE *file;
	file = fopen(log_file_path, "a");

	if (file == NULL) {
		printf("\nUnable to open '%s' file.\n", log_file_path);
		return;
	}

	// Attempt, Data Size, Data Origin Timestamp, Data Transmission TImestamp, Status
	fprintf(file, "%d, ", attempt);
	fprintf(file, "%d, ", data_size);
	fprintf(file, "%u%06u, ", data_origin_ts, data_origin_ts_usec);
	fprintf(file, "%u%06u, ", data_tx_ts, data_tx_ts_usec);
	fprintf(file, "%d\n", status);

	fclose(file);
}




//returns current micro seconds
long get_usecs (void)
{
   struct timeval t;
   gettimeofday(&t,NULL);
   return t.tv_sec*1000000+t.tv_usec;
}

//returns the difference between startTime and currentTime in seconds
double secondsElapsed(long startTime,long currentTime)
{
	double dur = ((double)(currentTime-startTime))/1000000;
	return dur;
}
