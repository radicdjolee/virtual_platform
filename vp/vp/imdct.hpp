#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include "types.hpp"
#include "address.hpp"
#include "tables.hpp"
#include "mp3.hpp"
#include "id3.hpp"
#include "xing.hpp"
#include "util.hpp"
#include <vector>
#include <tlm>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

class imdct : public sc_core::sc_module
{
public:
	imdct(sc_core::sc_module_name);

	tlm_utils::simple_target_socket<imdct> int_tsoc;
	tlm_utils::simple_initiator_socket<imdct> imdct_isoc;

protected:
	typedef tlm::tlm_base_protocol_types::tlm_payload_type pl_t;

	void imdct_function(int gr, int ch);
	void b_transport(pl_t &, sc_core::sc_time &);
	void msg(const pl_t &);

	std::vector<float> package;
	fixed_point samples2[2][2][576];
	fixed_point prev_samples[2][32][18];

	int block_type[2][2];
	int counter = 0;
	unsigned char gr = 0, ch = 0;
};

#endif /* MP3_DECODER_H */
