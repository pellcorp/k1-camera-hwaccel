#include <stdio.h>
#include <stdlib.h>

#include <osd/OsdApi.h>
#include <syncframe.h>

int set_layer_info(struct osd_head_info *osd_head,
		   int width, int height, int x, int y,
		   int fmt, int order)
{
  osd_lay_cfg *layer = osd_head->layer;

  layer->lay_en = 1;
  layer->lay_z_order = order;

  layer->lay_scale_en = 1;
  layer->scale_w = 720;
  layer->scale_h = 1280;
  layer->source_w = width;
  layer->source_h = height * 2;
  layer->disp_pos_x = x;
  layer->disp_pos_y = y;
  layer->g_alpha_en = 0;
  layer->g_alpha_val = 0xff;
  layer->color = 0;
  layer->format = fmt;
  layer->stride = width;
  return 0;
}

void syncframe_combination(void *inputl, void *inputr, __u32 width, __u32 height, void *output) 
{
    __u8 *out = (__u8 *)output;
    __u8 *inl = (__u8 *)inputl;
    __u8 *inr = (__u8 *)inputr;

    memcpy(out, inl, width * height);
    memcpy(out + (width * height), inr, width * height);
    memcpy(out + (width * height * 2), inl + width * height, width * height / 2);
    memcpy(out + (width * height * 5 / 2), inr + width * height, width * height / 2);

}

int main()
{
  int i, ret;
  struct osd_head_info *osd_head;

  osd_head = (struct osd_head_info *)malloc(sizeof(struct osd_head_info));
  int width = 1280, height = 720, x = 0, y = 0, fmt, space = 0, layer = 0;
  fmt = LAYER_CFG_FORMAT_NV12;
  int type = 0, time = 0, color = 0;

  init_osd_client((char*)("client"));

  get_osd_reserved_package (osd_head);
  set_layer_info (osd_head, width, height, x, y, fmt, 0);

  {
    struct config config;           //get syncframe struct
    struct cameras_buf *syncframe;  //syncframe infomation
    void *out = malloc(width * height * 3);

    char r_name[32] = "/dev/video0";
    char l_name[32] = "/dev/video3";
    config.width = width;
    config.height = height;
    config.pixfmt = V4L2_PIX_FMT_NV12;
    config.camera_r_name = r_name;
    config.camera_l_name = l_name;
    config.camera_name = NULL;
    config.bcount = 3;


    if (cameras_init(SYNCFRAME, &config) < 0) {//init device
      fprintf(stderr, "cameras_init failed!\n");
      return 0;
    }

    i = 500;
    while(1) {
      if ((syncframe = dqbuf_syncframe(&config)) == NULL ) {//get syncframe function
        fprintf(stderr, "get syncframe failed\n");
        return 0;
      }

      printf("syncframe time difference = %0.6f ms\n", syncframe->time_diff);
      syncframe_combination(syncframe->Lcamera.camera_buf, syncframe->Lcamera.camera_buf, width, height, out);
      memcpy(osd_head->shm_mem, out, config.buffer_size * 2);

      qbuf_syncframe(&config, syncframe);//free syncframe

      post_osd_package (osd_head);
    }

  uninit_video(&config);//uninit device
  }


  uinit_client();
  printf("uninit_client\n");
  return 0;
}
