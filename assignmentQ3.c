/*
 *  C code to demonstrate control of the LED matrix for the
 *  Raspberry Pi Sense HAT add-on board.
 *
 *  Uses the mmap method to map the led device into memory
 *
 *  Build with:  gcc -Wall -O2 led_matrix.c -o led_matrix
 *
 *  Tested with:  Raspbian GNU/Linux 10 (buster) / Raspberry Pi 3 B+
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#define FILEPATH "/dev/fb1"
#define NUM_WORDS 64
#define FILESIZE (NUM_WORDS * sizeof(uint16_t))

#define R 0xF800
#define G 0x07E0
#define B 0x001F
#define W 0xFFFF

void delay(int);
uint16_t colorSet(int choice);

int main(void)
{
    int i, choice;
    int fbfd;
    uint16_t *map;
    uint16_t *p;
    struct fb_fix_screeninfo fix_info;
    printf("COLOR SETTER\n");
    printf("1. Red\n");
    printf("2. Green\n");
    printf("3. Blue\n");
    printf("4. White\n");    
    printf("Select Color:");
    scanf("%d", &choice);
    uint16_t N = colorSet(choice);
    uint16_t array[64] =    {0,0,0,N,N,0,0,0,
                             0,0,N,N,N,N,0,0,
                             0,N,N,0,0,N,N,0,
                             N,N,0,0,0,0,N,N,
                             N,N,N,N,N,N,N,N,
                             N,0,0,0,0,0,0,N,
                             N,0,0,0,0,0,0,N,
                             N,0,0,0,0,0,0,N};

    /* open the led frame buffer device */
    fbfd = open(FILEPATH, O_RDWR);
    if (fbfd == -1) {
	perror("Error (call to 'open')");
	exit(EXIT_FAILURE);
    }

    /* read fixed screen info for the open device */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) == -1) {
	perror("Error (call to 'ioctl')");
	close(fbfd);
	exit(EXIT_FAILURE);
    }

    /* now check the correct device has been found */
    if (strcmp(fix_info.id, "RPi-Sense FB") != 0) {
	printf("%s\n", "Error: RPi-Sense FB not found");
	close(fbfd);
	exit(EXIT_FAILURE);
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (map == MAP_FAILED) {
	close(fbfd);
	perror("Error mmapping the file");
	exit(EXIT_FAILURE);
    }

    /* set a pointer to the start of the memory area */
    p = map;

    /* clear the led matrix */
    memset(map, 0, FILESIZE);

    /* light it up! */
    for (i = 0; i < NUM_WORDS; i++) {
	*(p + i) = array[i];
	// delay(0);
    }
    delay(1000);

    /* flash white */
    // for (i = 0; i < 3; i++) {
	// delay(250);
	// memset(map, 0xFF, FILESIZE);
	// delay(250);
	// memset(map, 0, FILESIZE);
    // }
    // delay(250);

    /* clear the led matrix */
    memset(map, 0, FILESIZE);

    /* un-map and close */
    if (munmap(map, FILESIZE) == -1) {
	perror("Error un-mmapping the file");
    }
    close(fbfd);

    return 0;
}

void delay(int t)
{
    usleep(t * 1000);
}

uint16_t colorSet(int choice)
{
    uint16_t n;
    if(choice == 1)
    {
        n = R;
    }
    else if(choice == 2)
    {
        n = G;
    }
    else if(choice == 3)
    {
        n = B;
    }
    else if(choice == 4)
    {
        n = W;
    }
    else
    {
        n = 0;
    }
    
    return n;
}