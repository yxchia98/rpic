/*
 *  C code to demonstrate control of the LED matrix for the
 *  Raspberry Pi Sense HAT add-on board.
 *
 *  Uses the mmap method to map the led device into memory
 *
 *  Build with:  gcc -Wall -O2 led_matrix.c -o led_matrix
 *
 *  Tested with:  Raspbian GNU/Linux 10 (buster) / Raspberry Pi 4 model B
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

#define BOLDBLACK "\033[1m\033[30m" //bold black printing color
#define BOLDGREEN "\033[1m\033[32m" //bold green printing color
#define BOLDRED "\033[1m\033[31m"   //bold red printing color
#define BOLDBLUE "\033[1m\033[34m"  //bold blue printing color
#define RESET "\033[0m"             //reset printing color

void delay(int);
void colorSet(int choice, uint16_t *n);
// uint16_t letterA[64], letterB[64], letterC[64], letterD[64], letterE[64], letterF[64], letterG[64], letterH[64], letterI[64], letterJ[64], letterK[64], letterL[64], letterM[64], letterN[64], letterO[64], letterP[64], letterQ[64], letterR[64], letterS[64], letterT[64], letterU[64], letterV[64], letterW[64], letterX[64], letterY[64], letterZ[64], heart[64], singapore[64], smile[64];
void editMatrix(uint16_t *ptr, uint16_t *N, uint16_t user_matrix[64], uint16_t *map);
void setColor(uint16_t N, uint16_t (*letter_ptr)[64]);
void selectColor(uint16_t *ptr, uint16_t *N, uint16_t letter[][64], uint16_t *map);
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
    uint16_t user_matrix[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
    while (choice != 0)
    {
        printf("%sMAIN MENU%s\n1. Change Color\n2. Edit Matrix\n3. Display Message\n4. Test Game\n0. Exit\nEnter Selection:", BOLDBLACK, RESET);
        scanf("%d", &choice);
        switch (choice)
        {
        case 0:
            break;

        case 1:
            selectColor(p, &N, letter, map);
            break;
        case 2:
            editMatrix(p, &N, user_matrix, map);
            break;
        case 3:
            displayText(p, letter, message, ch);
            break;
        case 4:
            squareGame(8, 1, p, N, map);
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

void editMatrix(uint16_t *ptr, uint16_t *N, uint16_t user_matrix[64], uint16_t *map)
{
    int i, j, k, row, col, edit, choice;
    k = 0;
    choice = 0;
    printf("Matrix customizer\nEnter 0 at anytime to quit and return to main menu\nPress 1 to Continue...\n");
    scanf("%d", &choice);
    while (choice != 0)
    {
        printf("USER MATRIX\n");
        for (i = 0; i < NUM_WORDS; i++)
        {
            *(ptr + i) = user_matrix[i];
        }
        for (i = 0, k = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++, k++)
            {
                user_matrix[k] != 0 ? printf("1 ") : printf("0 ");
            }
            printf("\n");
        }
        printf("Enter row:");
        scanf("%d", &row);
        if (row == 0)
            break;
        printf("Enter col:");
        scanf("%d", &col);
        if (col == 0)
            break;
        if (row <= 0 || col <= 0)
        {
            printf("Enter positive values for rows and columns\n");
        }
        else
        {
            edit = (row - 1) * 8 + col - 1;
            user_matrix[edit] = user_matrix[edit] == 0 ? *N : 0;
        }
    }
    memset(map, 0, FILESIZE);
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

//set color choosen
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
}

//select color choosen
void selectColor(uint16_t *ptr, uint16_t *N, uint16_t letter[][64], uint16_t *map)
{
    int i, choice = 1;
    for (i = 0; i < NUM_WORDS; i++)
    {
        *(ptr + i) = letter[0][i];
    }
    printf("COLOR SETTER\n 0. Exit\n %s1. Red\n %s2. Green\n %s3. Blue\n %s4. White\n", BOLDRED, BOLDGREEN, BOLDBLUE, RESET);
    while (choice != 0)
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
    memset(map, 0, FILESIZE);
}

//display message typed in with sliding animation
void displayText(uint16_t *p, uint16_t letter[26][64], char message[100], char ch)
{
    int option = 1;
    printf("Display Message\nPress 0. Exit\n");
    fgetc(stdin);
    while (option != 0)
    {
        //clear array
        memset(message, 0, 100);
        printf("\nEnter alphabetic message: ");
        fgets(message, 100, stdin);
        if (strlen(message) != 1)
        {
            if (message[0] == 48)
            {
                option = 0;
            }
            else
            {
                int lengthOfMessage = strlen(message) - 1;
                int arr_length = lengthOfMessage * 8;
                uint16_t Choosenletter[8][1000] = {};
                int count = 0;
                uint16_t zero = 0;
                for (int i = 0; i < lengthOfMessage; i++)
                {
                    count = 0;
                    message[i] = changeUpperCase(message[i]);
                    int letterValue = message[i] - 65;
                    int spacing = i * 8;
                    //to set the array of each row of display
                    //compile letters together
                    for (int k = 0; k < 64; k += 8)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            Choosenletter[count][j + spacing] = letter[letterValue][j + k];
                        }
                        // Choosenletter[count][spacing + 1] = zero;

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
//Change lowercase's value to uppercase
char changeUpperCase(char letter)
{
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