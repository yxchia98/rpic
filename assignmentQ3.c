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
#define Y 0xFFE0
#define BK 0x0000

void delay(int);
void colorSet(int choice, uint16_t *n);
// uint16_t letterA[64], letterB[64], letterC[64], letterD[64], letterE[64], letterF[64], letterG[64], letterH[64], letterI[64], letterJ[64], letterK[64], letterL[64], letterM[64], letterN[64], letterO[64], letterP[64], letterQ[64], letterR[64], letterS[64], letterT[64], letterU[64], letterV[64], letterW[64], letterX[64], letterY[64], letterZ[64], heart[64], singapore[64], smile[64];
void setColor(uint16_t N, uint16_t (*letter_ptr)[64]);
void light_it_up(uint16_t *ptr, uint16_t letter[26][64], int letterValue);
void selectColor(uint16_t *ptr, uint16_t *N, uint16_t letter[26][64]);
void displayText(uint16_t *p, uint16_t letter[26][64], char message[100], char ch);
int squareGame(int squareLength, int i, uint16_t *ptr, uint16_t N, uint16_t *map);
char changeUpperCase(char letter);
int main(void)
{
    char message[100] = {}, ch;
    int i, choice;
    int fbfd;
    uint16_t *map;
    uint16_t *p;
    uint16_t N = W;
    uint16_t letter[26][64] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0},
                               {0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, N, N, N, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, N, N, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, N, 0, N, 0, N, 0, N, 0, 0, N, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, 0, 0, N, 0, 0, 0, N, 0, N, 0, N, 0, 0, 0, N, 0, 0, N, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, N},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, N, 0, 0, N, 0, 0, N, 0, N, 0, N, 0, 0, 0, N, 0, N, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    struct fb_fix_screeninfo fix_info;
    // scanf("%d", &choice);
    // Nchoice = colorSet(choice);

    fbfd = open(FILEPATH, O_RDWR);
    if (fbfd == -1)
    {
        perror("Error (call to 'open')");
        exit(EXIT_FAILURE);
    }

    /* read fixed screen info for the open device */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) == -1)
    {
        perror("Error (call to 'ioctl')");
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    /* now check the correct device has been found */
    if (strcmp(fix_info.id, "RPi-Sense FB") != 0)
    {
        printf("%s\n", "Error: RPi-Sense FB not found");
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (map == MAP_FAILED)
    {
        close(fbfd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    /* set a pointer to the start of the memory area */
    p = map;

    /* clear the led matrix */
    memset(map, 0, FILESIZE);
    //MAIN MENU, TO BE BRANCHED TO SUB MENUS, ETC
    while (choice != 6)
    {
        printf("MAIN MENU\n1. Change Color\n2. Edit Matrix\n3. Change Display Style\n4. Display Message\n5. Test Game\n6. Exit\nEnter Selection:");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            selectColor(p, &N, letter);
        case 2:
            break;
        case 3:
            break;
        case 4:
            displayText(p, letter, message, ch);
        case 5:
            squareGame(8, 1, p, N, map);
        case 6:
            break;
        }
    }

    /* clear the led matrix */
    memset(map, 0, FILESIZE);

    /* un-map and close */
    if (munmap(map, FILESIZE) == -1)
    {
        perror("Error un-mmapping the file");
    }
    close(fbfd);

    return 0;
}

void delay(int t)
{
    usleep(t * 1000);
}

void colorSet(int choice, uint16_t *n)
{
    if (choice == 1)
    {
        *n = R;
    }
    else if (choice == 2)
    {
        *n = G;
    }
    else if (choice == 3)
    {
        *n = B;
    }
    else if (choice == 4)
    {
        *n = W;
    }
    else
    {
    }
}

void setColor(uint16_t N, uint16_t (*letter_ptr)[64])
{
    for (int i = 0; i < 26; i++)
    {
        for (int k = 0; k < 64; k++)
        {
            if (letter_ptr[i][k] != 0)
            {
                letter_ptr[i][k] = N;
            }
        }
    }
    // letter[0][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0};
    // letter[1][64] = {0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, N, N, N, 0, 0};
    // letter[2][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0};
    // letter[3][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[4][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[5][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[6][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, N, N, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[7][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[8][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[9][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[10][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[11][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[12][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, N, 0, N, 0, N, 0, N, 0, 0, N, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[13][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, N, 0, 0, N, 0, 0, 0, N, 0, N, 0, N, 0, 0, 0, N, 0, 0, N, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[14][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[15][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[16][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, N};
    // letter[17][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[18][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[19][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[20][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[21][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[22][64] = {0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, N, 0, N, 0, 0, N, 0, 0, N, 0, 0, N, 0, N, 0, N, 0, 0, 0, N, 0, N, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[23][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[24][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, N, 0, 0, 0, 0, N, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    // letter[25][64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // heart[64] = {
    //     0, 0, 0, 0, 0, 0, 0, 0,
    //     0, N, N, 0, 0, N, N, 0,
    //     N, N, N, N, N, N, N, N,
    //     N, N, N, N, N, N, N, N,
    //     0, N, N, N, N, N, N, 0,
    //     0, 0, N, N, N, N, 0, 0,
    //     0, 0, 0, N, N, 0, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0};

    // singapore[64] = {
    //     0, 0, 0, 0, 0, 0, 0, 0,
    //     0, N, N, 0, 0, N, N, 0,
    //     N, 0, 0, 0, N, 0, 0, N,
    //     N, N, 0, 0, N, 0, 0, 0,
    //     0, 0, N, 0, N, 0, N, N,
    //     N, 0, N, 0, N, 0, 0, N,
    //     0, N, 0, 0, 0, N, N, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0};

    // smile[64] = {
    //     0,
    //     0,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     0,
    //     0,
    //     0,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     0,
    //     Y,
    //     W,
    //     BK,
    //     Y,
    //     Y,
    //     W,
    //     BK,
    //     Y,
    //     Y,
    //     BK,
    //     W,
    //     Y,
    //     Y,
    //     BK,
    //     W,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     BK,
    //     W,
    //     W,
    //     BK,
    //     Y,
    //     Y,
    //     0,
    //     Y,
    //     Y,
    //     R,
    //     R,
    //     Y,
    //     Y,
    //     0,
    //     0,
    //     0,
    //     Y,
    //     Y,
    //     Y,
    //     Y,
    //     0,
    //     0,
    // };
}
void light_it_up(uint16_t *ptr, uint16_t letter[26][64], int letterValue)
{
    /* light it up! */
    for (int i = 0; i < NUM_WORDS; i++)
    {
        *(ptr + i) = letter[letterValue][i];
    }
    delay(500);
    // for (int i = 0; i < 3; i++)
    // {
    //     delay(250);
    //     memset(p, 0, FILESIZE);
    //     delay(250);
    //     memset(p, 0, FILESIZE);
    // }
    // delay(250);

    // /* light it up! */
    // for (i = 0; i < NUM_WORDS; i++)
    // {
    //     *(p + i) = heart[i];
    //     //delay(25);
    // }
    // delay(500);

    // /* light it up! */
    // for (i = 0; i < NUM_WORDS; i++)
    // {
    //     *(p + i) = singapore[i];
    //     //delay(25);
    // }
    // delay(500);

    // /* light it up! */
    // for (i = 0; i < NUM_WORDS; i++)
    // {
    //     *(p + i) = smile[i];
    //     //delay(25);
    // }
    // delay(5000);
}

void selectColor(uint16_t *ptr, uint16_t *N, uint16_t letter[26][64])
{
    int i, choice = 0;
    printf("COLOR SETTER\n1. Red\n2. Green\n3. Blue\n4. White\n5. Exit\n");
    while (choice != 5)
    {
        printf("Select color:");
        scanf("%d", &choice);
        colorSet(choice, N);
        setColor(*N, letter);
        for (i = 0; i < NUM_WORDS; i++)
        {
            *(ptr + i) = letter[0][i];
        }
    }
    printf("Color changed to:0x%04X\n", *N);
}

void displayText(uint16_t *p, uint16_t letter[26][64], char message[100], char ch)
{
    int option = 0;
    printf("Display Message\nPress 2. Exit\n");
    fgetc(stdin);
    while (option != 2)
    {
        //clear array
        memset(message, 0, 100);
        printf("\nEnter alphabetic message: ");
        fgets(message, 100, stdin);
        printf("first length of message %d\n", strlen(message));
        if (strlen(message) != 1)
        {
            if (message[0] == 50)
            {
                option = 2;
            }
            else
            {
                int lengthOfMessage = strlen(message) - 1;
                int arr_length = lengthOfMessage * 10;
                uint16_t Choosenletter[8][1000] = {};
                int count = 0;
                uint16_t zero = 0;
                for (int i = 0; i < lengthOfMessage; i++)
                {
                    count = 0;
                    message[i] = changeUpperCase(message[i]);
                    int letterValue = message[i] - 65;
                    int spacing = i * 10;
                    //to set the array of each row of display
                    //compile letters together
                    for (int k = 0; k < 64; k += 8)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            Choosenletter[count][j + spacing] = letter[letterValue][j + k];
                        }
                        for (int j = 8; j < 10; j++)
                        {
                            Choosenletter[count][spacing + j] = zero;
                        }
                        count++;
                    }
                }
                int number = 0;
                //sliding animation
                for (int m = 0; m < arr_length; m++)
                {
                    count = 0;
                    for (int k = 0; k < 8; k++)
                    {
                        for (int l = 0; l < 8; l++)
                        {
                            *(p + count + l) = Choosenletter[k][l + m];
                        }
                        count += 8;
                    }
                    number++;
                    delay(300);
                    memset(p, 0, FILESIZE);
                }
            }
        }
    }
}
char changeUpperCase(char letter)
{
    //Change lowercase's value to uppercase
    if (letter >= 'a' && letter <= 'z')
    {
        letter -= 32;
        return letter;
    }
}

int squareGame(int squareLength, int i, uint16_t *ptr, uint16_t N, uint16_t *map)
{
    int option = 0;
    int n = squareLength * squareLength;
    int result, number = i;
    char ch;

    printf("Enter 2 to Exit\n");

    while (option != 2)
    {
        printf("\nEnter your direction: ");
        //fgets(message, 100, stdin);
        scanf(" %c", &ch); //a leading space in your format string to eat up any whitespace

        result = number;
        if (ch == 87 || ch == 119)
        { //letter W for UP
            result -= squareLength;
            if (result < 1) // move up
            {
                number += (n - squareLength);
            }
            else
            {
                number -= squareLength;
            }
        }
        else if (ch == 83 || ch == 115)
        { //letter S for DOWN
            result += squareLength;
            if (result > n) // move down
            {
                number -= (n - squareLength);
            }
            else
            {
                number += squareLength;
            }
        }
        else if (ch == 65 || ch == 97)
        { //letter A for LEFT
            result -= 1;
            if (result % squareLength == 0)
            {
                number += (squareLength - 1); // move left
            }
            else
            {
                number -= 1;
            }
        }
        else if (ch == 68 || ch == 100)
        { //letter D for RIGHT
            result += 1;
            if (result % squareLength == 1)
            {
                number -= (squareLength - 1); //move right
            }
            else
            {
                number += 1;
            }
        }
        else if (ch == 50)
        { //press 2 to exit
            option = 2;
            break;
        }

        //printf("\n%d", result);
        printf("\n%d", number);
        //return number;
        /* light it up! */

        *(ptr + (number - 1)) = N;
        delay(50);
        //Clear LED matrix
        memset(map, 0, FILESIZE);
    }
}