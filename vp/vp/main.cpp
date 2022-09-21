/*
 * A simplistic MPEG-1 layer 3 decoder.
 */
#include "types.hpp"
#include "vp.hpp"
#include <systemc>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API

int sc_main(int argc, char **argv)
{
	vp uut("uut", argv);
	sc_start(15, sc_core::SC_SEC);

	return 0;
}
