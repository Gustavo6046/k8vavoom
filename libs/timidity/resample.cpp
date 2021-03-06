/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    resample.c
*/

#include <stdlib.h>
#include <inttypes.h>

#include "timidity.h"

namespace LibTimidity
{

#define RESAMPLATION \
      v1=src[ofs>>FRACTION_BITS];\
      v2=src[(ofs>>FRACTION_BITS)+1];\
      *dest++ = (sample_t)(v1 + (((v2 - v1) * (ofs & FRACTION_MASK)) >> FRACTION_BITS));
#define INTERPVARS sample_t v1, v2

//#define FINALINTERP if (ofs == le) *dest++=src[ofs>>FRACTION_BITS];
//k8: dunno, wtf?
#define FINALINTERP if (ofs == le) *dest++=src[(ofs>>FRACTION_BITS)-1]/2;
/* So it isn't interpolation. At least it's final. */

#define PRECALC_LOOP_COUNT(start, end, incr) (((end) - (start) + (incr) - 1) / (incr))

/*************** resampling with fixed increment *****************/

static sample_t* rs_plain(MidiSong* song, int v, int32* countptr)
{
	/* Play sample until end, then free the voice. */
	INTERPVARS;
	Voice* vp = &song->voice[v];
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;
	int32
		ofs = vp->sample_offset,
		incr = vp->sample_increment,
		le = vp->sample->data_length,
		count = *countptr;

	int32 i;

	if (incr == 0) return song->resample_buffer; //k8: hack for "chaos_bank_v1_9_12mb.sf2"

	if (incr < 0)
	{
		incr = -incr; /* In case we're coming out of a bidir loop */
	}
	/* Precalc how many times we should go through the loop.
		NOTE: Assumes that incr > 0 and that ofs <= le */
	//i = (le - ofs) / incr + 1;
	i = PRECALC_LOOP_COUNT(ofs, le, incr);

	if (i > count)
	{
		i = count;
		count = 0;
	}
	else
	{
		count -= i;
	}

	//while (i--)
	for (int32 j = 0; j < i; j++)
	{
		RESAMPLATION;
		ofs += incr;
	}

	if (ofs >= le)
	{
		FINALINTERP;
		vp->status = VOICE_FREE;
		*countptr -= count + 1;
	}
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

static sample_t* rs_loop(MidiSong* song, Voice* vp, int32 count)
{
	/* Play sample until end-of-loop, skip back and continue. */
	INTERPVARS;
	int32
		ofs = vp->sample_offset,
		incr = vp->sample_increment,
		le = vp->sample->loop_end,
		ll = le - vp->sample->loop_start;
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;

	int32 i;

	if (incr == 0) return song->resample_buffer; //k8: hack for "chaos_bank_v1_9_12mb.sf2"

	while (count)
	{
		if (ofs >= le)
		{
			/* NOTE: Assumes that ll > incr and that incr > 0. */
			ofs -= ll;
		}
		/* Precalc how many times we should go through the loop */
		//i = (le - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, le, incr);

		if (i > count)
		{
			i = count;
			count = 0;
		}
		else
		{
			count -= i;
		}

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}
	}
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

static sample_t* rs_bidir(MidiSong* song, Voice* vp, int32 count)
{
	INTERPVARS;
	int32
		ofs = vp->sample_offset,
		incr = vp->sample_increment,
		le = vp->sample->loop_end,
		ls = vp->sample->loop_start;
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;

	int32
		le2 = le << 1,
		ls2 = ls << 1,
		i;

	if (incr == 0) return song->resample_buffer; //k8: hack for "chaos_bank_v1_9_12mb.sf2"

	/* Play normally until inside the loop region */
	if (ofs <= ls)
	{
		/* NOTE: Assumes that incr > 0, which is NOT always the case
		when doing bidirectional looping.  I have yet to see a case
		where both ofs <= ls AND incr < 0, however. */
		//i = (ls - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, ls, incr);
		if (i > count)
		{
			i = count;
			count = 0;
		}
		else
		{
			count -= i;
		}

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}
	}

	/* Then do the bidirectional looping */
	while(count)
	{
		/* Precalc how many times we should go through the loop */
		//i = ((incr > 0 ? le : ls) - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, incr > 0 ? le : ls, incr);

		if (i > count)
		{
			i = count;
			count = 0;
		}
		else
		{
			count -= i;
		}

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}

		if (ofs>=le)
		{
			/* fold the overshoot back in */
			ofs = le2 - ofs;
			incr *= -1;
		}
		else if (ofs <= ls)
		{
			ofs = ls2 - ofs;
			incr *= -1;
		}
	}
	vp->sample_increment = incr;
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

/*********************** vibrato versions ***************************/

/* We only need to compute one half of the vibrato sine cycle */
static int vib_phase_to_inc_ptr(int phase)
{
	if (phase < VIBRATO_SAMPLE_INCREMENTS / 2)
	{
		return VIBRATO_SAMPLE_INCREMENTS / 2 - 1 - phase;
	}
	else if (phase >= 3 * VIBRATO_SAMPLE_INCREMENTS / 2)
	{
		return 5 * VIBRATO_SAMPLE_INCREMENTS / 2 - 1 - phase;
	}
	else
	{
		return phase - VIBRATO_SAMPLE_INCREMENTS / 2;
	}
}

static int32 update_vibrato(Voice* vp, int sign)
{
	int32 depth;
	int phase, pb;
	double a;

	if (vp->vibrato_phase++ >= 2 * VIBRATO_SAMPLE_INCREMENTS - 1)
	{
		vp->vibrato_phase = 0;
	}
	phase = vib_phase_to_inc_ptr(vp->vibrato_phase);

	if (vp->vibrato_sample_increment[phase])
	{
		if (sign)
		{
			return -vp->vibrato_sample_increment[phase];
		}
		else
		{
			return vp->vibrato_sample_increment[phase];
		}
	}
	/* Need to compute this sample increment. */
	depth = vp->sample->vibrato_depth << 7;

	if (vp->vibrato_sweep)
	{
		/* Need to update sweep */
		vp->vibrato_sweep_position += vp->vibrato_sweep;

		if (vp->vibrato_sweep_position >= (1<<SWEEP_SHIFT))
		{
			vp->vibrato_sweep = 0;
		}
		else
		{
			/* Adjust depth */
			depth *= vp->vibrato_sweep_position;
			depth >>= SWEEP_SHIFT;
		}
	}
	a = VTIM_FSCALE(((double)(vp->sample->sample_rate) *
		(double)(vp->frequency)) /
		((double)(vp->sample->root_freq) *
		(double)(OUTPUT_RATE)),
		FRACTION_BITS);

	pb=(int)((sine(vp->vibrato_phase *
		(SINE_CYCLE_LENGTH/(2*VIBRATO_SAMPLE_INCREMENTS)))
		* (double)(depth) * VIBRATO_AMPLITUDE_TUNING));

	if (pb < 0)
	{
		pb = -pb;
		a /= bend_fine[(pb >> 5) & 0xFF] * bend_coarse[pb >> 13];
	}
	else
	{
		a *= bend_fine[(pb >> 5) & 0xFF] * bend_coarse[pb >> 13];
	}

	/* If the sweep's over, we can store the newly computed sample_increment */
	if (!vp->vibrato_sweep)
	{
		vp->vibrato_sample_increment[phase] = (int32)a;
	}

	if (sign)
	{
		a = -a; /* need to preserve the loop direction */
	}

	return (int32) a;
}

static sample_t* rs_vib_plain(MidiSong* song, int v, int32* countptr)
{
	/* Play sample until end, then free the voice. */
	INTERPVARS;
	Voice* vp = &song->voice[v];
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;
	int32
		le = vp->sample->data_length,
		ofs = vp->sample_offset,
		incr = vp->sample_increment,
		count = *countptr;
	int
		cc = vp->vibrato_control_counter;

	/* This has never been tested */
	if (incr < 0)
	{
		incr = -incr; /* In case we're coming out of a bidir loop */
	}

	//k8: hack for "chaos_bank_v1_9_12mb.sf2"
	if ((uintptr_t)src <= 65536)
	{
		while (count--) *dest++ = 0;
		vp->status = VOICE_FREE;
		*countptr -= count + 1;
		ofs = 0;
		if (incr > 0) incr = 1;
		cc = 0;
	}
	else
	{
		while (count--)
		{
			if (!cc--)
			{
				cc = vp->vibrato_control_ratio;
				incr = update_vibrato(vp, 0);
			}
			RESAMPLATION;
			ofs += incr;

			if (ofs >= le)
			{
				FINALINTERP;
				vp->status = VOICE_FREE;
				*countptr -= count + 1;
				break;
			}
		}
	}
	vp->vibrato_control_counter = cc;
	vp->sample_increment = incr;
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

static sample_t* rs_vib_loop(MidiSong* song, Voice* vp, int32 count)
{
	/* Play sample until end-of-loop, skip back and continue. */
	INTERPVARS;
	int32
		ofs=vp->sample_offset,
		incr=vp->sample_increment,
		le=vp->sample->loop_end,
		ll=le - vp->sample->loop_start;
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;
	int cc = vp->vibrato_control_counter;

	int32 i;
	int vibflag = 0;

	if (incr == 0) return song->resample_buffer; //k8: hack for "chaos_bank_v1_9_12mb.sf2"

	while (count)
	{
		/* Hopefully the loop is longer than an increment */
		while (ofs >= le)
		{
			ofs -= ll;
		}
		/* Precalc how many times to go through the loop, taking
		the vibrato control ratio into account this time. */
		//i = (le - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, le, incr);

		if (i > count)
		{
			i = count;
		}

		if (i > cc)
		{
			i = cc;
			vibflag = 1;
		}
		else
		{
			cc -= i;
		}
		count -= i;

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}

		if (vibflag)
		{
			cc = vp->vibrato_control_ratio;
			incr = update_vibrato(vp, 0);
			vibflag = 0;
		}
	}
	vp->vibrato_control_counter = cc;
	vp->sample_increment = incr;
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

static sample_t* rs_vib_bidir(MidiSong* song, Voice* vp, int32 count)
{
	INTERPVARS;
	int32
		ofs = vp->sample_offset,
		incr = vp->sample_increment,
		le = vp->sample->loop_end,
		ls = vp->sample->loop_start;
	sample_t* dest = song->resample_buffer;
	sample_t* src = vp->sample->data;
	int cc = vp->vibrato_control_counter;

	int32
		le2 = le << 1,
		ls2 = ls << 1,
		i;
	int vibflag = 0;

	if (incr == 0) return song->resample_buffer; //k8: hack for "chaos_bank_v1_9_12mb.sf2"

	/* Play normally until inside the loop region */
	while (count && incr > 0 && ofs <= ls)
	{
		//i = (ls - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, ls, incr);

		if (i > count)
		{
			i = count;
		}

		if (i > cc)
		{
			i = cc;
			vibflag = 1;
		}
		else
		{
			cc -= i;
		}
		count -= i;

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}

		if (vibflag)
		{
			cc = vp->vibrato_control_ratio;
			incr = update_vibrato(vp, 0);
			vibflag = 0;
		}
	}

	/* Then do the bidirectional looping */
	while (count)
	{
		/* Precalc how many times we should go through the loop */
		//i = ((incr > 0 ? le : ls) - ofs) / incr + 1;
		i = PRECALC_LOOP_COUNT(ofs, incr > 0 ? le : ls, incr);

		if (i > count)
		{
			i = count;
		}

		if (i > cc)
		{
			i = cc;
			vibflag = 1;
		}
		else
		{
			cc -= i;
		}
		count -= i;

		//while (i--)
		for (int32 j = 0; j < i; j++)
		{
			RESAMPLATION;
			ofs += incr;
		}

		if (vibflag)
		{
			cc = vp->vibrato_control_ratio;
			incr = update_vibrato(vp, (incr < 0));
			vibflag = 0;
		}

		if (ofs >= le)
		{
			/* fold the overshoot back in */
			ofs = le2 - ofs;
			incr *= -1;
		}
		else if (ofs <= ls)
		{
			ofs = ls2 - ofs;
			incr *= -1;
		}
	}
	vp->vibrato_control_counter = cc;
	vp->sample_increment = incr;
	vp->sample_offset = ofs; /* Update offset */

	return song->resample_buffer;
}

sample_t* resample_voice(MidiSong* song, int v, int32* countptr)
{
	uint8 modes;
	Voice* vp = &song->voice[v];

	if (!(vp->sample->sample_rate))
	{
		/* Pre-resampled data -- just update the offset and check if
			we're out of data. */
		int32 ofs = vp->sample_offset >> FRACTION_BITS; /* Kind of silly to use
							FRACTION_BITS here... */

		if (*countptr >= (vp->sample->data_length >> FRACTION_BITS) - ofs)
		{
			/* Note finished. Free the voice. */
			vp->status = VOICE_FREE;

			/* Let the caller know how much data we had left */
			*countptr = (vp->sample->data_length >> FRACTION_BITS) - ofs;
		}
		else
		{
			vp->sample_offset += *countptr << FRACTION_BITS;
		}

		return vp->sample->data + ofs;
	}
	/* Need to resample. Use the proper function. */
	modes = vp->sample->modes;

	if (vp->vibrato_control_ratio)
	{
		if ((modes & MODES_LOOPING) &&
			((modes & MODES_ENVELOPE) ||
			(vp->status == VOICE_ON || vp->status == VOICE_SUSTAINED)))
		{
			if (modes & MODES_PINGPONG)
			{
				return rs_vib_bidir(song, vp, *countptr);
			}
			else
			{
				return rs_vib_loop(song, vp, *countptr);
			}
		}
		else
		{
			return rs_vib_plain(song, v, countptr);
		}
	}
	else
	{
		if ((modes & MODES_LOOPING) &&
			((modes & MODES_ENVELOPE) ||
			(vp->status == VOICE_ON || vp->status == VOICE_SUSTAINED)))
		{
			if (modes & MODES_PINGPONG)
			{
				return rs_bidir(song, vp, *countptr);
			}
			else
			{
				return rs_loop(song, vp, *countptr);
			}
		}
		else
		{
			return rs_plain(song, v, countptr);
		}
	}
}

void pre_resample(Sample* sp)
{
	double a, xdiff;
	int32 incr, ofs, newlen, count;
	sample_t *newdata, *dest, *src = sp->data, *vptr;
	int32 v, v1, v2, v3, v4, i;
	static const char note_name[12][3] =
	{
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
	};

	ctl->cmsg(CMSG_INFO, VERB_NOISY, " * pre-resampling for note %d (%s%d)",
		sp->note_to_use,
		note_name[sp->note_to_use % 12], (sp->note_to_use & 0x7F) / 12);

	a = ((double) (sp->sample_rate) * freq_table[(int) (sp->note_to_use)]) /
		((double) (sp->root_freq) * OUTPUT_RATE);
	newlen = (int32)(sp->data_length / a);
	dest = newdata = (sample_t*)safe_malloc(newlen >> (FRACTION_BITS - 1));

	count = (newlen >> FRACTION_BITS) - 1;
	ofs = incr = (sp->data_length - (1 << FRACTION_BITS)) / count;

	if (newlen <= 0 || count <= 0 || incr <= 0)
	{
		// k8: i absolutely don't know what i am doing here
		fprintf(stderr, "TIMIDITY FUCKED!\n");
		*newdata = 0;
		sp->data_length = 1;
		sp->loop_start = 0;
		sp->loop_end = 1;
		free(sp->data);
		sp->data = (sample_t *)newdata;
		sp->sample_rate = 0;
		return;
	}

	if (--count)
	{
		*dest++ = src[0];
	}

	/* Since we're pre-processing and this doesn't have to be done in
		real-time, we go ahead and do the full sliding cubic interpolation. */
	count--;
	for(i = 0; i < count; i++)
	//while (--count)
	{
		vptr = src + (ofs >> FRACTION_BITS);
		/*
		 * Electric Fence to the rescue: Accessing *(vptr - 1) is not a
		 * good thing to do when vptr <= src. (TiMidity++ has a similar
		 * safe-guard here.)
		 */
		v1 = (vptr == src) ? *vptr : *(vptr - 1);
		v2 = *vptr;
		v3 = *(vptr + 1);
		v4 = *(vptr + 2);
		xdiff = VTIM_FSCALENEG(ofs & FRACTION_MASK, FRACTION_BITS);
#if defined(USE_FPU_MATH)
		v = v2;
#else
		v = (int32)(v2 + (xdiff / 6.0) * (-2 * v1 - 3 * v2 + 6 * v3 - v4 +
			xdiff * (3 * (v1 - 2 * v2 + v3) + xdiff * (-v1 + 3 * (v2 - v3) + v4))));
#endif
		if (v < -32768) v = -32768; else if (v > 32767) v = 32767;
		*dest++ = (sample_t)v;
		ofs += incr;
	}

	if (ofs & FRACTION_MASK)
	{
		RESAMPLATION;
	}
	else
	{
		*dest++ = src[ofs >> FRACTION_BITS];
	}

	//k8:dunno, really; it should be safe
	*dest = *(dest - 1) / 2;
	++dest;
	*dest = *(dest - 1) / 2;

	sp->data_length = newlen;
	sp->loop_start = (int32)(sp->loop_start / a);
	sp->loop_end = (int32)(sp->loop_end / a);
	free(sp->data);
	sp->data = (sample_t *) newdata;
	sp->sample_rate = 0;
}

};
