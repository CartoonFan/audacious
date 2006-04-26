/*
 * Mpeg Layer-1,2,3 audio decoder
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 * version for slower machines .. decodes only every fourth sample
 * dunno why it sounds THIS annoying (maybe we should adapt the window?)
 * absolutely not optimized for this operation
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"

#define WRITE_SAMPLE(samples,sum,clip) \
  if( (sum) > 32767.0) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < -32768.0) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = sum; }

int mpg123_synth_4to1_8bit(mpgdec_real *bandPtr,int channel,unsigned char *samples,int *pnt)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp + channel;
  int i,ret;
  int pnt1 = 0;

  ret = mpg123_synth_4to1(bandPtr,channel,(unsigned char *) samples_tmp,&pnt1);
  samples += channel + *pnt;

  for(i=0;i<8;i++) {
    *samples = mpg123_conv16to8[*tmp1>>AUSHIFT];
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 16;

  return ret;
}

int mpg123_synth_4to1_8bit_mono(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpg123_synth_4to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<8;i++) {
    *samples++ = mpg123_conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  *pnt += 8;

  return ret;
}


int mpg123_synth_4to1_8bit_mono2stereo(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpg123_synth_4to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<8;i++) {
    *samples++ = mpg123_conv16to8[*tmp1>>AUSHIFT];
    *samples++ = mpg123_conv16to8[*tmp1>>AUSHIFT];
    tmp1 += 2;
  }
  *pnt += 16;

  return ret;
}

int mpg123_synth_4to1_mono(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[16];
  short *tmp1 = samples_tmp;
  int i,ret;
  int pnt1 = 0;

  ret = mpg123_synth_4to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  for(i=0;i<8;i++) {
    *( (short *)samples) = *tmp1;
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 16;

  return ret;
}

int mpg123_synth_4to1_mono2stereo(mpgdec_real *bandPtr,unsigned char *samples,int *pnt)
{
  int i,ret;

  ret = mpg123_synth_4to1(bandPtr,0,samples,pnt);
  samples = samples + *pnt - 32;

  for(i=0;i<8;i++) {
    ((short *)samples)[1] = ((short *)samples)[0];
    samples+=4;
  }

  return ret;
}

int mpg123_synth_4to1(mpgdec_real *bandPtr,int channel,unsigned char *out,int *pnt)
{
  static mpgdec_real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
  short *samples = (short *) (out + *pnt);

  mpgdec_real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    mpg123_dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    mpg123_dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }

  {
    register int j;
    mpgdec_real *window = mpg123_decwin + 16 - bo1;

    for (j=4;j;j--,b0+=0x30,window+=0x70)
    {
      mpgdec_real sum;
      sum  = *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }

    {
      mpgdec_real sum;
      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
      b0-=0x40,window-=0x80;
    }
    window += bo1<<1;

    for (j=3;j;j--,b0-=0x50,window-=0x70)
    {
      mpgdec_real sum;
      sum = -*(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;

      WRITE_SAMPLE(samples,sum,clip); samples += step;
#if 0
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
      WRITE_SAMPLE(samples,sum,clip); samples += step;
#endif
    }
  }
  
  *pnt += 32;

  return clip;
}


