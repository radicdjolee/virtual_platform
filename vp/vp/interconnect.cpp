#include "interconnect.hpp"

using namespace std;
using namespace tlm;
using namespace sc_core;
using namespace sc_dt;

interconnect::interconnect(sc_module_name name) : sc_module(name)
{
	cpu_tsoc.register_b_transport(this, &interconnect::b_transport);
}

void interconnect::b_transport(pl_t &pl, sc_core::sc_time &offset)
{
	uint64 addr = pl.get_address();
	pt = *((unsigned char *)pl.get_data_ptr());

	uint64 taddr;
	offset += sc_time(0, SC_NS);

	if (addr == WRITE_PACKAGE)
	{
		cout << "INTERCONNECT" << endl;

		taddr = addr & 0x0000000F;
		pl.set_address(taddr);
		pl.set_command(TLM_WRITE_COMMAND);
		pl.set_data_length(1);
		pl.set_data_ptr((unsigned char *)&pt);
		pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

		inter_isoc->b_transport(pl, offset);
	}

	pl.set_address(addr);
}

void interconnect::msg(const pl_t &pl)
{
}
