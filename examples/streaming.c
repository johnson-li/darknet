#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#include <time.h>
#include "stb_image_write.h"
#include "darknet.h"

#define uint8_t unsigned char
#define uint32_t unsigned int
#define uint16_t unsigned short 
#define uint64_t unsigned long long 
#define FRAMES_SIZE 128
#define BUFFER_SIZE 100 * 1024 * 1024
#define CONTENT_SIZE BUFFER_SIZE - 2048

struct shared_frames {
    uint32_t size;
    uint32_t offset;
    struct {
        uint32_t offset;
        uint32_t length;
        uint16_t width;
        uint16_t height;
        uint32_t timestamp;
        int8_t finished;
    } indexes[FRAMES_SIZE];
    uint8_t padding[1912];
    uint8_t content[CONTENT_SIZE];
} *frames = NULL;

void init_shared_memory() {
    int fd_shm;
    if ((fd_shm = shm_open("/webrtc_frames", O_RDONLY, 0660)) == -1) {
        perror("shm_open");
    }
    if ((frames = (struct shared_frames*) mmap(NULL, sizeof(struct shared_frames), PROT_READ, MAP_SHARED, fd_shm, 0)) == MAP_FAILED) {
        perror ("mmap");
    }
}

image get_image(uint8_t* data, int width, int height) {
    image im = make_image(width, height, 3);
    int c = 4;
    int i, j, k;
    for(k = 0; k < c - 1; ++k){
        for(j = 0; j < height; ++j){
            for(i = 0; i < width; ++i){
                int dst_index = i + width*j + width*height*(2 - k);
                int src_index = k + c*i + c*width*j;
                int alpha_index = 3 + c*i + c*width*j;
                im.data[dst_index] = (float)data[src_index]/255. * data[alpha_index] / 255.;
            }
        }
    }
    return im;
}

void save_image2(char* path, uint8_t* data, int width, int height, int len) {
    image im = get_image(data, width, height);
    //stbi_write_jpg(path, width, height, 4, data, 0);
    save_image(im, path);
}

void handle_image(uint8_t* data, int width, int height, int len) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    save_image2("test", data, width, height, len); 

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    fprintf(stderr, "Processing image costs %lld ms\n", delta_us);
}

void handle_images() {
    fprintf(stderr, "Initial frames size: %d\n", frames->size);
    int index = frames->size - 1;
    //index = frames->size + 2 - FRAMES_SIZE ;
    if (index < 0) {
        index = 0;
    }
    while (1) {
        int i  = index % FRAMES_SIZE;
        //printf("%d %d %d\n", index, frames->size, frames->indexes[i].finished);
        if (index < frames->size && frames->indexes[i].finished == 1) {
            fprintf(stderr, "Index: %d, offset: %u, size: [%dx%d], finished: %d, timestamp: %u\n", index, frames->indexes[i].offset, frames->indexes[i].width, frames->indexes[i].height, frames->indexes[i].finished, frames->indexes[i].timestamp);
            handle_image(frames->content + frames->indexes[i].offset, frames->indexes[i].width, frames->indexes[i].height, frames->indexes[i].length);
            index++;
        } 
    }
}

void init_nn() {

}

int main(int argc, char **argv) {
    init_shared_memory();
    init_nn();
    handle_images();
    return 0;
}

