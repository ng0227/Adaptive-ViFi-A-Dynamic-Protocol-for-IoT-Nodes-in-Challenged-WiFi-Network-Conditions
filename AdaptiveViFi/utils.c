/*
* Utility functions.
*/

#include "headers.h"

#define BUFFER_SIZE 1024
#define CONSOLE_STRING_LENGTH 34

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


int channelNumber(char * string)
{
	if(strstr(string,"channel 11"))//because channel 1 is substring of channel 11 so we have to check channel 11 first.
		return 11;
	if(strstr(string,"channel 1"))
		return 1;
	if(strstr(string,"channel 2"))
		return 2;
	if(strstr(string,"channel 3"))
		return 3;
	if(strstr(string,"channel 4"))
		return 4;
	if(strstr(string,"channel 5"))
		return 5;
	if(strstr(string,"channel 6"))
		return 6;
	if(strstr(string,"channel 7"))
		return 7;
	if(strstr(string,"channel 8"))
		return 8;
	if(strstr(string,"channel 9"))
		return 9;
	if(strstr(string,"channel 10"))
		return 10;
	
	//malformed row
	return -1;
}


struct channel_Utilisation_Results * getChannelWithMinimumChannelUtilisation(char * apFilter)//apFilter points cs AP Names or MAC
{
	struct channel_Utilisation_Results * results = (struct channel_Utilisation_Results *)malloc(sizeof(struct channel_Utilisation_Results));
	results->channel_Utilisation_Percentage = 0.0f;
	results->channelNumber = 12;
	strcpy(results->buffer,"ERROR in file opening");
	
	FILE *fileDescriptor;
	char line[BUFFER_SIZE];
	//char* filename = "./iwGrepLog.txt"; // the file in which iwScan results are stored
	_Bool extractChannelUtilisationValue;
	int channel = -1;
	int numberOfChannels = 11;
	int * sumOfChannelUtilisationPerChannel = (int *)malloc(sizeof(int)*numberOfChannels);
	int * countOfBeaconsPerChannel = (int *)malloc(sizeof(int)*numberOfChannels);
	char buffer[BUFFER_SIZE];
	char * interfaceName = "monitor";
	strncpy(buffer,"sudo iw dev ",BUFFER_SIZE);
	strncat(buffer,interfaceName,BUFFER_SIZE);	//hardcoding virtual interface
	strncat(buffer," scan ",BUFFER_SIZE);

	for(int i=0;i<numberOfChannels;i++)
	{
		sumOfChannelUtilisationPerChannel[i] =0;
		countOfBeaconsPerChannel[i] = 0;
	}	

	//fileDescriptor = fopen(filename, "r");
	fileDescriptor = popen(buffer, "r");
	if (fileDescriptor == NULL){
		printf("Could not open file %s",buffer);
		return results;
	}

	// Reading and parsing the results of IW scan
	while (fgets(line, BUFFER_SIZE, fileDescriptor) != NULL)
	{
	  
	   if(strstr(line,"DS Parameter set")) 
	   {		  
		    
		channel = channelNumber(line);
		if(channel!=-1)
			extractChannelUtilisationValue = 1;
		else
			extractChannelUtilisationValue = 0;
	   }
	   
	   if(extractChannelUtilisationValue) 
	   {
	   	if(strstr(line,"channel utilisation")) //It is assumed that if channel utilisation field is present then it will definitely have a value
	   	{
	   		char * pointerToSlash = strchr(line,'/');
			*(pointerToSlash) = '\0';
			int channelUtilisationValue = 0;
			if(*(pointerToSlash-3)!=' ')  //p-3 p-2 p-1 p
				sscanf((pointerToSlash-3),"%d",&channelUtilisationValue); //100/255
			else if(*(pointerToSlash-2)!=' ')
				sscanf((pointerToSlash-2),"%d",&channelUtilisationValue);//10/255
			else if(*(pointerToSlash-1)!=' ')
				sscanf((pointerToSlash-1),"%d",&channelUtilisationValue);//1/255		

			//store the channel utilisation value in corresponding channel field
	   		//printf("My channel is %d and my utilisation is %d\n",channel,channelUtilisationValue);
			sumOfChannelUtilisationPerChannel[channel-1] += channelUtilisationValue;
			countOfBeaconsPerChannel[channel-1] += 1;		

	   	}
	   }
	   
	}
	fclose(fileDescriptor);
	  
	  int channelWithMinimumChannelUtilisation = INT8_MIN;
	  double currentChannelUtilisationPercentage = 0.0f;
	  double minChannelUtilisationPercentage = 100.0f;

/*
int i=2;
	  if(countOfBeaconsPerChannel[i]!=0){
currentChannelUtilisationPercentage = (double)sumOfChannelUtilisationPerChannel[i]/countOfBeaconsPerChannel[i];
currentChannelUtilisationPercentage = (currentChannelUtilisationPercentage/255)*100;
printf("Channel Utilisation Percentage of channel Number %d : %f\n",(i+1),currentChannelUtilisationPercentage);
minChannelUtilisationPercentage = currentChannelUtilisationPercentage;
channelWithMinimumChannelUtilisation = i+1; 
}
*/

double cu1=0.0f;
double cu11=0.0f;

	  for(int i = 0;i<numberOfChannels;i=i+10)
	   {
	       if(countOfBeaconsPerChannel[i]!=0)//some beacons were received on this channel
	   	{
	   		currentChannelUtilisationPercentage = (double)sumOfChannelUtilisationPerChannel[i]/countOfBeaconsPerChannel[i];
			currentChannelUtilisationPercentage = (currentChannelUtilisationPercentage/255)*100; 
		   	printf("\nCU of all channels (available):\n");
		   	printf("Channel Utilisation Percentage of channel Number %d : %f\n",(i+1),currentChannelUtilisationPercentage);
if(i==0)
	cu1=currentChannelUtilisationPercentage;
if(i==10)
	cu11=currentChannelUtilisationPercentage;
			if(currentChannelUtilisationPercentage<minChannelUtilisationPercentage)
			{
				minChannelUtilisationPercentage = currentChannelUtilisationPercentage;
				channelWithMinimumChannelUtilisation = i+1; 
			}
		}
	   }
	   
	results->channel_Utilisation_Percentage = minChannelUtilisationPercentage;
	results->channelNumber = channelWithMinimumChannelUtilisation;
	strcpy(results->buffer," ");
	sprintf(results->buffer,"%lf,%d,%lf,%lf",minChannelUtilisationPercentage,channelWithMinimumChannelUtilisation,cu1,cu11);//may add interval later
	printf("\n Minimum Channel Utilisation is %f and is on Channel Number %d\n ",minChannelUtilisationPercentage,channelWithMinimumChannelUtilisation);
	return results;
}

double lowCUTable[11][2] = {
{17.6, 20000.0},
{10.79, 40000.0},
{4.08, 240000.0},
{2.84, 440000.0},
{1.1, 1000000.0},
{0.72, 2000000.0},
{0.64,3000000.0},
{0.50,4000000.0},
{0.39,5000000.0},
{0.05,8000000.0},
{0.05,16000000.0}
};

double mediumCUTable[11][2] = {
{18.03, 20000.0},
{12.05, 40000.0},
{4.2, 240000.0},
{3.09,440000.0},
{1.81, 1000000.0},
{1.18,2000000.0},
{0.7,3000000.0},
{0.7,4000000.0},
{0.3,5000000.0},
{0.3,8000000.0},
{0.05,16000000.0}
};

double highCUTable[11][2] = {
{18.46, 20000.0},
{14.63, 40000.0},
{5.06, 240000.0},
{3.8, 440000.0},
{2.57, 1000000.0},
{1.53,2000000.0},
{0.64,3000000.0},
{0.50,4000000.0},
{0.39,5000000.0},
{0.05,8000000.0},
{0.05,16000000.0}
};


double selectIntervalFromBenchmarkingTable(double benchmarkingTable[11][2],double currentCU,double threshold)
{

	if(currentCU>=threshold)
		return benchmarkingTable[10][1];

	for(int i=0; i<11; i++){
		if(benchmarkingTable[i][0]+currentCU < threshold)
		return benchmarkingTable[i][1];
	}
}

double calculateInterval(double channelUtilisationPercentage,double threshold,double interval,int currentChannel,int selectedChannel)
{
	double selectedInterval = interval;
	
	// when iw scan returns a valid CU only then interval needs to be modified!   
	if(channelUtilisationPercentage!=0.00 && currentChannel==selectedChannel && selectedChannel!=12)
	{	
	   
		if(channelUtilisationPercentage>0 && channelUtilisationPercentage <=10){
			selectedInterval = selectIntervalFromBenchmarkingTable(lowCUTable,channelUtilisationPercentage,threshold);
		}
		else if(channelUtilisationPercentage>10 && channelUtilisationPercentage <=50){
			selectedInterval = selectIntervalFromBenchmarkingTable(mediumCUTable,channelUtilisationPercentage,threshold);
		}
		else{
			selectedInterval = selectIntervalFromBenchmarkingTable(highCUTable,channelUtilisationPercentage,threshold);
		}	
	}

printf("\nselected interval = %f\n",selectedInterval);
return selectedInterval;
}
/*
double calculateInterval(double channelUtilisationPercentage,double threshold,double interval,int currentChannel,int selectedChannel)
{
	double minimumIntervalValue = 1.0;
	double maximumIntervalValue = 900.00;
	
	// when iw scan returns a valid CU only then interval needs to be modified!   
	if(channelUtilisationPercentage!=0.00&&currentChannel==selectedChannel&&selectedChannel!=12)
	{	
	   if(channelUtilisationPercentage>threshold) {
		  if(interval==1)
			interval = 2;
		 else if(interval*2 <= maximumIntervalValue)
		 {
			interval = interval*2; 
		 }

		} 
	    else
	    {
		if(interval/2 >= minimumIntervalValue)
		{
			interval = interval/2;
		}
	   }  
	 }
	 else if(channelUtilisationPercentage!=0.00)
	 {

		interval = 1;// INTERVAL resets after a new channel is selected 
 	  if(channelUtilisationPercentage>threshold) {
		  if(interval==1)
			interval = 2;
		 else if(interval*2 <= maximumIntervalValue)
		 {
			interval = interval*2; 
		 }

		} 
	    else
	    {
		if(interval/2 >= minimumIntervalValue)
		{
			interval = interval/2;
		}
	   }
	 
	 }
	 	 
	 return 0;
}
*/
