#include "camera.h"

#include <stddef.h>
#include <stdio.h>

#include "global_config.h"

#if CONFIG_CAMERA_OV2640
#include "ov2640.h"
#endif

#if (CONFIG_CAMERA_GC0328_SINGLE || CONFIG_CAMERA_GC0328_DUAL)
#include "face_lib.h"
#endif

static camera_t camera = {
    .slv_id = 0x0,

    .camera_config = NULL,
    .camera_set_hmirror = NULL,
    .camera_set_vflip = NULL,
    .camera_read_id = NULL,
};

int camera_read_id(uint16_t *manuf_id, uint16_t *device_id)
{
    if (camera.camera_read_id)
    {
        camera.camera_read_id(&camera, manuf_id, device_id);
        return 0;
    }
    return 1;
}

int camera_set_hmirror(uint8_t val)
{
    printf("camera_set_hmirror: %d\r\n", val);

    if (camera.camera_set_hmirror)
    {
        camera.camera_set_hmirror(&camera, val);
        return 0;
    }

    return 1;
}

int camera_set_vflip(uint8_t val)
{
    printf("camera_set_vflip: %d\r\n", val);

    if (camera.camera_set_vflip)
    {
        camera.camera_set_vflip(&camera, val);
        return 0;
    }
    return 1;
}

int camera_config(void)
{
    if (camera.camera_config)
    {
        camera.camera_config(&camera);
        return 0;
    }
    return 1;
}

int camera_init(camera_type_t type)
{
    switch (type)
    {
#if CONFIG_CAMERA_OV2640
    case CAM_OV2640:
    {
        ov2640_init(&camera);
    }
    break;
#endif
#if (CONFIG_CAMERA_GC0328_SINGLE || CONFIG_CAMERA_GC0328_DUAL)
    case CAM_GC0328_SINGLE:
    {
        gc0328_single_init(&camera);
    }
    break;
    case CAM_GC0328_DUAL:
    {
        gc0328_dual_init(&camera);
    }
    break;
#endif
    default:
    {
        printf("unknown camera type !!!\r\n");
        return 1;
    }
    break;
    }
    /* config camera */
    camera.camera_config(&camera);
    return 0;
}
