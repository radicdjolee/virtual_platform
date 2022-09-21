#include "imdct.hpp"
#define PI 3.141592653589793
#define SQRT2 1.414213562373095

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace sc_dt;

SC_HAS_PROCESS(imdct);
imdct::imdct(sc_module_name name) : sc_module(name),
									int_tsoc("int_tsoc"),
									imdct_isoc("imdct_isoc")
{
	int_tsoc.register_b_transport(this, &imdct::b_transport);
}

sc_time loct;
tlm_generic_payload pl;
tlm_utils::tlm_quantumkeeper qk;

void imdct::b_transport(pl_t &pl, sc_time &offset)
{

	tlm_command cmd = pl.get_command();
	uint64 addr = pl.get_address();

	vector<float> help_vector;
	vector<int> help_vector2;

	switch (cmd)
	{

	case TLM_WRITE_COMMAND:
	{

		switch (addr)
		{

		case ADRESS_ONE:
		{
			package.clear();

			if (counter % 2 == 0)
			{
				cout << "IMDCT:RECEIVING GR FROM CPU" << endl;
				gr = *((unsigned char *)pl.get_data_ptr());

				counter++;
			}
			else
			{
				cout << "IMDCT:RECEIVING CH FROM CPU" << endl;
				ch = *((unsigned char *)pl.get_data_ptr());

				counter++;

				cout << "IMDCT:RECEIVING SAMPLES FROM MEMORY" << endl;

				pl.set_address(ADDRESS_B);
				pl.set_command(TLM_READ_COMMAND);
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

				imdct_isoc->b_transport(pl, loct);
				qk.set_and_sync(loct);
				loct += sc_time(10, SC_NS);

				help_vector = *((vector<float> *)pl.get_data_ptr());

				int q = 0;
				for (int f = 0; f < 2; f++)
					for (int h = 0; h < 576; h++)
						for (int g = 0; g < 2; g++)
							samples2[f][g][h] = help_vector[q++];

				help_vector.clear();

				cout << "IMDCT: RECEIVING ARRAY FOR IMDCT FROM MEMORY " << endl;

				pl.set_address(ADDRESS_A);
				pl.set_command(TLM_READ_COMMAND);
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

				imdct_isoc->b_transport(pl, loct);
				qk.set_and_sync(loct);
				loct += sc_time(10, SC_NS);

				help_vector2 = *((vector<int> *)pl.get_data_ptr());

				int w = 0;
				for (int f = 0; f < 2; f++)
					for (int g = 0; g < 2; g++)
						block_type[f][g] = help_vector2[w++];

				help_vector2.clear();

				//------------------------------------DECODING-------------------------------------

				imdct_function(gr, ch);

				//---------------------------------------------------------------------------------

				for (int f = 0; f < 2; f++)
					for (int h = 0; h < 576; h++)
						for (int g = 0; g < 2; g++)
							package.push_back(samples2[f][g][h]);

				cout << "IMDCT: WRITING DECODED PACKAGE TO MEMORY " << endl;
				pl.set_response_status(TLM_OK_RESPONSE);

				pl.set_address(ADDRESS_B);
				pl.set_command(TLM_WRITE_COMMAND);
				pl.set_data_length(package.size());
				pl.set_data_ptr((unsigned char *)&package);
				pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

				imdct_isoc->b_transport(pl, loct);
				qk.set_and_sync(loct);
				loct += sc_time(10, SC_NS);
			}
		}
		break;
		}
		break;
	}
	default:
		pl.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
	}
}

void imdct::msg(const pl_t &pl)
{
}

void imdct::imdct_function(int gr, int ch)
{
	static bool init = true;
	static fixed_point sine_block[4][36];
	fixed_point sample_block[36];

	if (init)
	{
		int i;
		for (i = 0; i < 36; i++)
			sine_block[0][i] = std::sin(PI / 36.0 * (i + 0.5));
		for (i = 0; i < 18; i++)
			sine_block[1][i] = std::sin(PI / 36.0 * (i + 0.5));
		for (; i < 24; i++)
			sine_block[1][i] = 1.0;
		for (; i < 30; i++)
			sine_block[1][i] = std::sin(PI / 12.0 * (i - 18.0 + 0.5));
		for (; i < 36; i++)
			sine_block[1][i] = 0.0;
		for (i = 0; i < 12; i++)
			sine_block[2][i] = std::sin(PI / 12.0 * (i + 0.5));
		for (i = 0; i < 6; i++)
			sine_block[3][i] = 0.0;
		for (; i < 12; i++)
			sine_block[3][i] = std::sin(PI / 12.0 * (i - 6.0 + 0.5));
		for (; i < 18; i++)
			sine_block[3][i] = 1.0;
		for (; i < 36; i++)
			sine_block[3][i] = std::sin(PI / 36.0 * (i + 0.5));
		init = false;
	}

	const int n = block_type[gr][ch] == 2 ? 12 : 36;
	const int half_n = n / 2;
	int sample = 0;

	for (int block = 0; block < 32; block++)
	{
		for (int win = 0; win < (block_type[gr][ch] == 2 ? 3 : 1); win++)
		{
			for (int i = 0; i < n; i++)
			{
				float xi = 0.0;
				for (int k = 0; k < half_n; k++)
				{
					float s = samples2[gr][ch][18 * block + half_n * win + k];
					xi += s * std::cos(PI / (2 * n) * (2 * i + 1 + half_n) * (2 * k + 1));
				}

				/* Windowing samples. */
				sample_block[win * n + i] = xi * sine_block[block_type[gr][ch]][i];
			}
		}

		if (block_type[gr][ch] == 2)
		{
			fixed_point temp_block[36];

			for (int x = 0; x < 36; x++)
				temp_block[x] = sample_block[x];

			int i = 0;
			for (; i < 6; i++)
				sample_block[i] = 0;
			for (; i < 12; i++)
				sample_block[i] = temp_block[0 + i - 6];
			for (; i < 18; i++)
				sample_block[i] = temp_block[0 + i - 6] + temp_block[12 + i - 12];
			for (; i < 24; i++)
				sample_block[i] = temp_block[12 + i - 12] + temp_block[24 + i - 18];
			for (; i < 30; i++)
				sample_block[i] = temp_block[24 + i - 18];
			for (; i < 36; i++)
				sample_block[i] = 0;
		}

		/* Overlap. */
		for (int i = 0; i < 18; i++)
		{
			samples2[gr][ch][sample + i] = sample_block[i] + prev_samples[ch][block][i];
			prev_samples[ch][block][i] = sample_block[18 + i];
		}
		sample += 18;
	}
}
