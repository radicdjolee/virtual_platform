#ifndef _VP_HPP_
#define _VP_HPP_

#include "types.hpp"
#include <systemc>
#include "interconnect.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "imdct.hpp"

class vp : sc_core::sc_module
{

public:
	vp(sc_core::sc_module_name, char **arg);

protected:
	interconnect ic;
	cpu cp;
	memory mem;
	imdct hw;
};

#endif
