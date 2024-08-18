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
//#include <linux/i2c-dev.h>
//#include <i2c/smbus.h>
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

// If we add I2C support
//get offsets from vitis SDK in Lab3
#define I2C_PERIPH_ADDRESS 0x41600000
#define SSM2603_ADDR	0b0011010

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
	close(mem_fd);
	return (radio_base);
}


int dds_phase_inc(float freq, unsigned int dds_bitsize, unsigned int dds_clk_rate);
void tune_adc(float freq);
void tune_radio(float freq);
int append_sample(uint32_t sample);
void send_frame(int idx);
//void stream_ethernet();
void initialize_radio();
void read_radio();
void execute_buffer();
void parse_console();
void draw_screen();
void configure_network();

uint16_t frame_counter = 0;
int frame_idx = 0;
int frame_byte_idx = 0;
uint8_t frame_byte_buf[IQ_BUFFER_COUNT * IQ_FRAME_BYTE_SIZE] = {0};

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
	
	volatile unsigned int* radio_peripheral;
	volatile unsigned int *radio_fifo;
	int network_fd;

	char menu_buffer[80]; // We'll allow up to 80 characters in a multi character entry
};
struct MachineState machine_state = {0}; 

int dds_phase_inc(float freq, unsigned int dds_bitsize, unsigned int dds_clk_rate){
	float phase_inc = freq * (float) (1 << dds_bitsize) / dds_clk_rate;
	return (int) phase_inc;
}

void tune_adc(float freq){
	volatile unsigned int* adc_dds = machine_state.radio_peripheral +RADIO_ADC_PINC_OFFSET;
	int pinc = dds_phase_inc(freq, RADIO_ADC_DDS_BITSIZE, RADIO_SAMPLE_RATE);
	*(adc_dds) = pinc;
	machine_state.adc_freq = freq;
	machine_state.adc_pinc = pinc;
}	

void tune_radio(float freq){
	volatile unsigned int* tuner_dds = machine_state.radio_peripheral +RADIO_TUNER_PINC_OFFSET;
	int pinc = dds_phase_inc(freq, RADIO_TUNER_DDS_BITSIZE, RADIO_SAMPLE_RATE);
	*(tuner_dds) = pinc;
	machine_state.tuner_freq = freq;
	machine_state.tuner_pinc = pinc;
}	

void run_machine() {
	if(!machine_state.initialized) initialize_radio();

	int n = 0;
	read_radio();
	if (machine_state.redraw_screen) draw_screen();
	//if (machine_state.stream_network) stream_ethernet(); // maybe doesn't handle not yet full well
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

// We don't currently support detecting full buffers
//void stream_ethernet(void) {}

int fake_fifo = 0;
int fake_sample = 0;
void read_radio() {
	// maybe update to use the full value of the interrupt register and change when it reports full
	// Possibly also use a wider fifo to use a single read?
	//if (fake_fifo++ >= IQ_FRAME_SAMPLE_SIZE) {
	//	fake_fifo = 0;
	if (machine_state.radio_fifo[RADIO_FIFO_OCCUPANCY_OFFSET /4] >= IQ_FRAME_SAMPLE_SIZE) {
		int idx = -1;
		//printf("Created buffer b->frame_idx: %u, global idx: %u", iq_frame_idx);
		for(int i = 0; i < IQ_FRAME_SAMPLE_SIZE; i++) {
			idx = append_sample(machine_state.radio_fifo[RADIO_FIFO_READ_OFFSET / 4]);
			//status = append_sample(fake_sample++);
		}
		if (idx >= 0) {
			if(machine_state.stream_network){
				send_frame(idx);
			}
		}
	}
}

void initialize_radio(){
	machine_state.radio_peripheral = get_a_pointer(RADIO_PERIPH_ADDRESS);
	machine_state.radio_fifo = get_a_pointer(RADIO_FIFO_ADDRESS);

	bzero(&machine_state.network_dest,sizeof(machine_state.network_dest));
	machine_state.network_dest.sin_family = AF_INET;
	inet_pton(AF_INET, BROADCAST_ADDRESS, &machine_state.network_dest.sin_addr.s_addr);
	//machine_state.network_dest.sin_addr.s_addr = inet_addr(BROADCAST_ADDRESS);
	machine_state.network_dest.sin_port = htons(BROADCAST_PORT);
	machine_state.redraw_screen = true;

	system("stty raw"); // set terminal to raw mode for our TUI

	machine_state.initialized = true;
}

void configure_network(){
	if (!machine_state.stream_network) {
		// Create the network connection
		machine_state.network_fd = socket(AF_INET,SOCK_DGRAM,0);
		if(machine_state.network_fd<0){
			perror("cannot open socket");
			exit(-1);
		}
	} else {
		// Close the network connection
		close(machine_state.network_fd);
	}
	machine_state.stream_network = !machine_state.stream_network;
}

void draw_screen(){
	machine_state.redraw_screen = false;
	system("clear");
	printf("Welcome to Lab7 by: Aaron Covrig\r\n");
	printf("Current Status:\r\n");
	printf("\tEthernet Streaming: %s\r\n", machine_state.stream_network ? "True" : "False");
	//printf("\t\tDestination Address %s\r\n", inet_ntop(AF_INET, machine_state.network_dest)); //inet_ntoa is deprecated
	printf("\t\tDestination Address %s:%i\r\n", inet_ntoa(machine_state.network_dest.sin_addr), ntohs(machine_state.network_dest.sin_port));
	printf("\tADC Frequency: %f, Phase Increment: %x\r\n", machine_state.adc_freq, machine_state.adc_pinc);
	printf("\tTuner Frequency: %f, Phase Increment: %x\r\n", machine_state.tuner_freq, machine_state.tuner_pinc);
	printf("Input Options:\r\n");
	printf("\tA,a:\tSet ethernet stream address\r\n");
	printf("\tP,p:\tSet ethernet stream port\r\n");
	printf("\tN,n:\tToggle network streaming of IQ samples\r\n");
	printf("\tF,f:\tSet the ADC frequency\r\n");
	printf("\tT,t:\tSet the Tuner frequency\r\n");
	printf("\tU:\tIncrement the Tuner frequency by 1000 Hertz\r\n");
	printf("\tu:\tIncrement the Tuner frequency by 100 Hertz\r\n");
	printf("\tD:\tDecrement the Tuner frequency by 1000 Hertz\r\n");
	printf("\td:\tDecrement the Tuner frequency by 100 Hertz\r\n");
	printf("\tctrl-d\tTerminate the Program\r\n");
	printf("\r\n");
	printf("\r\n");
	printf("Execution:\r\n");
	printf("\tCommand Buffer: %s\r\n", machine_state.menu_buffer);
	printf("\tCommand History:\r\n");
	printf("\t\t%s\r\n", "TODO");
}

void execute_buffer(){
	// Maybe update by adding a wrapper around number inputs to handle different bases
	// Maybe update to allow chaining commands (aka, process substrings)
	char cmd = machine_state.menu_buffer[0];
	int cmd_length = strnlen(machine_state.menu_buffer, sizeof(machine_state.menu_buffer));
	char *eptr = &machine_state.menu_buffer[cmd_length];
	char *cmd_value = &machine_state.menu_buffer[1];
	switch(cmd){
		// Maybe add support ip:port notation
		case 'A': /* Fall through */
		case 'a':
		{
			inet_pton(AF_INET, cmd_value, &machine_state.network_dest.sin_addr.s_addr);
			break;
		}
		case 'P': /* Fall through */
		case 'p':
		{
			int port = (int) strtol(cmd_value, &eptr, 10);
			machine_state.network_dest.sin_port = htons(port);
			break;
		}
		case 'F': /* Fall through */
		case 'f':
		{
			float freq = (float) strtod(cmd_value, &eptr);
			tune_adc(freq);
			break;
		}
		case 'T': /* Fall through */
		case 't':
		{
			float freq = (float) strtod(cmd_value, &eptr);
			tune_radio(freq);
			break;
		}
	}
	bzero(&machine_state.menu_buffer, sizeof(machine_state.menu_buffer)); // Clear the buffer
}

void parse_console(){
	// Process user inputs
	int input_val = fgetc(stdin); // Maybe add EOF and other checks
	char recv_char = (char) input_val;
	switch(recv_char) {
		// Execute non-immediate command
		case '\r': /* Fall through */
		case '\n':
			// Execute buffer
			execute_buffer();
			break;
		// Increment volume (only to audio output)
		case '+':
			// Not supported
			break;
		// Decrement volume (only to audio output)
		case '-':
			// Not supported
			break;
		// Set network stream destination address
		case 'A': /* Fall through */
		case 'a':
			strncat(machine_state.menu_buffer, &recv_char, sizeof(recv_char));
			break;
		// Set network stream destination port
		case 'P': /* Fall through */
		case 'p':
			strncat(machine_state.menu_buffer, &recv_char, sizeof(recv_char));
			break;
		// Toggle network streaming (requires network destination)
		case 'N': /* Fall through */
		case 'n':
			configure_network();
			break;
		// Set ADC frequency
		case 'F': /* Fall through */
		case 'f': 
		{
			strncat(machine_state.menu_buffer, &recv_char, sizeof(recv_char));
			break;
		}
		// Tune Radio (Set radio channel frequency)
		case 'T': /* Fall through */
		case 't':
		{
			strncat(machine_state.menu_buffer, &recv_char, sizeof(recv_char));
			break;
		}
		// Increment
		case 'U':
			tune_radio(machine_state.tuner_freq +1000);
			break;
		case 'u':
			tune_radio(machine_state.tuner_freq +100);
			break;
		// Decrement
		case 'D':
			tune_radio(machine_state.tuner_freq -1000);
			break;
		case 'd':
			tune_radio(machine_state.tuner_freq -100);
			break;
		// Maybe
		case 'S': /* Fall through */
		case 's':
			break;
		// Mute
		case 'm':
			break;
		// Zero/Reset
		case 'Z': /* Fall through */
		case 'z':
			break;
		// Help menu
		case 'H': /* Fall through */
		case 'h':
			break;
		// Load numbers
		case '9': /* Fall through */
		case '8': /* Fall through */
		case '7': /* Fall through */
		case '6': /* Fall through */
		case '5': /* Fall through */
		case '4': /* Fall through */
		case '3': /* Fall through */
		case '2': /* Fall through */
		case '1': /* Fall through */
		case '0': /* Fall through */
		case '.': /* Fall through */
		case ',': 
			strncat(machine_state.menu_buffer, &recv_char, sizeof(recv_char));
			break;
		// Remove an entry from the buffer
		case '\b':   /* Fall through */
		case '\x7f':
			machine_state.menu_buffer[strnlen(machine_state.menu_buffer, sizeof(machine_state.menu_buffer)) -1] = '\0';
			break;
		// Exit (CTRL-D)
		case '\04':
			system("stty cooked"); // Return to normal shell behavior
			printf("Goodbye\r\n");
			exit(0);
			break;
		default:
			break;
	}
	machine_state.redraw_screen = true;
}

int append_sample(uint32_t sample){
	// Check if new frame (add frame index if it is)
	int write_idx = frame_byte_idx + (IQ_FRAME_BYTE_SIZE * frame_idx);
	if (frame_byte_idx == 0){
		memcpy(&frame_byte_buf[write_idx], &frame_counter, sizeof(frame_counter));
		frame_byte_idx +=2;
	}
	// Write sample into buffer
	write_idx = frame_byte_idx + (IQ_FRAME_BYTE_SIZE * frame_idx);
	memcpy(&frame_byte_buf[write_idx], &sample, sizeof(sample));
	//uint8_t sample_bytes = *((uint8_t *) &sample + 0);
	//volatile unsigned int* adc_dds = machine_state.radio_peripheral +RADIO_ADC_PINC_OFFSET;
	//uint8_t flipped[4] = {0};
	//uint8_t *sample_bytes = &sample;
	//memcpy(&flipped[0], sample_bytes +1, 1);
	//memcpy(&flipped[1], sample_bytes +0, 1);
	//memcpy(&flipped[2], sample_bytes +3, 1);
	//memcpy(&flipped[3], sample_bytes +2, 1);
	//uint32_t flipped_sample = 0;
	//memcpy(&flipped_sample, &flipped, sizeof(flipped));
	//memcpy(&frame_byte_buf[write_idx], &flipped_sample, sizeof(flipped_sample));
	//printf("Sample: %x, Flipped: %x", sample, flipped_sample);
	frame_byte_idx +=4;
	// Check if we have finished a frame
	if (frame_byte_idx >= IQ_FRAME_BYTE_SIZE){
		frame_byte_idx = 0;
		int finished_frame_idx = frame_idx++;
		if (frame_idx >= IQ_BUFFER_COUNT){
			frame_idx = 0;
		}
		//return true
		return finished_frame_idx;
	}
	// Default to not finished
	//return false;
	return -1;
}

void send_frame(int idx) {
	if (idx >= 0 && machine_state.stream_network) {
		uint8_t *frame_start_idx = &frame_byte_buf[idx * IQ_FRAME_BYTE_SIZE];
		//printf("Sending frame: %u, index val: %u\r\n", (uint16_t) frame_start_idx, idx);
		if (sendto(machine_state.network_fd, frame_start_idx, IQ_FRAME_BYTE_SIZE, 0, (struct sockaddr*)&machine_state.network_dest, sizeof(machine_state.network_dest)) < 0) {
			perror("cannot send message");
			close(machine_state.network_fd);
			exit(-1);
		}
	}
}
