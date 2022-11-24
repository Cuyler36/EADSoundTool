#include "sound.h"

extern void Nas_EnvInit(envp* envp, envdat* dat, short* unused) {
	envp->state = 0;
	envp->now_key = 0;
	envp->min_vol = 0.0f;
	envp->volume = 0.0f;
	envp->step_value = 0.0f;
	envp->decay_release = 0.0f;
	envp->env_data = dat;
}

extern f32 Nas_EnvProcess(envp* envp) {
	env_state now_state = envp->env_state;
	switch (now_state.state) {
	case ADSRState::Stopped:
		return 0.0f;
	case ADSRState::CheckSustainBeforeProcessing:
		if (now_state.control.do_sustain != FALSE) {
			envp->env_state.state = ADSRState::Sustain;
			break;
		}
		/* Fall-through to case 2 */
	case ADSRState::InitProcessing:
		envp->env_table_idx = 0;
		envp->env_state.state = ADSRState::ProcessState;
		/* Fall-through to case 3 */
	case ADSRState::ProcessState:
		int key;
		while (TRUE) {
			while (TRUE) {
				envp->now_key = envp->env_data[envp->env_table_idx].key;
				key = envp->now_key;
				if (key != ADSRKeyType::Jump) {
					break;
				}
				envp->env_table_idx = envp->env_data[envp->env_table_idx].value; // Jump to new index
			}

			if (key == ADSRKeyType::StopNote) {
				envp->env_state.state = ADSRState::Stopped; // Stop processing
			}
			else if (key == ADSRKeyType::SustainNote) {
				envp->env_state.state = ADSRState::Sustain; // Sustain
			}
			else if (key == ADSRKeyType::Unknown) {
				envp->unk_18 = (f32)envp->env_data[envp->env_table_idx].value / VOLUME_MAX;
				envp->env_state.control.unk00 = TRUE;
				envp->env_table_idx++; // Move to next envelope data
			}

			if (key != ADSRKeyType::Unknown) {
				break;
			}
		}

		if (key != ADSRKeyType::StopNote && key != ADSRKeyType::SustainNote) {
			if (key < ADSRKeyType::Unknown || key > ADSRKeyType::StopNote) {
				/* Time-Volume key-value pairs */

				envp->now_key = (f32)key * 0.0f; // TODO: Should be multiplied by a value in AG (Audio Globals)
				if (envp->now_key == 0) {
					envp->now_key = 1; // Minimum time is 1 tick
				}

				envp->target_volume = (f32)envp->env_data[envp->env_table_idx].value / VOLUME_MAX;
				envp->target_volume = envp->target_volume * envp->target_volume;
				envp->step_value = (envp->target_volume - envp->volume) / (f32)envp->now_key;
				envp->env_state.state = ADSRState::Attack;
				envp->env_table_idx++;
			}
			else {
				envp->env_state.state = ADSRState::CheckSustainBeforeProcessing;
			}
		}

		if (envp->env_state.state != ADSRState::Attack) {
			break;
		}
		/* Fall-through to case 4 */
	case ADSRState::Attack:
		envp->volume += envp->step_value;
		s16 sustain_time = envp->now_key - 1;
		envp->now_key = sustain_time;
		if (sustain_time < 1) {
			envp->env_state.state = ADSRState::ProcessState; // Done transitioning
		}
		break;

	case ADSRState::Decay:
	case ADSRState::Release:
		envp->volume -= envp->decay_release;
		f32 min_vol = envp->min_vol;

		if (min_vol == 0.0f || now_state.state != ADSRState::Decay) {
			if (envp->volume < 0.00001f) {
				envp->volume = 0.0f;
				envp->env_state.state = ADSRState::Stopped; // Stopped
			}
		}
		else if (envp->volume < min_vol) {
			/* Sustain minimum volume for 128 ticks? */
			envp->volume = min_vol;
			envp->now_key = 128;
			envp->env_state.state = ADSRState::SustainDecay;
		}
		break;

	case ADSRState::SustainDecay:
		envp->now_key--;
		if (envp->now_key == 0) {
			envp->env_state.state = ADSRState::Release;
		}
		break;
	}

	/* Check if state is forced to decay -- set in __Nas_Release_Channel_Main */
	if (envp->env_state.control.do_decay != FALSE) {
		envp->env_state.state = ADSRState::Decay;
		envp->env_state.control.do_decay = FALSE;
	}

	/* Check if state is forced to release -- set in __Nas_InterReleaseTrack & __Nas_Release_Channel_Main */
	if (envp->env_state.control.do_release != FALSE) {
		envp->env_state.state = ADSRState::Release;
		envp->env_state.control.do_release = FALSE;
	}

	/* Clamp volume in range [0.0f, 1.0f] */
	f32 vol = 0.0f;
	if (envp->volume >= 0.0f) {
		vol = envp->volume;
		if (vol > 1.0f) {
			vol = 1.0f;
		}
	}

	return vol;
}
