#ifndef _CPU_H_
#define _CPU_H_

#include "types.hpp"
#include "address.hpp"
#include "id3.hpp"
#include "mp3.hpp"
#include "imdct.hpp"
#include "xing.hpp"
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <alsa/asoundlib.h>
#include <string>
#include <fstream>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <vector>

class cpu :

	public sc_core::sc_module
{

public:
	cpu(sc_core::sc_module_name, char **arg);

	tlm_utils::simple_initiator_socket<cpu> int_isoc;
	tlm_utils::simple_initiator_socket<cpu> mem_isoc;

protected:
	void proces();

	void frequency_inversion(int gr, int ch, float (&samples2)[2][2][576]);
	void synth_filterbank(int gr, int ch, float (&samples2)[2][2][576]);

	std::vector<unsigned char> file;
	float pack[2304];
	float fifo[2][1024];
	char **arg1;

	typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;
};

std::vector<unsigned char> get_file(const char *dir);

std::vector<id3> get_id3_tags(std::vector<unsigned char> &buffer, unsigned &offset);

#endif
