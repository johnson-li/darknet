#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#define uint8_t unsigned char
#define uint32_t unsigned int
#define BUFFER_SIZE 100 * 1024 * 1024
#define CONTENT_SIZE BUFFER_SIZE - 2048

struct shared_frames {
    uint32_t size;
    uint32_t offset;
    struct {
        uint32_t offset;
        uint32_t length;
        uint32_t timestamp;
        uint8_t finished;
    } indexes[128];
    uint8_t padding[376];
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

int main(int argc, char **argv) {
    init_shared_memory();
    int index = 0;
    while (index < frames->size && frames->indexes[index].finished == 1) {
        printf("%d\n", index);
    }
    return 0;
}

