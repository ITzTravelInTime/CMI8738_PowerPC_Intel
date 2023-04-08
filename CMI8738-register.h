/*
Copyright (c) 2005 dogbert <dogber1@gmail.com>, Takashi Iwai <tiwai@suse.de>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * CM8x38 registers definition
 */

#define CM_REG_FUNCTRL0		0x00
#define CM_RST_CH1		0x00080000
#define CM_RST_CH0		0x00040000
#define CM_CHEN1		0x00020000	/* ch1: enable */
#define CM_CHEN0		0x00010000	/* ch0: enable */
#define CM_PAUSE1		0x00000008	/* ch1: pause */
#define CM_PAUSE0		0x00000004	/* ch0: pause */
#define CM_CHADC1		0x00000002	/* ch1, 0:playback, 1:record */
#define CM_CHADC0		0x00000001	/* ch0, 0:playback, 1:record */

#define CM_REG_FUNCTRL1		0x04
#define CM_ASFC_MASK		0x0000E000	/* ADC sampling frequency */
#define CM_ASFC_SHIFT		13
#define CM_DSFC_MASK		0x00001C00	/* DAC sampling frequency */
#define CM_DSFC_SHIFT		10
#define CM_SPDF_1		0x00000200	/* SPDIF IN/OUT at channel B */
#define CM_SPDF_0		0x00000100	/* SPDIF OUT only channel A */
#define CM_SPDFLOOP		0x00000080	/* ext. SPDIIF/OUT -> IN loopback */
#define CM_SPDO2DAC		0x00000040	/* SPDIF/OUT can be heard from internal DAC */
#define CM_INTRM		0x00000020	/* master control block (MCB) interrupt enabled */
#define CM_BREQ			0x00000010	/* bus master enabled */
#define CM_VOICE_EN		0x00000008	/* legacy voice (SB16,FM) */
#define CM_UART_EN		0x00000004	/* UART */
#define CM_JYSTK_EN		0x00000002	/* joy stick */

#define CM_REG_CHFORMAT		0x08
#define  CM_DBLSPDS          0x40 /* double SPDF sample rate to support 88.2/96khz */

#define CM_CHB3D5C		0x80000000	/* 5,6 channels */
#define CM_FMOFFSET2    0x40000000  /* initial FM PCM offset 2 when Fmute=1 */ 
#define CM_CHB3D		0x20000000	/* 4 channels */

#define CM_CHIP_MASK1		0x1f000000
#define CM_CHIP_037		0x01000000

#define CM_SPDIF_SELECT1	0x00080000	/* for model <= 037 ? */
#define CM_AC3EN1		0x00100000	/* enable AC3: model 037 */
#define CM_SPD24SEL		0x00020000	/* 24bit spdif: model 037 */
/* #define CM_SPDIF_INVERSE	0x00010000 */ /* ??? */

#define CM_ADCBITLEN_MASK	0x0000C000	
#define CM_ADCBITLEN_16		0x00000000
#define CM_ADCBITLEN_15		0x00004000
#define CM_ADCBITLEN_14		0x00008000
#define CM_ADCBITLEN_13		0x0000C000

#define CM_ADCDACLEN_MASK	0x00003000
#define CM_ADCDACLEN_060	0x00000000
#define CM_ADCDACLEN_066	0x00001000
#define CM_ADCDACLEN_130	0x00002000
#define CM_ADCDACLEN_280	0x00003000

#define CM_CH1_SRATE_176K    0x00000800
#define CM_CH1_SRATE_96K    0x00000800    /* model 055? */
#define CM_CH1_SRATE_88K    0x00000400
#define CM_CH0_SRATE_176K    0x00000200
#define CM_CH0_SRATE_96K    0x00000200    /* model 055? */
#define CM_CH0_SRATE_88K    0x00000100
#define CM_CH0_SRATE_128K    0x00000300
#define CM_CH0_SRATE_MASK    0x00000300

#define CM_SPDIF_INVERSE2	0x00000080	/* model 055? */

#define CM_CH1FMT_MASK		0x0000000C
#define CM_CH1FMT_SHIFT		2
#define CM_CH0FMT_MASK		0x00000003
#define CM_CH0FMT_SHIFT		0

#define CM_REG_INT_HLDCLR	0x0C
#define CM_CHIP_MASK2		0xff000000
#define CM_CHIP_8768        0x20000000
#define CM_CHIP_055        0x08000000
#define CM_CHIP_039		0x04000000
#define CM_CHIP_039_6CH		0x01000000
#define CM_TDMA_INT_EN		0x00040000
#define CM_CH1_INT_EN		0x00020000
#define CM_CH0_INT_EN		0x00010000
#define CM_INT_HOLD		0x00000002
#define CM_INT_CLEAR		0x00000001

#define CM_REG_INT_STATUS	0x10
#define CM_INTR			0x80000000
#define CM_VCO			0x08000000	/* Voice Control? CMI8738 */
#define CM_MCBINT		0x04000000	/* Master Control Block abort cond.? */
#define CM_UARTINT		0x00010000
#define CM_LTDMAINT		0x00008000
#define CM_HTDMAINT		0x00004000
#define CM_XDO46		0x00000080	/* Modell 033? Direct programming EEPROM (read data register) */
#define CM_LHBTOG		0x00000040	/* High/Low status from DMA ctrl register */
#define CM_LEG_HDMA		0x00000020	/* Legacy is in High DMA channel */
#define CM_LEG_STEREO		0x00000010	/* Legacy is in Stereo mode */
#define CM_CH1BUSY		0x00000008
#define CM_CH0BUSY		0x00000004
#define CM_CHINT1		0x00000002
#define CM_CHINT0		0x00000001

#define CM_REG_LEGACY_CTRL	0x14
#define CM_NXCHG		0x80000000	/* h/w multi channels? */
#define CM_VMPU_MASK		0x60000000	/* MPU401 i/o port address */
#define CM_VMPU_330		0x00000000
#define CM_VMPU_320		0x20000000
#define CM_VMPU_310		0x40000000
#define CM_VMPU_300		0x60000000
#define CM_VSBSEL_MASK		0x0C000000	/* SB16 base address */
#define CM_VSBSEL_220		0x00000000
#define CM_VSBSEL_240		0x04000000
#define CM_VSBSEL_260		0x08000000
#define CM_VSBSEL_280		0x0C000000
#define CM_FMSEL_MASK		0x03000000	/* FM OPL3 base address */
#define CM_FMSEL_388		0x00000000
#define CM_FMSEL_3C8		0x01000000
#define CM_FMSEL_3E0		0x02000000
#define CM_FMSEL_3E8		0x03000000
#define CM_ENSPDOUT		0x00800000	/* enable XPDIF/OUT to I/O interface */
#define CM_SPDCOPYRHT		0x00400000	/* set copyright spdif in/out */
#define CM_DAC2SPDO		0x00200000	/* enable wave+fm_midi -> SPDIF/OUT */
#define CM_SETRETRY		0x00010000	/* 0: legacy i/o wait (default), 1: legacy i/o bus retry */
#define CM_CHB3D6C		0x00008000	/* 5.1 channels support */
#define CM_LINE_AS_BASS		0x00006000	/* use line-in as bass */

#define CM_REG_MISC_CTRL	0x18
#define CM_PWD			0x80000000
#define CM_RESET		0x40000000
#define CM_SFIL_MASK		0x30000000
#define CM_TXVX			0x08000000
#define CM_N4SPK3D		0x04000000	/* 4ch output */
#define CM_SPDO5V		0x02000000	/* 5V spdif output (1 = 0.5v (coax)) */
#define CM_SPDIF48K		0x01000000	/* write */
#define CM_SPATUS48K		0x01000000	/* read */
#define CM_ENDBDAC		0x00800000	/* enable dual dac */
#define CM_XCHGDAC		0x00400000	/* 0: front=ch0, 1: front=ch1 */
#define CM_SPD32SEL		0x00200000	/* 0: 16bit SPDIF, 1: 32bit */
#define CM_SPDFLOOPI		0x00100000	/* int. SPDIF-IN -> int. OUT */
#define CM_FM_EN		0x00080000	/* enalbe FM */
#define CM_AC3EN2		0x00040000	/* enable AC3: model 039 */
#define CM_VIDWPDSB		0x00010000 
#define CM_SPDF_AC97		0x00008000	/* 0: SPDIF/OUT 44.1K, 1: 48K */
#define CM_MASK_EN		0x00004000
#define CM_VIDWPPRT		0x00002000
#define CM_SFILENB		0x00001000
#define CM_MMODE_MASK		0x00000E00
#define CM_SPDIF_SELECT2	0x00000100	/* for model > 039 ? */
#define CM_ENCENTER		0x00000080
#define CM_FLINKON		0x00000040
#define CM_FLINKOFF		0x00000020
#define CM_MIDSMP		0x00000010
#define CM_UPDDMA_MASK		0x0000000C
#define CM_TWAIT_MASK		0x00000003

	/* byte */
#define CM_REG_MIXER0		0x20

#define CM_REG_SB16_DATA	0x22
#define CM_REG_SB16_ADDR	0x23

#define CM_REFFREQ_XIN		(315*1000*1000)/22	/* 14.31818 Mhz reference clock frequency pin XIN */
#define CM_ADCMULT_XIN		512			/* Guessed (487 best for 44.1kHz, not for 88/176kHz) */
#define CM_TOLERANCE_RATE	0.001			/* Tolerance sample rate pitch (1000ppm) */
#define CM_MAXIMUM_RATE		80000000		/* Note more than 80MHz */

#define CM_REG_MIXER1		0x24
#define CM_FMMUTE		0x80	/* mute FM */
#define CM_FMMUTE_SHIFT		7
#define CM_WSMUTE		0x40	/* mute PCM */
#define CM_WSMUTE_SHIFT		6
#define CM_SPK4			0x20	/* lin-in -> rear line out */
#define CM_SPK4_SHIFT		5
#define CM_REAR2FRONT		0x10	/* exchange rear/front */
#define CM_REAR2FRONT_SHIFT	4
#define CM_WAVEINL		0x08	/* digital wave rec. left chan */
#define CM_WAVEINL_SHIFT	3
#define CM_WAVEINR		0x04	/* digical wave rec. right */
#define CM_WAVEINR_SHIFT	2
#define CM_X3DEN		0x02	/* 3D surround enable */
#define CM_X3DEN_SHIFT		1
#define CM_CDPLAY		0x01	/* enable SPDIF/IN PCM -> DAC */
#define CM_CDPLAY_SHIFT		0

#define CM_REG_MIXER2		0x25
#define CM_RAUXREN		0x80	/* AUX right capture */
#define CM_RAUXREN_SHIFT	7
#define CM_RAUXLEN		0x40	/* AUX left capture */
#define CM_RAUXLEN_SHIFT	6
#define CM_VAUXRM		0x20	/* AUX right mute */
#define CM_VAUXRM_SHIFT		5
#define CM_VAUXLM		0x10	/* AUX left mute */
#define CM_VAUXLM_SHIFT		4
#define CM_VADMIC_MASK		0x0e	/* mic gain level (0-3) << 1 */
#define CM_VADMIC_SHIFT		1
#define CM_MICGAINZ		0x01	/* mic boost */
#define CM_MICGAINZ_SHIFT	0

#define CM_REG_AUX_VOL		0x26
#define CM_VAUXL_MASK		0xf0
#define CM_VAUXR_MASK		0x0f

#define CM_REG_MISC		0x27
#define CM_XGPO1		0x20
// #define CM_XGPBIO		0x04
#define CM_MIC_CENTER_LFE	0x04	/* mic as center/lfe out? (model 039 or later?) */
#define CM_SPDIF_INVERSE	0x04	/* spdif input phase inverse (model 037) */
#define CM_SPDVALID		0x02	/* spdif input valid check */
#define CM_DMAUTO		0x01

#define CM_REG_AC97		0x28	/* hmmm.. do we have ac97 link? */
/*
 * For CMI-8338 (0x28 - 0x2b) .. is this valid for CMI-8738
 * or identical with AC97 codec?
 */
#define CM_REG_EXTERN_CODEC	CM_REG_AC97

/*
 * MPU401 pci port index address 0x40 - 0x4f (CMI-8738 spec ver. 0.6)
 */
#define CM_REG_MPU_PCI		0x40

/*
 * FM pci port index address 0x50 - 0x5f (CMI-8738 spec ver. 0.6)
 */
#define CM_REG_FM_PCI		0x50

/*
 * for CMI-8338 .. this is not valid for CMI-8738.
 */
#define CM_REG_EXTENT_IND	0xf0
#define CM_VPHONE_MASK		0xe0	/* Phone volume control (0-3) << 5 */
#define CM_VPHONE_SHIFT		5
#define CM_VPHOM		0x10	/* Phone mute control */
#define CM_VSPKM		0x08	/* Speaker mute control, default high */
#define CM_RLOOPREN		0x04    /* Rec. R-channel enable */
#define CM_RLOOPLEN		0x02	/* Rec. L-channel enable */

/*
 * CMI-8338 spec ver 0.5 (this is not valid for CMI-8738):
 * the 8 registers 0xf8 - 0xff are used for programming m/n counter by the PLL
 * unit (readonly?).
 */
#define CM_REG_PLL		0xf8

/*
 * extended registers
 */
#define CM_REG_CH0_FRAME1	0x80	/* base address */
#define CM_REG_CH0_FRAME2	0x84
#define CM_REG_CH1_FRAME1	0x88	/* 0-15: count of samples at bus master; buffer size */
#define CM_REG_CH1_FRAME2	0x8C	/* 16-31: count of samples at codec; fragment size */

#define CM_REG_EXT_MISC        0x90
#define CM_ADC48K44K        0x10000000    /* ADC parameters group, 0: 44k, 1: 48k */
#define CM_CHB3D8C        0x00200000    /* 7.1 channels support */
#define CM_SPD32FMT        0x00100000    /* SPDIF/IN 32k sample rate */
#define CM_ADC2SPDIF        0x00080000    /* ADC output to SPDIF/OUT */
#define CM_SHAREADC        0x00040000    /* DAC in ADC as Center/LFE */
#define CM_REALTCMP        0x00020000    /* monitor the CMPL/CMPR of ADC */
#define CM_INVLRCK        0x00010000    /* invert ZVPORT's LRCK */
#define CM_UNKNOWN_90_MASK    0x0000FFFF    /* ? */

/*
 * size of i/o region
 */
#define CM_EXTENT_CODEC	  0x100
#define CM_EXTENT_MIDI	  0x2
#define CM_EXTENT_SYNTH	  0x4


/*
 * pci ids
 */
#ifndef PCI_VENDOR_ID_CMEDIA
#define PCI_VENDOR_ID_CMEDIA         0x13F6
#endif
#ifndef PCI_DEVICE_ID_CMEDIA_CM8338A
#define PCI_DEVICE_ID_CMEDIA_CM8338A 0x0100
#endif
#ifndef PCI_DEVICE_ID_CMEDIA_CM8338B
#define PCI_DEVICE_ID_CMEDIA_CM8338B 0x0101
#endif
#ifndef PCI_DEVICE_ID_CMEDIA_CM8738
#define PCI_DEVICE_ID_CMEDIA_CM8738  0x0111
#endif
#ifndef PCI_DEVICE_ID_CMEDIA_CM8738B
#define PCI_DEVICE_ID_CMEDIA_CM8738B 0x0112
#endif

/*
 * channels for playback / capture
 */
#define CM_CH_PLAY	0
#define CM_CH_CAPT	1

/*
 * flags to check device open/close
 */
#define CM_OPEN_NONE	0
#define CM_OPEN_CH_MASK	0x01
#define CM_OPEN_DAC	0x10
#define CM_OPEN_ADC	0x20
#define CM_OPEN_SPDIF	0x40
#define CM_OPEN_MCHAN	0x80
#define CM_OPEN_PLAYBACK	(CM_CH_PLAY | CM_OPEN_DAC)
#define CM_OPEN_PLAYBACK2	(CM_CH_CAPT | CM_OPEN_DAC)
#define CM_OPEN_PLAYBACK_MULTI	(CM_CH_PLAY | CM_OPEN_DAC | CM_OPEN_MCHAN)
#define CM_OPEN_CAPTURE		(CM_CH_CAPT | CM_OPEN_ADC)
#define CM_OPEN_SPDIF_PLAYBACK	(CM_CH_PLAY | CM_OPEN_DAC | CM_OPEN_SPDIF)
#define CM_OPEN_SPDIF_CAPTURE	(CM_CH_CAPT | CM_OPEN_ADC | CM_OPEN_SPDIF)


#if CM_CH_PLAY == 1
#define CM_PLAYBACK_SRATE_176K	CM_CH1_SRATE_176K
#define CM_PLAYBACK_SPDF	CM_SPDF_1
#define CM_CAPTURE_SPDF		CM_SPDF_0
#else
#define CM_PLAYBACK_SRATE_176K CM_CH0_SRATE_176K
#define CM_PLAYBACK_SPDF	CM_SPDF_0
#define CM_CAPTURE_SPDF		CM_SPDF_1
#endif
