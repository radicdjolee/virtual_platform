#include "vp.hpp"

using namespace sc_core;

vp::vp(sc_module_name name, char **arg) : sc_module(name),
										  cp("cpu", arg),
										  ic("interconnect"),
										  hw("imdct"),
										  mem("memory")
{
	cp.mem_isoc.bind(mem.cpu_tsoc);
	cp.int_isoc.bind(ic.cpu_tsoc);

	ic.inter_isoc.bind(hw.int_tsoc);

	hw.imdct_isoc.bind(mem.imdct_tsoc);

	SC_REPORT_INFO("VP", "Platform is constructed");
}
