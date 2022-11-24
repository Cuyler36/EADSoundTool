#pragma once

#include "common.h"

#define VOLUME_MAX 32767.0f

enum ADSRState {
	Stopped = 0,
	CheckSustainBeforeProcessing = 1,
	InitProcessing = 2,
	ProcessState = 3,
	Attack = 4,
	Sustain = 5,
	Decay = 6,
	Release = 7,
	SustainDecay = 8
};

enum ADSRKeyType {
	Unknown = -4,
	Restart = -3,
	Jump = -2,
	SustainNote = -1,
	StopNote = 0
};

typedef struct {
	s16 key;
	s16 value;
} envdat; /* ADSR envelope data */

typedef struct {
	struct {
		u8 unk00 : 1;
		u8 do_sustain : 1;
		u8 do_decay : 1;
		u8 do_release : 1;
	} control;
	u8 state : 4; // State of ADSR. Dictates what will be done on the current pass
} env_state;

typedef struct {
	union {
		env_state env_state;
		u8 state;
	};

	u8 env_table_idx; // Current env_data index

	s16 now_key; // Current ADSR key-type.
	f32 min_vol; // Minimum volume during decay & release
	f32 step_value; // Internal step value
	f32 decay_release; // Decay or release step value
	f32 volume; // Current volume
	f32 target_volume; // Desired target volume
	f32 unk_18; // Set to normalized volume when key == -4
	envdat* env_data; // Current ADSR environment data
} envp; /* ADSR envelope environment process structure */

