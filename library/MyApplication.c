#include<headers.h>
 
int main(){

set_wifi_interface("wlp19s0"); // add the name of your interface
set_effort(1);
set_probe_tx_interval(1.0);
set_data_to_stuff_size_limit(200);

start_probe_stuffing(1,"test.txt")

return 0;
}
