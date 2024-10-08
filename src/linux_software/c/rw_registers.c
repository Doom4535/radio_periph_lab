#include <stdio.h>
#include <sys/mman.h> 
#include <fcntl.h> 
#include <unistd.h>
#define _BSD_SOURCE

#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

#define RADIO_FIFO_RESET_OFFSET 0x18
#define RADIO_FIFO_OCCUPANCY_OFFSET 0x1c
#define RADIO_FIFO_READ_OFFSET 0x20
#define RADIO_FIFO_LENGTH_OFFSET 0x24
#define RADIO_FIFO_ADDRESS 0x43c10000

// the below code uses a device called /dev/mem to get a pointer to a physical
// address.  We will use this pointer to read/write the custom peripheral
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{

	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	volatile unsigned int *radio_base = (volatile unsigned int *)map_base; 
	return (radio_base);
}

int main()
{

// first, get a pointer to the peripheral base address using /dev/mem and the function mmap
    volatile unsigned int *my_periph = get_a_pointer(RADIO_PERIPH_ADDRESS);	
    printf("\r\n\r\n\r\nLab 6 Silly Register Read and Write Program\n\r");
    printf("Writing 4 registers in the periph with a counting pattern\n");
    my_periph[0] = 0; my_periph[1] = 1; my_periph[2] = 2; my_periph[3] = 3;
    for (int i = 0; i<8; i++)
    	printf("reading address %x = %x\n",RADIO_PERIPH_ADDRESS+4*i, my_periph[i]);
    printf("was that what you expected?\n");

    printf("now testing the fifo registers\r\n");
    volatile unsigned int *my_fifo = get_a_pointer(RADIO_FIFO_ADDRESS);
    unsigned int fifo_offsets[] = {RADIO_FIFO_RESET_OFFSET, RADIO_FIFO_OCCUPANCY_OFFSET, RADIO_FIFO_READ_OFFSET, RADIO_FIFO_LENGTH_OFFSET};
    for (int i = 0; i <4; i++){
	volatile unsigned int READ_ADDRESS = RADIO_FIFO_ADDRESS + fifo_offsets[i];
	printf("reading address %x = %x\r\n", READ_ADDRESS, my_fifo[READ_ADDRESS]);
    }
    return 0;
}
