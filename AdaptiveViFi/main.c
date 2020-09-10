
/*
 *  Main function.
 */

#include "headers.h"

#define BUFFER_SIZE 2048

volatile _Bool shutdownThread = 0;
long threadTime = 0;//t
pthread_mutex_t * lock;//t
//to-do: take iface and sec as user input



//records and monitor channelUtilisationValue
void * analyseInformationFromIWscan(void * arguments)
{
	struct thread_Arguments * args = (struct thread_Arguments *) arguments;
	char * wifi_interface = "monitor";//h
	char * channelNumber = "6";//h
	struct channel_Utilisation_Results * results;	
	while(!shutdownThread)
	{
long ttime = get_usecs();		
printf("\nThread started again: %d\n",ttime);
	
		results = getChannelWithMinimumChannelUtilisation("No AP filtering in use right now!");
		args->result->channel_Utilisation_Percentage = results->channel_Utilisation_Percentage;
		args->result->channelNumber = results->channelNumber;
		strcpy(args->result->buffer,results->buffer);//unsafe why ?
		free(results);
		
		pthread_mutex_lock(lock);//t
		threadTime = get_usecs();//t
		printf("\n%ld\n",threadTime);
		pthread_mutex_unlock(lock);//t
		
		args->resultCalculated = 1;

		printf("\nThread Going to sleeeeeeep\n");
	
		sleep(10);//In seconds		
	}
	
}


int main(int argc, char **argv) {

    int opt;
    int effort = 2;
    char *wifi_interface = NULL;
    char *data_to_stuff = NULL;
    char *file_path = NULL;
    int band = 0;	
    char row[BUFFER_SIZE]; 
    lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));//t
    	int numberOfChannels = 11;//by default 2.4 Ghz
    	long startTime = get_usecs();
    	long currentTime;
    	_Bool userInput = 0;
	char *channelNumber = "1"; // hardcoded value   
	int chanNum = 1;// to set frequency	
	double minimumIntervalValue = 1000000.0;
    	double interval = minimumIntervalValue ;
    	
    	char * fileName = "channelUtilisationFromIWLog.csv";
	int fileDescriptor = open(fileName,O_CREAT|O_RDWR|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	char * columnNames = "Channel Utilization,Current Channel,cu1,cu11,Transmission Interval\n";
	int wstatus=0;
	wstatus = write(fileDescriptor,columnNames,strlen(columnNames));
	
	int packetLossCount=0;
	int successPacketcount=0;
	int totalPacketsGenerated=0;
	int driverError=0;

	int probeNumber=1;
	if(fileDescriptor!=-1)
	{
		printf("\nLogging will be done and logs will be saved in channelUtilisationLog.csv\n");
	}
	else
	{
		printf("\nERROR: file creation failed!\n");
	}
    	
    /*
     * Reading command line parameters.
     *
     * i = WiFi interface on which scan request will be sent. The data
     *     will be stuffed inside the PReq packets sent during the scan.
     * d = Data to stuff inside the IEs.
     * f = Path to the file containing data to stuff.
     * e = Delivery effort (1 = Best delivery, 2 = Guaranteed delivery). Default = 2.
     * b = frequency band to be used 2.4 Ghz or 5 Ghz -r-
     * NOTE: We can specify data to stuff inside IEs using either 'd' or 'f'
     * command line parameter. If both are specified, then we will give
     * preference to file, i.e. data to stuff will be read from the specified file.
     *
     * E.g. (read data from CLI): sudo ./probe_stuffing -i wlan0 -d "LoremIpsumDolor"
     * E.g. (read data from file): sudo ./probe_stuffing -i wlan0 -f abc.txt
     */
    while ((opt = getopt(argc, argv, "i:d:f:e:")) != -1) {
        switch (opt) {
            case 'i':
                wifi_interface = optarg;
                break;
            case 'd':
                data_to_stuff = optarg;
                userInput = 1;
                break;
            case 'f':
                file_path = optarg;
                userInput = 1;
                break;
            case 'e':
                effort = atoi(optarg);
                break;
/*            case 'b':*/
/*            	band = atoi(optarg);    */
/*        	break;*/
        }
    }


/*    if(band==0)*/
/*    {*/
/*    	printf("Error: Invalid command line argument(s). Requires band.\n");*/
/*	exit(EXIT_FAILURE);*/
/*    */
/*    }*/

	if(band==5)//currently not programmed for
		numberOfChannels = 25 ;//https://www.accessagility.com/blog/introduction-to-5-ghz-wifi-channels	

	
	//Thread creation:
	pthread_t * thread = (pthread_t *)malloc(sizeof(pthread_t));
	struct channel_Utilisation_Results * results = (struct channel_Utilisation_Results *)malloc(sizeof(struct channel_Utilisation_Results));
	struct thread_Arguments * arguments = (struct thread_Arguments *)malloc(sizeof(struct thread_Arguments));
	arguments->resultCalculated = 0;	
	arguments->result = results;
	
	//creates a new thread
	pthread_create(thread,NULL,analyseInformationFromIWscan,(void *)arguments);
	threadTime = get_usecs();//t
	

    // Check if client provided a WiFi interface via CLI.
    if (!wifi_interface) {
        printf("Error: Invalid command line argument(s). Requires interface.\n");
        exit(EXIT_FAILURE);
    }

    // Check if client provided a file path to read data from via CLI.
    if (file_path) {
        data_to_stuff = read_file(file_path);//headers.h
        if (!data_to_stuff) {
            printf("Error: Unable to read input file.\n");
            exit(EXIT_FAILURE);
        }
    }
    // } else if (!data_to_stuff) { // Exit if client has not provided any data to stuff via CLI.
    //     printf("Error: Invalid command line argument(s). Requires data "
    //         "either from command line or from a file.\n");
    //     exit(EXIT_FAILURE);
    // }

    if (effort == 2) {
        printf("Effort: Guaranteed delivery\n");
    } else {
        printf("Effort: Best delivery\n");
    }

   

    while(1) {

        printf("\n======================================\n\n");
      // 	if(!userInput)
       // {
		printf("Enter data to stuff: ");
		data_to_stuff = (char*)malloc(302);
		
										
sprintf(data_to_stuff,"%d,aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",probeNumber);
probeNumber=probeNumber+1;

	//} 
        int interface_index = if_nametoindex(wifi_interface);//headers.h
        int driver_id;
        struct nl_sock *socket = NULL;

        /*
         * We are sequencing each PReq frame.
         */

        // Sequence number of the current PReq frame.
        int seq_num = 0;
        // Sequence number expected in PRes frame.
        int ack_seq_num = -1;
        // Retries left for sending a scan request.
        // Each retry will occur after interval seconds.
        int scan_retries_left = 4;
        // Retries left for resending the PReq frame is wrong ACK is received.
        // Each retry will occur after interval seconds.
        int ack_retries_left = 4;

        // How much data we need to stuff inside PReq frames.
        int len_data_to_stuff = strlen(data_to_stuff);
        // How much data we have already stufed inside PReq frames.
        int len_data_already_stuffed = 0;
        /*
         * We can stuff max 1452 bytes of IEs in a PReq frame.
         *
         * Each IE can be max 256 bytes out of which 3 bytes are for
         * OUI (Organizationally Unique Identifier), 1 byte for vendor specific type,
         * and 252 bytes for data.
         *
         * IEs will be structured inside a PReq packet as follows:
         *
         * IE0 Data: Seq. Number | Timestamp | More Fragment Bit
         * IE1...N Data: Data
         *
         * We will reserve 9 bytes for Seq. Number, 19 bytes for Timestamp, and
         * 2 bytes for More Fragment Bit, i.e. a total of 4 (reserved) + 9 + 19 + 2 = 34 bytes.
         *
         * So, the actual IEs (IE1...N) that contain the stuffed data can be 1452 - 34 = 1418 bytes.
         * So, we need a min of floor(1418 / 256) + 1 = 6 IEs.
         *
         * So, the actual bytes availabe to stuff data = 1418 - (6 * 4) = 1394 bytes.
         */
        int ies_max_data_size = 1394;

        // Current Timestamp (in ms). This will be same for all the PReq frames.
        struct timeval data_origin_tv;
        gettimeofday(&data_origin_tv, NULL);
        unsigned int data_origin_ts = (data_origin_tv.tv_sec);
        unsigned int data_origin_ts_usec = (data_origin_tv.tv_usec);
        char data_origin_ts_str[20];
        sprintf(data_origin_ts_str, "%012u%06u", data_origin_ts, data_origin_ts_usec);
        printf("Timestamp (in ms): %s\n", data_origin_ts_str);
        printf("\n======================================\n");

        // Continue untill we have no more data to stuff.
        while (len_data_already_stuffed < len_data_to_stuff) {
	
		char *data;
		int data_size;
		// Index in our 'data_to_stuff' string from where we will start stuffing.
		int data_start_idx = seq_num * ies_max_data_size;
		// More fragment bit.
		int mfb;

		// Check if we have more than 1402 bytes of data to stuff.
		// If so, we will need more PReq frames to stuff the data, i.e. mfb = 1.
		if (len_data_to_stuff - len_data_already_stuffed > ies_max_data_size) {
		    data = (char *)calloc(ies_max_data_size + 1, sizeof(char *));
		    memcpy(data, &data_to_stuff[data_start_idx], ies_max_data_size);
		    data[ies_max_data_size] = '\0';
		    len_data_already_stuffed += ies_max_data_size;
		    mfb = 1;
		} else { // This is the last stuffed PReq frame we will send. mfb will be 0.
		    // Amount of data left to stuff.
		    int len_data_left = len_data_to_stuff - len_data_already_stuffed;
		    data = (char *)calloc(len_data_left + 1, sizeof(char *));
		    memcpy(data, &data_to_stuff[data_start_idx], len_data_left);
		    data[len_data_left] = '\0';
		    len_data_already_stuffed = len_data_to_stuff;
		    mfb = 0;
		}

		socket = init_socket();
		// Find the nl80211 driver ID.
		driver_id = genl_ctrl_resolve(socket, "nl80211");

		// Number of IEs we will need to stuff the data.
		int n_ies = get_n_ies_reqd(strlen(data));//headers.h
		struct ie_info **ies = (struct ie_info **) malloc((n_ies + 1) * sizeof(struct ie_info *));
		// Combined length of all IEs.
		size_t ies_len = 0;

		/*
		 * The first IE contains the sequence number, timestamp, and more fragment bit.
		 */

		// Sequence number.
		char seq_num_str[9];
		// More fragment bit.
		char mfb_str[2];
		// Additional data that we will send in the first IE.
		char additional_data[28];

		sprintf(seq_num_str, "%08d", seq_num);
		sprintf(mfb_str, "%d", mfb);
		strcpy(additional_data, seq_num_str);
		strcat(additional_data, data_origin_ts_str);
		strcat(additional_data, mfb_str);

		// Create the first IE.
		ies[0] = create_vendor_ie(additional_data);
		ies_len += ies[0]->ie_len;

		// Divide the data into 252 bytes chunks.
		char **raw_ies_data = split_data(data, n_ies);
		data_size = strlen(data);
		free(data);

		for (int i = 1; i < n_ies + 1; ++i) {
		    // Create a vendor specific IE corresponding to the data chunk.
		    ies[i] = create_vendor_ie(raw_ies_data[i-1]);
		    ies_len += ies[i]->ie_len;
		}

		// Freeing up.
		for (int i = 0; i < n_ies; ++i) {
		    free(raw_ies_data[i]);
		}
		free(raw_ies_data);

		// Append all the IEs into one string. We will pass this string to nl80211
		// and it will split them into IEs appropriately.
		unsigned char *ies_data = (unsigned char *)malloc(ies_len);
		int k = 0;

		for (int i = 0; i < n_ies + 1; ++i) {
		    for (int j = 0; j < ies[i]->ie_len; ++j) {
		        ies_data[k] = ies[i]->ie_data[j];
		        ++k;
		    }
		}

		while (1) {
		
		    /*
		     * Send stuffed probe request frame.
		     */
	
		    // Data transmission timestamp.
		    struct timeval data_tx_tv;
		    gettimeofday(&data_tx_tv, NULL);
		    unsigned int data_tx_ts = (data_tx_tv.tv_sec);
		    unsigned int data_tx_ts_usec = (data_tx_tv.tv_usec);

		    printf("\nData transmission timestamp (in ms): %u%06u", data_tx_ts, data_tx_ts_usec);

		    // Issue NL80211_CMD_TRIGGER_SCAN to the kernel and wait for it to finish.
		
		    int err = do_probe_stuffing(socket, interface_index, driver_id, ies_len, ies_data,chanNum);

		    if (err != 0) {  // Scanning failed.
		        printf("do_probe_stuffing() failed with %d.\n", err);
		        scan_retries_left--;

		        if (scan_retries_left > 0) {
			    driverError=driverError+1;
		            nl_socket_free(socket);
		            // //sleep for interval seconds before doing next scan.
		            printf("%d retries left. Will retry after %fs.\n", scan_retries_left,interval);
		            //QUESTION: if scanning is not successful even then we have to //sleep according to adaptive algorithm?  	
		            
		            //sleep(15);
		            usleep(interval);
		            // Re-initialize the socket.
		            socket = init_socket();
		        } else {
		            break;
		        }
		    } else {  // Scanning done.

			totalPacketsGenerated=totalPacketsGenerated+1;

		        scan_retries_left = 4;
		        // ////sleep(5);

		        /*
		         * Now get ACK for the stuffed probe request frames.
		         */

		        struct nl_msg *msg = nlmsg_alloc();
		        // Run NL80211_CMD_GET_SCAN command.
		        genlmsg_put(msg, 0, 0, driver_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
		        // Interface on which we scanned. Scan results will available on this interface only.
		        nla_put_u32(msg, NL80211_ATTR_IFINDEX, interface_index);
		        // Callbacks.
		        nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, get_ack, &ack_seq_num);

		        int ret = nl_send_auto(socket, msg);
		        ret = nl_recvmsgs_default(socket);
		        nlmsg_free(msg);

		        if (ret < 0) {  // Didn't receive an ACK.
			    packetLossCount=packetLossCount+1;
		            
			    printf("ERROR: nl_recvmsgs_default() returned %d (%s).\n", ret, nl_geterror(-ret));
		            log_info(wifi_interface, data_origin_ts, data_origin_ts_usec, data_origin_ts_str, data_tx_ts,
		                     data_tx_ts_usec, data_size, 5 - ack_retries_left, -1);
				
				
		            // Retry only if it is guaranteed delivery effort.
		            if (effort == 2) {
		                ack_retries_left--;

		                if (ack_retries_left > 0) {
		                    nl_socket_free(socket);
		                    // Wait for 15 seconds before sending new scan request.
		                    //printf("%d retries left. Will retry after %fs.\n", ack_retries_left,interval);
		                    printf("%d retries left. Will retry after 15s.\n", ack_retries_left);   
		                    //sleep(15);
		                    usleep(interval);
		                    socket = init_socket();
		                } else {
		                    break;
		                }
		            } else { // Don't retry for best delivery effort.
		                printf("Will not retry since it is best delivery effort...\n");
		                seq_num += 1;
		                break;
		            }
		        } else {  // Received an ACK.
		        
		            printf("Expected ACK: %d\n", seq_num);
		            printf("Received ACK: %d\n", ack_seq_num);
		            // Invalid ACK. Retry.
		            if (ack_seq_num == -1 || ack_seq_num != seq_num) {

				packetLossCount=packetLossCount+1;

		                log_info(wifi_interface, data_origin_ts, data_origin_ts_usec, data_origin_ts_str,
		                    data_tx_ts, data_tx_ts_usec, data_size, 5 - ack_retries_left, -1);
		                // Retry only if it is guaranteed delivery effort.
		                if (effort == 2) {
		                    ack_retries_left--;
		                    printf("Invalid received ACK.\n");
				

		                    if (ack_retries_left > 0) {
		                        nl_socket_free(socket);
		                        // Wait for 15 seconds before sending new scan request.
		                        //printf("%d retries left. Will retry after %fs.\n", ack_retries_left,interval);
		                  	
		                        printf("%d retries left. Will retry after 15s.\n", ack_retries_left);
		                        //sleep(15);
		                        usleep(interval);
		                        socket = init_socket();
		                    } else {
		                        break;
		                    }
		                } else { // Don't retry for best delivery effort.
		                    printf("Will not retry since it is best delivery effort...\n");
		                    seq_num += 1;
		                    break;
		                }
		            } else {  // Everything went well. Scan and ACK flow completed.

				successPacketcount=successPacketcount+1;

		                log_info(wifi_interface, data_origin_ts, data_origin_ts_usec, data_origin_ts_str,
		                    data_tx_ts, data_tx_ts_usec, data_size, 5 - ack_retries_left, 1);
		                
				printf("ACK received successfully.\n\nWaiting for %fs before sending more data.",interval);
				usleep(interval);
		                fflush(stdout);
		                seq_num += 1;
		                free(ies_data);
		                ack_retries_left = 4;
		                break;
		            }
		        }
		    }
		    
		    
		} //braces of while(1)

		if (scan_retries_left == 0) {
		    printf("No more retries left for scanning. Exiting...\n");
		    break;
		} else if (ack_retries_left == 0) {
		    printf("No more retries left for getting ACK from an AP. Exiting...\n");
		    break;
		} else { // Everything went well :)
		   if (len_data_already_stuffed != len_data_to_stuff) {
		    printf("\nGoing to //sleep for %fs before sending new scan request.\n",interval);
		    usleep(interval);
		   }	
		}

		nl_socket_free(socket);
		printf("\n");
		usleep(interval);
  	  }
  	  
  	  	

	//if(userInput!=1)
		free(data_to_stuff);
//printf("\nfree error\n");

 				while(arguments->resultCalculated!=1);//busy wait

				double channelUtilisationThreshold = 70.0; // to be calculated empirically
			
				// thread will calculate and return this:
				  currentTime = get_usecs();
				 double channelUtilisationPercentage = arguments->result->channel_Utilisation_Percentage;	 
				 double selectedChannel = arguments->result->channelNumber;
				 pthread_mutex_lock(lock);//t
				 //printf("\n%ld\n",currentTime);
				 double delay = secondsElapsed(threadTime,currentTime);//t
				 pthread_mutex_unlock(lock);//t			 
				interval = calculateInterval(channelUtilisationPercentage,channelUtilisationThreshold,interval,chanNum,selectedChannel);
			      if(selectedChannel>=1&&selectedChannel<=11)
			      	chanNum = selectedChannel;
				
				//sprintf(row,"%d,%lf,%lf\n",chanNum,channelUtilisationPercentage,interval);
				sprintf(row,"%s,%f,%f\n",arguments->result->buffer,interval);//t
				//printf("\n\nDEBUG %s\n\n",row);
				wstatus = write(fileDescriptor,row,strlen(row));
				//printf("\nreturn status of write %d\n",wstatus);
				//printf("\n\t\tinterval updated to %f\n ",interval);
				
				double stopAfterSeconds = 600.0;//  x/60 minutes 
			        currentTime = get_usecs();
				double duration = secondsElapsed(startTime,currentTime);
				
				if(duration>stopAfterSeconds)
				{
					printf("\nDuration: %f\n",duration);
					printf("\nTotal packets generated= %d\n",totalPacketsGenerated);					
					printf("\npacketSuccess= %d\n",successPacketcount);
					printf("\npacketLoss= %d\n",packetLossCount);
					printf("\ndriverError= %d\n",driverError);
					//signal the thread to shutdown;
					shutdownThread = 1;//should be locked?
					//wait for the thread to join:
					pthread_join(*thread,NULL);
					free(thread);
					free(results);
					free(arguments);
					close(fileDescriptor);
					return 0;
				}
       
   
//it will break the most outer loop. Main ends after this loop    	
  // userInput = 0;
    }
    close(fileDescriptor);		
    return 0;
}
