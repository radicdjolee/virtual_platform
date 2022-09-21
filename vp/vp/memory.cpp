#include "memory.hpp"
#include "address.hpp"

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace sc_dt;

memory::memory(sc_module_name name) : sc_module(name),
									  cpu_tsoc("cpu_tsoc"),
									  imdct_tsoc("imdct_tsoc")
{
	cpu_tsoc.register_b_transport(this, &memory::b_transport);
	imdct_tsoc.register_b_transport(this, &memory::b_transport);
}

void memory::b_transport(pl_t &pl, sc_time &offset)
{

	tlm_command cmd = pl.get_command();
	uint64 addr = pl.get_address();
	unsigned int len = pl.get_data_length();

	vector<int> help_vector_1;
	vector<float> help_vector_2;

	switch (cmd)
	{
	case TLM_WRITE_COMMAND:
	{
		switch (addr)
		{
		case ADDRESS_A:
			//----------------------------WRITING ARRAY FOR IMDCT(CPU)-----------------------------
			memory_1.clear();

			help_vector_1 = *((vector<int> *)pl.get_data_ptr());

			for (unsigned int i = 0; i < len; i++)
			{
				memory_1.push_back(help_vector_1[i]);
			}
			help_vector_1.clear();
			break;

		case ADDRESS_B:
			//--------------------WRITING DECODED PACKAGE(IMDCT) AND SAMPLES(CPU)------------------
			memory_2.clear();

			help_vector_2 = *((vector<float> *)pl.get_data_ptr());

			for (unsigned int i = 0; i < len; i++)
			{
				memory_2.push_back(help_vector_2[i]);
			}

			help_vector_2.clear();
			break;
		}
		break;
	}

	case TLM_READ_COMMAND:
	{
		switch (addr)
		{

		case ADDRESS_A:

			//-----------------------READING ARRAY FOR IMDCT FUNCTION(IMDCT)------------------------
			pl.set_data_ptr((unsigned char *)&memory_1);
			pl.set_data_length(memory_1.size());
			pl.set_response_status(TLM_OK_RESPONSE);

			break;

		case ADDRESS_B:

			//----------------------READING DECODED PACKAGE(CPU) AND SAMPLES(IMDCT)----------------
			pl.set_data_ptr((unsigned char *)&memory_2);
			pl.set_data_length(memory_2.size());
			pl.set_response_status(TLM_OK_RESPONSE);
			break;
		}
		break;
	}
	default:
		pl.set_response_status(TLM_COMMAND_ERROR_RESPONSE);
		SC_REPORT_ERROR("MEMORY", "TLM bad command");
	}
}

void memory::msg(const pl_t &pl)
{
}
