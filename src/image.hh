#ifndef __IMAGE_HH__
#define __IMAGE_HH__

// The DilloImage data-structure and methods


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "bitvec.h"
#include "url.h"

typedef struct _DilloImage DilloImage;

typedef enum {
   DILLO_IMG_TYPE_INDEXED,
   DILLO_IMG_TYPE_RGB,
   DILLO_IMG_TYPE_GRAY,
   DILLO_IMG_TYPE_NOTSET    /* Initial value */
} DilloImgType;

/* These will reflect the Image's "state" */
typedef enum {
   IMG_Empty,      /* Just created the entry */
   IMG_SetParms,   /* Parameters set */
   IMG_SetCmap,    /* Color map set */
   IMG_Write,      /* Feeding the entry */
   IMG_Close,      /* Whole image got! */
   IMG_Abort       /* Image transfer aborted */
} ImageState;

struct _DilloImage {
   void *dw;

   /* Parameters as told by image data */
   uint_t width;
   uint_t height;

   const uchar_t *cmap;     /* Color map (only for indexed) */
   DilloImgType in_type;    /* Image Type */
   int32_t bg_color;        /* Background color */

   bitvec_t *BitVec;        /* Bit vector for decoded rows */
   uint_t ScanNumber;       /* Current decoding scan */
   ImageState State;        /* Processing status */

   int RefCount;            /* Reference counter */
};


/*
 * Function prototypes
 */
DilloImage *a_Image_new(int width, int height,
                        const char *alt_text, int32_t bg_color);
void a_Image_ref(DilloImage *Image);
void a_Image_unref(DilloImage *Image);

void a_Image_set_parms(DilloImage *Image, void *v_imgbuf, DilloUrl *url,
                       int version, uint_t width, uint_t height,
                       DilloImgType type);
void a_Image_set_cmap(DilloImage *Image, const uchar_t *cmap);
void a_Image_new_scan(DilloImage *image, void *v_imgbuf);
void a_Image_write(DilloImage *Image, void *v_imgbuf,
                   const uchar_t *buf, uint_t y, int decode);
void a_Image_close(DilloImage *Image);

void a_Image_imgbuf_ref(void *v_imgbuf);
void a_Image_imgbuf_unref(void *v_imgbuf);
void *a_Image_imgbuf_new(void *v_dw, int img_type, int width, int height) ;
int a_Image_imgbuf_last_reference(void *v_imgbuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IMAGE_HH__ */

