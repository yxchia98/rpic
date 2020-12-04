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
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <sys/ioctl.h>
#include <poll.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"
#define DEV_FB "/dev"
#define FB_DEV_NAME "fb"

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
#define BOLDYELLOW "\033[1m\033[33m"  //bold blue printing color
#define RESET "\033[0m"             //reset printing color

void delay(int);
void colorSet(int choice, uint16_t *n);
void editMatrix(uint16_t *ptr, uint16_t *N, uint16_t user_matrix[64], uint16_t *map);
void setColor(uint16_t N, uint16_t (*letter_ptr)[64]);
void selectColor(uint16_t *ptr, uint16_t *N, uint16_t letter[][64], uint16_t *map);
void displayText(uint16_t *p, uint16_t letter[128][64], char message[100], char ch);
int gameSnake(int fbfd, uint16_t N);
void render(uint16_t N);
int check_collision(int appleCheck);
void game_logic(void);
void reset(void);
void change_dir(unsigned int code);
void handle_events(int evfd);

enum direction_t
{
    UP,
    RIGHT,
    DOWN,
    LEFT,
    NONE,
};
struct segment_t
{
    struct segment_t *next;
    int x;
    int y;
};
struct snake_t
{
    struct segment_t head;
    struct segment_t *tail;
    enum direction_t heading;
};
struct apple_t
{
    int x;
    int y;
};

struct fb_t
{
    uint16_t pixel[8][8];
};

int running = 1;

struct snake_t snake = {
    {NULL, 4, 4},
    NULL,
    NONE,
};
struct apple_t apple = {
    4,
    4,
};
struct pollfd evpoll = {
    .events = POLLIN,
};

struct fb_t *fb;

static int is_event_device(const struct dirent *dir)
{
    return strncmp(EVENT_DEV_NAME, dir->d_name,
                   strlen(EVENT_DEV_NAME) - 1) == 0;
}
static int is_framebuffer_device(const struct dirent *dir)
{
    return strncmp(FB_DEV_NAME, dir->d_name,
                   strlen(FB_DEV_NAME) - 1) == 0;
}

static int open_evdev(const char *dev_name)
{
    struct dirent **namelist;
    int i, ndev;
    int fd = -1;

    ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
    if (ndev <= 0)
        return ndev;

    for (i = 0; i < ndev; i++)
    {
        char fname[64];
        char name[256];

        snprintf(fname, sizeof(fname),
                 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
        fd = open(fname, O_RDONLY);
        if (fd < 0)
            continue;
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if (strcmp(dev_name, name) == 0)
            break;
        close(fd);
    }

    for (i = 0; i < ndev; i++)
        free(namelist[i]);

    return fd;
}

static int open_fbdev(const char *dev_name)
{
    struct dirent **namelist;
    int i, ndev;
    int fd = -1;
    struct fb_fix_screeninfo fix_info;

    ndev = scandir(DEV_FB, &namelist, is_framebuffer_device, versionsort);
    if (ndev <= 0)
        return ndev;

    for (i = 0; i < ndev; i++)
    {
        char fname[64];
        char name[256];

        snprintf(fname, sizeof(fname),
                 "%s/%s", DEV_FB, namelist[i]->d_name);
        fd = open(fname, O_RDWR);
        if (fd < 0)
            continue;
        ioctl(fd, FBIOGET_FSCREENINFO, &fix_info);
        if (strcmp(dev_name, fix_info.id) == 0)
            break;
        close(fd);
        fd = -1;
    }
    for (i = 0; i < ndev; i++)
        free(namelist[i]);

    return fd;
}

int main(void)
{
    char message[100] = {}, ch;
    int i, choice;
    //int fbfd;
    uint16_t *map;
    uint16_t *p;
    uint16_t N = W;
    uint16_t user_matrix[64] = {};
    uint16_t letter[128][64] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, N, N, N, N, N, 0, 0, N, N, 0, N, N, 0, 0, N, N, N, N, N, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, N, 0, N, N, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, N, N, N, N, N, N, N, N, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, N, 0, N, N, 0, N, N, N, N, 0, N, N, N, N, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, N, N, N, N, 0, N, N, 0, N, N, N, N, 0, N, N, 0, N, N, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, N, 0, 0, N, N, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, N, 0, N, 0, 0, 0, 0, N, N, 0, 0, 0, N, 0, N, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, N, 0, 0, N, N, 0, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, N, 0, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, 0, N, N, 0, N, N, N, 0, N, N, N, 0, N, N, N, N, N, N, N, 0, N, N, N, N, N, N, N, 0, N, N, 0, N, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, N, N, N, N, 0, N, N, 0, N, N, 0, N, N, N, N, 0, N, N, 0, 0, N, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, 0, 0, N, 0, N, N, 0, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, N, 0, N, N, 0, N, N, N, N, N, N, N, 0, N, N, N, 0, N, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, N, N, N, N, 0, N, N, 0, 0, 0, N, N, 0, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, N, N, N},
                               {0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0},
                               {N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0},
                               {N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, N, N, 0, N, N, N, N, N, N, N, 0, N, N, 0, N, 0, N, N, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, N, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, N, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, N, 0, N, N, 0, N, 0, N, N, 0, N, N, N, N, N, N, N, 0, N, N, N, N, N, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, N, N, 0, 0, N, N, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, N, N, 0, 0, 0, N, N, N, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, N, N, N, N, N, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, N, N, N, N, 0, 0, N, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, N, N, 0, 0, N, 0, 0, N, N, N, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {N, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, N, N, 0, 0, 0, 0, 0, 0, N, N, 0, 0, 0, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, N, N, N, 0, N, N, 0, N, N, 0, N, N, N, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    int ret = 0;
    int fbfd = 0;

    srand(time(NULL));

    evpoll.fd = open_evdev("Raspberry Pi Sense HAT Joystick");
    if (evpoll.fd < 0)
    {
        fprintf(stderr, "Event device not found.\n");
        return evpoll.fd;
    }

    fbfd = open_fbdev("RPi-Sense FB");
    if (fbfd <= 0)
    {
        ret = fbfd;
        printf("Error: cannot open framebuffer device.\n");
        close(evpoll.fd);
    }

    fb = mmap(0, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (!fb)
    {
        ret = EXIT_FAILURE;
        printf("Failed to mmap.\n");
        close(fbfd);
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
            selectColor(p, &N, letter, map);    //calls the change color function
            break;
        case 2:
            editMatrix(p, &N, user_matrix, map);    //calls the edit matrix function
            break;
        case 3:
            displayText(p, letter, message, ch);    //calls the display message function
            break;
        case 4:
            gameSnake(fbfd, N);                    //calls the test game function
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
    close(evpoll.fd);
    return 0;
}

void delay(int t)
{
    usleep(t * 1000);
}

void editMatrix(uint16_t *ptr, uint16_t *N, uint16_t user_matrix[64], uint16_t *map)

{
    int i, j, k, row, col, edit, choice;
    char temp[500], value[10], delim[2] = ",";  //delimiters, temporary string storages
    char *token;
    FILE *save_ptr;
    save_ptr = fopen("saved.txt", "r");
    if (save_ptr == NULL)               //error checking for file not found
    {
        save_ptr = fopen("saved.txt", "w");         //creates a new save file if file not found
        printf("saved file not found, creating a new save\n");
    }
    fgets(temp, 300, save_ptr);                 //get the first line of the save file
    token = strtok(temp, delim);                //split the string file into user_matrix[] array values
    i = 0;
    while (token != NULL)
    {
        user_matrix[i] = atoi(token);           //split the line of string delimited by ',', and assign each element into the nodes of the LED Matrix
        token = strtok(NULL, delim);
        i++;
    }
    if (i != 64)                                //check if the data in the save file fully maps to the 8x8 LED Matrix
    {
        printf("Corrupted save, creating new save\n");
        for (j = 0; j < NUM_WORDS; j++)
        {
            user_matrix[j] = 0;                 //initialize a new matrix if the save file is corrupted
        }
    }
    fclose(save_ptr);                           //close the file pointer
    save_ptr = fopen("saved.txt", "w");         //open a new file pointer for writing
    i = 0;
    k = 0;
    choice = 0;
    printf("Matrix customizer\nEnter 0 at anytime to quit and return to main menu\nPress 1 to Continue...\n");
    scanf("%d", &choice);
    while (choice != 0)
    {
        printf("USER MATRIX\n");
        for (i = 0; i < NUM_WORDS; i++)         //displays the current matrix setup
        {
            *(ptr + i) = user_matrix[i];
        }
        for (i = 0, k = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++, k++)
            {
                user_matrix[k] != 0 ? printf("1 ") : printf("0 ");  //prints out a 8x8 layout of the current configuration
            }
            printf("\n");
        }
        printf("Enter row (1 - 8):");           //prompts user for row input
        scanf("%d", &row);
        if (row == 0)
            break;
        printf("Enter col (1 - 8):");           //prompts user for column input
        scanf("%d", &col);
        if (col == 0)
            break;
        if (row <= 0 || col <= 0 || row > 8 || col > 8)
        {
            printf("Enter positive values(1 - 8) for rows and columns\n");  //prompt user for input if out of range values are entered
        }
        else
        {
            edit = (row - 1) * 8 + col - 1;
            user_matrix[edit] = user_matrix[edit] == 0 ? *N : 0;        //set the respective node with the current color
        }
    }
    for (i = 0; i < NUM_WORDS; i++)         //write the matrix into the save file
    {
        sprintf(value, "%u", user_matrix[i]);
        fputs(value, save_ptr);
        if (i != NUM_WORDS - 1)
        {
            fputs(delim, save_ptr);
        }
    }
    fclose(save_ptr);
    memset(map, 0, FILESIZE);           //reset the framebuffer before return to main menu
}

void colorSet(int choice, uint16_t *n)  //function to set the current color of the LED.
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
        *n = Y;
    }
    else if (choice == 5)
    {
        *n = W;
    }
    else
    {
    }
}

//set color choosen
void setColor(uint16_t N, uint16_t (*letter_ptr)[64])   //apply the current color to change all ascii characters
{
    for (int i = 0; i < 128; i++)
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
    printf("COLOR SETTER\n 0. Exit\n %s1. Red\n %s2. Green\n %s3. Blue\n %s4. Yellow\n %s5. White\n", BOLDRED, BOLDGREEN, BOLDBLUE, BOLDYELLOW, RESET);
    while (choice != 0)
    {
        printf("Select color:");
        scanf("%d", &choice);
        colorSet(choice, N);
        setColor(*N, letter);
    }
    printf("Color changed to:0x%04X\n", *N);
    memset(map, 0, FILESIZE);
}

//display message typed in with sliding animation
void displayText(uint16_t *p, uint16_t letter[128][64], char message[100], char ch)
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
                //length message input
                int lengthOfMessage = strlen(message) - 1;
                //length of array that will be used
                int arr_length = lengthOfMessage * 8;
                uint16_t Choosenletter[8][1000] = {};
                int count = 0;
                uint16_t zero = 0;
                //loop each char in message input
                for (int i = 0; i < lengthOfMessage; i++)
                {
                    count = 0;
                    //get letter value to access letter[], where the pixels of each letter is stored
                    int letterValue = message[i] ;
                    int spacing = i * 8;
                    //to set the array of each row of display
                    //compile letters together
                    for (int k = 0; k < 64; k += 8)
                    {
                        for (int j = 0; j < 8; j++)
                        {
                            Choosenletter[count][j + spacing] = letter[letterValue][j + k];
                        }
                        count++;
                    }
                }
                int number = 0;
                //sliding animation
                for (int m = 0; m < arr_length; m++)
                {
                    count = 0;
                    //each loop moves "right" by 1, causes the sliding animation
                    for (int k = 0; k < 8; k++)
                    {
                        for (int l = 0; l < 8; l++)
                        {
                            *(p + count + l) = Choosenletter[k][l + m];
                        }
                        count += 8;
                    }
                    number++;
                    delay(100);
                    memset(p, 0, FILESIZE);
                }
            }
        }
    }
}

int gameSnake(int fbfd, uint16_t N)
{

    memset(fb, 0, 128);
    snake.tail = &snake.head;
    reset();
    while (running)
    {
        while (poll(&evpoll, 1, 0) > 0)
            handle_events(evpoll.fd);
        game_logic();
        if (check_collision(0))
        {
            reset();
        }
        render(N);
        usleep(300000);
    }
    memset(fb, 0, 128);
    reset();
    munmap(fb, 128);
}

void render(uint16_t N)
{
    struct segment_t *seg_i;
    memset(fb, 0, 128);
    fb->pixel[apple.x][apple.y] = 0xF800;
    for (seg_i = snake.tail; seg_i->next; seg_i = seg_i->next)
    {
        fb->pixel[seg_i->x][seg_i->y] = N;
    }
    fb->pixel[seg_i->x][seg_i->y] = 0xFFFF;
}

int check_collision(int appleCheck)
{
    struct segment_t *seg_i;

    if (appleCheck)
    {
        for (seg_i = snake.tail; seg_i; seg_i = seg_i->next)
        {
            if (seg_i->x == apple.x && seg_i->y == apple.y)
                return 1;
        }
        return 0;
    }

    for (seg_i = snake.tail; seg_i->next; seg_i = seg_i->next)
    {
        if (snake.head.x == seg_i->x && snake.head.y == seg_i->y)
            return 1;
    }

    if (snake.head.x < 0 || snake.head.x > 7 ||
        snake.head.y < 0 || snake.head.y > 7)
    {
        return 1;
    }
    return 0;
}

void game_logic(void)
{
    struct segment_t *seg_i;
    struct segment_t *new_tail;
    for (seg_i = snake.tail; seg_i->next; seg_i = seg_i->next)
    {
        seg_i->x = seg_i->next->x;
        seg_i->y = seg_i->next->y;
    }
    if (check_collision(1))
    {
        new_tail = malloc(sizeof(struct segment_t));
        if (!new_tail)
        {
            printf("Ran out of memory.\n");
            running = 0;
            return;
        }
        new_tail->x = snake.tail->x;
        new_tail->y = snake.tail->y;
        new_tail->next = snake.tail;
        snake.tail = new_tail;

        while (check_collision(1))
        {
            apple.x = rand() % 8;
            apple.y = rand() % 8;
        }
    }
    switch (snake.heading)
    {
    case LEFT:
        seg_i->y--;
        break;
    case DOWN:
        seg_i->x++;
        break;
    case RIGHT:
        seg_i->y++;
        break;
    case UP:
        seg_i->x--;
        break;
    }
}

void reset(void)
{
    struct segment_t *seg_i;
    struct segment_t *next_tail;
    seg_i = snake.tail;
    while (seg_i->next)
    {
        next_tail = seg_i->next;
        free(seg_i);
        seg_i = next_tail;
    }
    snake.tail = seg_i;
    snake.tail->next = NULL;
    snake.tail->x = 2;
    snake.tail->y = 3;
    apple.x = rand() % 8;
    apple.y = rand() % 8;
    snake.heading = NONE;
}

void change_dir(unsigned int code)
{
    switch (code)
    {
    case KEY_UP:
        if (snake.heading != DOWN)
            snake.heading = UP;
        break;
    case KEY_RIGHT:
        if (snake.heading != LEFT)
            snake.heading = RIGHT;
        break;
    case KEY_DOWN:
        if (snake.heading != UP)
            snake.heading = DOWN;
        break;
    case KEY_LEFT:
        if (snake.heading != RIGHT)
            snake.heading = LEFT;
        break;
    }
}

void handle_events(int evfd)
{
    struct input_event ev[64];
    int i, rd;

    rd = read(evfd, ev, sizeof(struct input_event) * 64);
    if (rd < (int)sizeof(struct input_event))
    {
        fprintf(stderr, "expected %d bytes, got %d\n",
                (int)sizeof(struct input_event), rd);
        return;
    }
    for (i = 0; i < rd / sizeof(struct input_event); i++)
    {
        if (ev->type != EV_KEY)
            continue;
        if (ev->value != 1)
            continue;
        switch (ev->code)
        {
        case KEY_ENTER:
            running = 0;
            break;
        default:
            change_dir(ev->code);
        }
    }
}