//
// Created by lix16 on 5/30/18.
//
#include <stdio.h>
#include <dirent.h>
#include <darknet.h>
#include <malloc.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define CHANNEL 3
#define WIDTH 1280
#define HEIGHT 720
#define YUV_SIZE WIDTH * HEIGHT * CHANNEL / 2
#define RGB_SIZE WIDTH * HEIGHT * CHANNEL
#define nms .45

__uint8_t yuv[YUV_SIZE];
__uint8_t rgb_rgb[RGB_SIZE];
__uint8_t rgb_r[HEIGHT * WIDTH];
__uint8_t rgb_g[HEIGHT * WIDTH];
__uint8_t rgb_b[HEIGHT * WIDTH];

void yuv2rgb(__uint8_t* yuv) {
    AVFrame *frame = av_frame_alloc();
    av_image_fill_arrays(frame->data, frame->linesize, yuv, AV_PIX_FMT_YUV420P, WIDTH, HEIGHT, 1);
    struct SwsContext *sws_ctx = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_YUV420P, WIDTH, HEIGHT, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    int stride[1] = {3 * WIDTH};
    uint8_t *rgb24[] = {rgb_rgb};
    sws_scale(sws_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, HEIGHT, rgb24, stride);
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
        rgb_r[i] = rgb_rgb[i * 3 + 0];
        rgb_g[i] = rgb_rgb[i * 3 + 1];
        rgb_b[i] = rgb_rgb[i * 3 + 2];
    }
}

int run_one_time(network *net, float thresh, float hier_thresh, image **alphabet, char **names, char *outfile) {
    struct dirent **namelist;
    int n;

    n = scandir("/tmp/webrtc", &namelist, 0, alphasort);
    if (n <= 0) {
        printf("Could not open webrtc directory");
        return 0;
    }

    char* latest = namelist[n-2]->d_name;
    char path[100];
    sprintf(path, "/tmp/webrtc/%s", latest);
    printf("Open file: %s\n", path);
    FILE *ptr = fopen(path, "rb");
    if (ptr) {
        fread(yuv, YUV_SIZE, 1, ptr);
    } else {
        printf("Could not open file\n");
    }
    yuv2rgb(yuv);

    image im = make_image(WIDTH, HEIGHT, CHANNEL);
    for (int j = 0; j < HEIGHT; j++) {
        for (int k = 0; k < WIDTH; k++) {
            for (int i = 0; i < CHANNEL; i++) {
                im.data[k + WIDTH*j + HEIGHT*WIDTH*i] = (float)rgb_rgb[i + CHANNEL*k + CHANNEL*WIDTH*j] / 255;
            }
        }
    }

    printf("123123\n");
    image sized = letterbox_image(im, net->w, net->h);
    layer l = net->layers[net->n-1];
    float *X = sized.data;
    double time=what_time_is_it_now();
    network_predict(net, X);
    printf("%s: Predicted in %f seconds.\n", path, what_time_is_it_now()-time);
    int nboxes = 0;
    detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh, 0, 1, &nboxes);
    //printf("%d\n", nboxes);
    //if (nms) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    if (nms) do_nms_sort(dets, nboxes, l.classes, nms);
    draw_detections(im, dets, nboxes, thresh, names, alphabet, l.classes);
    free_detections(dets, nboxes);
    if(outfile){
        printf("write to: %s\n", outfile);
        save_image(im, outfile);
    } else {
        printf("no output file specified\n");
    }
    return 1;
}

int main(int argc, char **argv) {
    float thresh = find_float_arg(argc, argv, "-thresh", .5);
    char *outfile = find_char_arg(argc, argv, "-out", 0);

    char *datacfg = "cfg/coco.data";
    char *cfgfile = argv[2];
    char *weightfile = argv[3];
    list *options = read_data_cfg(datacfg);
    char *name_list = option_find_str(options, "names", "data/names.list");
    char **names = get_labels(name_list);

    image **alphabet = load_alphabet();
    network *net = load_network(cfgfile, weightfile, 0);
    set_batch_network(net, 1);
    srand(2222222);

    run_one_time(net, thresh, 0.5f, alphabet, names, outfile);
    return 1;
}
