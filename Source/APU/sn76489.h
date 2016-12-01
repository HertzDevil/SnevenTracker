/* 
    SN76489 emulation
    by Maxim in 2001 and 2002
*/

#ifndef _SN76489_H_
#define _SN76489_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char		uint8;		// // //
typedef unsigned short		uint16;
typedef unsigned long		uint32;
typedef signed char			int8;
typedef signed short		int16;
typedef signed long			int32;

#define SN_DISCRETE    0
#define SN_INTEGRATED  1

#ifndef INLINE
//#define INLINE
#define INLINE static __inline
#endif /* INLINE */

/* Function prototypes */
extern void SN76489_Init(void *m, int type);		// // //
extern void SN76489_Reset(void);
extern void SN76489_Config(unsigned int clocks, int preAmp, int boostNoise, int stereo);
extern void SN76489_Write(unsigned int clocks, unsigned int data);
extern void SN76489_Update(unsigned int cycles);
extern void *SN76489_GetContextPtr(void);
extern int SN76489_GetContextSize(void);

//extern void MixSN76489(CMixer *Mixer, int Time, int Level, int Right);		// // //

#ifdef __cplusplus
}
#endif

#endif /* _SN76489_H_ */
