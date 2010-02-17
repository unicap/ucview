#ifndef __DEBAYER_H__
#define __DEBAYER_H__


struct _debayer_data
{
      int use_ccm;
      int use_rbgain;

      // Color Correction Matrix
      int ccm[3][3];

      // red / blue gain
      int rgain;
      int bgain;
};

typedef struct _debayer_data debayer_data_t;


void debayer_ccm_rgb24_nn( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_ccm_rgb24_edge( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_ccm_uyvy( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_calculate_rbgain( unicap_data_buffer_t *buffer, int *rgain, int *bgain );

#endif//__DEBAYER_H__
