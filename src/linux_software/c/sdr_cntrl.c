#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#define _BSD_SOURCE

#define RADIO_ADC_PINC_OFFSET 0
#define RADIO_TUNER_PINC_OFFSET 1
#define RADIO_CONTROL_REG_OFFSET 2
#define RADIO_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

#define RADIO_FIFO_RESET_OFFSET 0x18
#define RADIO_FIFO_OCCUPANCY_OFFSET 0x1c
#define RADIO_FIFO_READ_OFFSET 0x20
#define RADIO_FIFO_LENGTH_OFFSET 0x24
#define RADIO_FIFO_ADDRESS 0x43c10000

#define BROADCAST_ADDRESS "192.168.42.43"
//#define BROADCAST_ADDRESS "192.168.122.13"
#define BROADCAST_PORT 25344

#define RADIO_SAMPLE_RATE 126e6
#define RADIO_ADC_DDS_BITSIZE 27
#define RADIO_TUNER_DDS_BITSIZE 27

#define IQ_BUFFER_COUNT 5
//#define IQ_FRAME_SAMPLE_SIZE 4
#define IQ_FRAME_SAMPLE_SIZE 256
#define IQ_FRAME_BYTE_SIZE (IQ_FRAME_SAMPLE_SIZE *4 +2)

// the below code uses a device called /dev/mem to get a pointer to a physical
// address.  We will use this pointer to read/write the custom peripheral
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{

	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	volatile unsigned int *radio_base = (volatile unsigned int *)map_base; 
	return (radio_base);
}

void dds_tuner(float freq, volatile unsigned int* dds, unsigned int dds_bitsize, unsigned int dds_clk_rate){
	float phase_inc = freq * (float) (1 << dds_bitsize) / dds_clk_rate;
	*(dds) = (int) phase_inc;
}

void tune_adc(float freq){
	volatile unsigned int* adc_dds = get_a_pointer(RADIO_PERIPH_ADDRESS) +RADIO_ADC_PINC_OFFSET;
	dds_tuner(freq, adc_dds, RADIO_TUNER_DDS_BITSIZE, RADIO_SAMPLE_RATE);
}	

void tune_radio(float freq){
	volatile unsigned int* tuner_dds = get_a_pointer(RADIO_PERIPH_ADDRESS) +RADIO_TUNER_PINC_OFFSET;
	dds_tuner(freq, tuner_dds, RADIO_TUNER_DDS_BITSIZE, RADIO_SAMPLE_RATE);
}	


struct iq_buf {
	bool busy;
	bool full;
	uint16_t frame_idx;
	unsigned int idx;
	uint8_t buf[IQ_FRAME_BYTE_SIZE]; // We'll add the frame index first
};


struct iq_buf* get_buffer();
bool block_fill_buffer(unsigned int sample, struct iq_buf* buf);
void lock_fill_buffer(unsigned int sample);
void stream_ethernet();
void initialize_radio();
void read_radio();
void parse_console();
void draw_screen();


uint16_t iq_frame_idx = 0;
unsigned int loading_iq_buf = 0;
struct iq_buf iq_buffer[IQ_BUFFER_COUNT] = {{0}};

enum InputState {Menu = 0, Reading};
enum MenuState {None = 0, Address, Port, Tuning, ADC};
struct MachineState {
	bool initialized;
	bool redraw_screen;
	float adc_freq;
	float tuner_freq;
	unsigned int adc_pinc;
	unsigned int tuner_pinc;
	//int stream_ip;
	//int stream_port;
	struct sockaddr_in network_dest;
	bool stream_network;
	//volume
	//mute
	
	volatile unsigned int *radio_fifo;
	int network_fd;

	enum InputState input_state;
	enum MenuState menu_state;
	char menu_buffer[80]; // We'll allow up to 80 characters in a multi character entry
};
struct MachineState machine_state = {0}; 

void run_machine() {
	if(!machine_state.initialized) initialize_radio();

	int n = 0;
	//read_radio();
	if (machine_state.redraw_screen) draw_screen();
	if (machine_state.stream_network) stream_ethernet();
	//if (ioctl(stdin, FIONREAD, &n) == 0 && n > 0) parse_console();
	if (ioctl(0, FIONREAD, &n) == 0 && n > 0) parse_console();
}

int main() {
	
	while(1) {
		run_machine();
	}

	close(machine_state.network_fd);

	return 0;
}

void stream_ethernet(void) {
	struct iq_buf* b = NULL;
	for (int i = 0; i < IQ_BUFFER_COUNT; i++) {
		struct iq_buf* gb = &iq_buffer[i];
		if (gb->full = true && gb->busy == false ){
			gb->busy = true;
			if (b == NULL) {
				b = gb;
			} 
			// Sorta handle overflows by only updating if the difference is small
			else if (gb->frame_idx - b->frame_idx < 5) {
				b->busy = false;
				b = gb;
			}
		}
	}
	// Send the frame and clear/reset the buffer
	if (b != NULL) {
		if (sendto(machine_state.network_fd, b->buf, sizeof(b->buf), 0, (struct sockaddr*)&machine_state.network_dest, sizeof(machine_state.network_dest)) < 0) {
			perror("cannot send message");
			close(machine_state.network_fd);
			exit -1;
		} else {
			b->full = false;
			b->frame_idx = 0;
			b->idx = 0;
			b->busy = false;
		}
	}
}

void read_radio() {
	// maybe update to use the full value of the interrupt register and change when it reports full
	// Possibly also use a wider fifo to use a single read?
	//if (my_fake_fifo++ >= IQ_FRAME_SAMPLE_SIZE) {
	//	my_fake_fifo = 0;
	volatile unsigned int* my_fifo = get_a_pointer(RADIO_FIFO_ADDRESS);
	if (my_fifo[RADIO_FIFO_OCCUPANCY_OFFSET /4] >= IQ_FRAME_SAMPLE_SIZE) {
		bool buf_full = false;
		struct iq_buf* buf = NULL;
		buf = get_buffer(); // We should probably try to do this before?
		//printf("Weee\r\n");
		for(int i = 0; i < IQ_FRAME_SAMPLE_SIZE; i++) {
			//buf_full = block_fill_buffer(my_fake_sample++, buf);
			buf_full = block_fill_buffer(my_fifo[RADIO_FIFO_READ_OFFSET / 4], buf);
			//printf("%x\r\n", my_fake_sample);
		}
		if (buf_full) {
			stream_ethernet();
		}
		buf = get_buffer(); // Get a new buffer here
	}
	//else {
	//    int fifo_occupancy = my_fifo[RADIO_FIFO_OCCUPANCY_OFFSET /4];
	//    printf("I think the buffer occupancy is %x or %d in decimal\r\n", fifo_occupancy, fifo_occupancy);
	//}
}

void initialize_radio(){
	machine_state.radio_fifo = get_a_pointer(RADIO_FIFO_ADDRESS);
	machine_state.network_fd = socket(AF_INET,SOCK_DGRAM,0);
	if(machine_state.network_fd<0){
		perror("cannot open socket");
		exit -1;
	}
	bzero(&machine_state.network_dest,sizeof(machine_state.network_dest));
	machine_state.network_dest.sin_family = AF_INET;
	machine_state.network_dest.sin_addr.s_addr = inet_addr(BROADCAST_ADDRESS);
	machine_state.network_dest.sin_port = htons(BROADCAST_PORT);
	machine_state.stream_network = true;
	machine_state.redraw_screen = true;

	machine_state.initialized = true;
}

void draw_screen(){
	machine_state.redraw_screen = false;
	system("clear");
	printf("Welcome to Lab7 by: Aaron Covrig\r\n");
	printf("Current Status:\r\n");
	printf("\tEthernet Streaming: %s\r\n", machine_state.stream_network ? "True" : "False");
	printf("\t\tDestination Address %s:%d\r\n", "TODO", 69);
	printf("\tADC Frequency: %f, Phase Increment: %x\r\n", 69.0, 0x0);
	printf("\tTuner Frequency: %f, Phase Increment: %x\r\n", 69.0, 0x0);
	printf("Input Options:\r\n");
	printf("\tA,a:\tSet ethernet stream address\r\n");
	printf("\tP,p:\tSet ethernet stream port\r\n");
	printf("\tF,f:\tSet the ADC frequency\r\n");
	printf("\tT,t:\tSet the Tuner frequency\r\n");
	printf("\tU:\tIncrement the Tuner frequency by 1000 Hertz\r\n");
	printf("\tu:\tIncrement the Tuner frequency by 100 Hertz\r\n");
	printf("\tD:\tDecrement the Tuner frequency by 1000 Hertz\r\n");
	printf("\td:\tDecrement the Tuner frequency by 100 Hertz\r\n");
}

void parse_console(){
	// Process user inputs
	int recv_char = fgetc(stdin);
	if (recv_char == '\r' || recv_char == '\n') {
		// do nothing
	} else {
		switch(recv_char) {
			case '+':
				// Not supported
				break;
			case '-':
				// Not supported
				break;
			case 'A': /* Fall through */
			case 'a':
				break;
			case 'P': /* Fall through */
			case 'p':
				break;
			case 'F': /* Fall through */
			case 'f': 
			{
				break;
			}
			case 'T': /* Fall through */
			case 't':
			{
				break;
			}
			case 'U':
				break;
			case 'u':
				break;
			case 'D':
				break;
			case 'd':
				break;
			case 'S': /* Fall through */
			case 's':
				break;
			case 'm':
				break;
			case 'Z': /* Fall through */
			case 'z':
				break;
			case 'H': /* Fall through */
			case 'h':
				break;
			default:
				break;
		
		}
		machine_state.redraw_screen = true;
	}
}

struct iq_buf* get_buffer() {
	// We can have issues if all buffers are in use/full (aka, make sure to empty them)
	struct iq_buf* buf = &iq_buffer[loading_iq_buf];
	if (buf->busy == false && buf->full == false) {
		buf->busy = true;
		// Preallocate the frame index
		if (buf->frame_idx == 0) {
			buf->frame_idx = iq_frame_idx++;
			memcpy(&buf->buf[0], &buf->frame_idx, sizeof(buf->frame_idx));
			buf->idx = sizeof(buf->frame_idx);
			//printf("New frame %d\r\n", buf->frame_idx);
		}
	} else {
		loading_iq_buf = (loading_iq_buf +1) % IQ_BUFFER_COUNT;
		buf = get_buffer();
		// This was fun, but we should probably go to something that
		// makes it easier for us to drop data if the buffer is full
	}	
	if (buf != NULL) {
		return buf;
	}
	return NULL;
}

void lock_fill_buffer(unsigned int sample) {
	struct iq_buf* buf = get_buffer();
	if (buf == NULL) {
		perror("Received NULL pointer buffer");
		exit -1;
	}
	if (buf->idx >= IQ_FRAME_BYTE_SIZE) {
		buf->full = true;
		buf->busy = false;
		buf = get_buffer();
	}
	memcpy(&buf->buf[buf->idx], &sample, sizeof(sample));
	buf->idx += 4;
	if (buf->idx >= IQ_FRAME_BYTE_SIZE) {
		buf->full = true;
	}
	buf->busy = false;
	// Maybe put debug code here to check if buffers are filling
	// faster than we can send them
	// We should also profile this, a simple buffer is probably
	// technically better (but far less exciting)
}

bool block_fill_buffer(unsigned int sample, struct iq_buf* buf) {
	memcpy(&buf->buf[buf->idx], &sample, sizeof(sample));
	buf->idx += 4;
	if (buf->idx >= IQ_FRAME_BYTE_SIZE) {
		buf->full = true;
		buf->busy = false;
	}
	// Maybe put debug code here to check if buffers are filling
	// faster than we can send them
	// We should also profile this, a simple buffer is probably
	// technically better (but far less exciting)

	return buf->full;
}
