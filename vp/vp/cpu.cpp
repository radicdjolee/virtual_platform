#include "cpu.hpp"

#define PI 3.141592653589793

using namespace sc_core;
using namespace sc_dt;
using namespace std;
using namespace tlm;

SC_HAS_PROCESS(cpu);
cpu::cpu(sc_module_name name, char **arg) : sc_module(name)
{
	arg1 = arg;
	SC_THREAD(proces);
}

void cpu::proces()
{
	int cycle_counter = 0;
	int package_counter = 0;

	sc_core::sc_time start;
	sc_core::sc_time end;

	sc_time loct;
	tlm_generic_payload pl;
	tlm_utils::tlm_quantumkeeper qk;
	qk.reset();

	//--------------------------------------LOADING MP3 FILE---------------------------------------

	std::vector<unsigned char> buffer = get_file(arg1[1]);
	vector<float> help_vector;
	unsigned offset = 0;

	cout << "CPU: CODED MP3 FILE IS LOADED" << endl;

	//--------------------------------------CREATING DECODER---------------------------------------

	std::vector<id3> tags = get_id3_tags(buffer, offset);
	mp3 decoder(&buffer[offset]);

	//----------------------------CPU: INITIALIZING ALSA SOUND LIBRARY-----------------------------

	unsigned sampling_rate = 44100;
	unsigned channels = 2;
	cout << "sampling_rate is: :" << sampling_rate << endl;
	cout << "channels is:" << channels << endl;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hw = NULL;
	snd_pcm_uframes_t frames = 128;

	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
		exit(1);

	snd_pcm_hw_params_alloca(&hw);
	snd_pcm_hw_params_any(handle, hw);

	if (snd_pcm_hw_params_set_access(handle, hw, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		exit(1);
	if (snd_pcm_hw_params_set_format(handle, hw, SND_PCM_FORMAT_FLOAT_LE) < 0)
		exit(1);
	if (snd_pcm_hw_params_set_channels(handle, hw, channels) < 0)
		exit(1);
	if (snd_pcm_hw_params_set_rate_near(handle, hw, &sampling_rate, NULL) < 0)
		exit(1);
	if (snd_pcm_hw_params_set_period_size_near(handle, hw, &frames, NULL) < 0)
		exit(1);
	if (snd_pcm_hw_params(handle, hw) < 0)
		exit(1);
	if (snd_pcm_hw_params_get_period_size(hw, &frames, NULL) < 0)
		exit(1);
	if (snd_pcm_hw_params_get_period_time(hw, &sampling_rate, NULL) < 0)
		exit(1);

	//----------------------------------------DECODING---------------------------------------------
	start = sc_core::sc_time_stamp();

	while (decoder.is_valid() && buffer.size() > offset + decoder.get_header_size())
	{
		decoder.init_header_params(&buffer[offset]);

		if (decoder.is_valid())
		{

			decoder.init_frame_params(&buffer[offset]);

			for (int gr = 0; gr < 2; gr++)
			{

				for (int ch = 0; ch < 2; ch++)
					decoder.requantize(gr, ch);

				if (decoder.channel_mode == mp3::JointStereo && decoder.mode_extension[0])
					decoder.ms_stereo(gr);

				for (int ch = 0; ch < 2; ch++)
				{
					if (decoder.block_type[gr][ch] == 2 || decoder.mixed_block_flag[gr][ch])
						decoder.reorder(gr, ch);
					else
						decoder.alias_reduction(gr, ch);

					std::vector<float> package;
					std::vector<int> help_block;

					for (int x = 0; x < 2; x++)
						for (int y = 0; y < 2; y++)
							help_block.push_back(decoder.block_type[x][y]);

					cout << "\e[1;1H\e[2J"; // alignment
					cout << "-----------------------------------------------------------------------" << endl;
					cout << "CPU: SENDING ARRAY FOR IMDCT TO MEMORY" << endl;

					cycle_counter++;

					pl.set_address(ADDRESS_A);
					pl.set_command(TLM_WRITE_COMMAND);
					pl.set_data_length(help_block.size());
					pl.set_data_ptr((unsigned char *)&help_block);
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

					mem_isoc->b_transport(pl, loct);
					qk.set_and_sync(loct);
					loct += sc_time(10, SC_NS);
					help_block.clear();

					for (int f = 0; f < 2; f++)
						for (int h = 0; h < 576; h++)
							for (int g = 0; g < 2; g++)
								package.push_back(decoder.samples[f][g][h]);

					cout << "CPU: SENDING SAMPLES TO MEMORY" << endl;

					cycle_counter++;

					pl.set_address(ADDRESS_B);
					pl.set_command(TLM_WRITE_COMMAND);
					pl.set_data_length(package.size());
					pl.set_data_ptr((unsigned char *)&package);
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

					mem_isoc->b_transport(pl, loct);
					qk.set_and_sync(loct);
					loct += sc_time(10, SC_NS);
					package.clear();

					unsigned char a = (unsigned char)gr, b = (unsigned char)ch;

					cout << "CPU: SENDING GR PARAMETER AND DECODING INSTRUCTION TO IMDCT" << endl;

					cycle_counter++;

					pl.set_address(WRITE_PACKAGE);
					pl.set_command(TLM_WRITE_COMMAND);
					pl.set_data_length(1);
					pl.set_data_ptr((unsigned char *)&a);
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

					int_isoc->b_transport(pl, loct);
					qk.set_and_sync(loct);
					loct += sc_time(10, SC_NS);

					cout << "CPU: SENDING CH PARAMETER AND DECODING INSTRUCTION TO IMDCT" << endl;

					cycle_counter += 4; // adittional 3 cycles from imdct module

					pl.set_address(WRITE_PACKAGE);
					pl.set_command(TLM_WRITE_COMMAND);
					pl.set_data_length(1);
					pl.set_data_ptr((unsigned char *)&b);
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

					int_isoc->b_transport(pl, loct);
					qk.set_and_sync(loct);
					loct += sc_time(10, SC_NS);

					cout << "CPU: RECEIVING DECODED PACKAGE" << endl;

					cycle_counter++;

					package_counter++;

					cout << "NUMBER OF PACKAGES: " << package_counter << endl;

					pl.set_address(ADDRESS_B);
					pl.set_command(TLM_READ_COMMAND);
					pl.set_response_status(TLM_INCOMPLETE_RESPONSE);

					mem_isoc->b_transport(pl, loct);
					qk.set_and_sync(loct);
					loct += sc_time(10, SC_NS);

					help_vector = *((vector<float> *)pl.get_data_ptr());
					int sz = help_vector.size();
					cout << endl;
					cout << "Samples:" << endl;
					for (int i = 0; i < sz; i++)
					{
						pack[i] = help_vector[i];

						if (i < 10)
							cout << pack[i] << endl;
					}

					help_vector.clear();

					cout << "-----------------------------------------------------------------------" << endl;

					int h = 0;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 576; j++)
							for (int k = 0; k < 2; k++)
								decoder.samples[i][k][j] = pack[h++];
					//-----------------------------------------------------------------------------
					frequency_inversion(gr, ch, decoder.samples);
					synth_filterbank(gr, ch, decoder.samples);
					//-----------------------------------------------------------------------------
					h = 0;
					for (int i = 0; i < 2; i++)
						for (int j = 0; j < 576; j++)
							for (int k = 0; k < 2; k++)
								pack[h++] = decoder.samples[i][k][j];
				}
			}
			offset += decoder.get_frame_size();
		}

		//--------------------------------STREAMING DECODED PACKAGE--------------------------------

		int e = snd_pcm_writei(handle, pack, 1152);
		if (e == -EPIPE)
			snd_pcm_recover(handle, e, 0);
		//-----------------------------------------------------------------------------------------
	}

	end = sc_core::sc_time_stamp();
	double t = (end - start).to_seconds();
	double total_data_in = (2304 * 15 + 32 * 4) * package_counter;
	double total_data_in_MB = total_data_in / (8 * 1024 * 1024);
	double total_data_out = 2304 * 15 * package_counter;
	double total_data_out_MB = total_data_out / (8 * 1024 * 1024);

	cout << "Number of cycles: " << cycle_counter << endl;
	cout << "Total elapsed time: " << t << " s" << endl;
	cout << "Throughput based on Sysc time(input bus)=" << total_data_in_MB / t << " MB/s" << endl;
	cout << "Throughput in cycles(input bus)=" << total_data_in / cycle_counter << " bits per cycle" << endl;
	cout << "Throughput based on Sysc time(output bus)=" << total_data_out_MB / t << " MB/s" << endl;
	cout << "Throughput in cycles(output bus)=" << total_data_out / cycle_counter << " bits per cycle" << endl;

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
}

std::vector<id3> get_id3_tags(std::vector<unsigned char> &buffer, unsigned &offset)
{
	std::vector<id3> tags;
	int i = 0;
	bool valid = true;

	while (valid)
	{
		id3 tag(&buffer[offset]);
		valid = tag.is_valid(); // changed

		if (valid)
		{
			tags.push_back(tag);
			offset += tags[i++].get_id3_offset() + 10;
		}
	}

	return tags;
}

std::vector<unsigned char> get_file(const char *dir)
{
	std::ifstream file(dir, std::ios::in | std::ios::binary | std::ios::ate);
	std::vector<unsigned char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read((char *)buffer.data(), buffer.size());
	file.close();
	return buffer; // changed
}

void cpu::frequency_inversion(int gr, int ch, float (&samples2)[2][2][576])
{
	for (int sb = 1; sb < 18; sb += 2)
		for (int i = 1; i < 32; i += 2)
			samples2[gr][ch][i * 18 + sb] *= -1;
}

/**
 * @param gr
 * @param ch
 */
void cpu::synth_filterbank(int gr, int ch, float (&samples2)[2][2][576])
{
	static float n[64][32];
	static bool init = true;

	if (init)
	{
		init = false;
		for (int i = 0; i < 64; i++)
			for (int j = 0; j < 32; j++)
				n[i][j] = std::cos((16.0 + i) * (2.0 * j + 1.0) * (PI / 64.0));
	}

	float s[32], u[512], w[512];
	float pcm[576];

	for (int sb = 0; sb < 18; sb++)
	{
		for (int i = 0; i < 25; i++)
			s[i] = samples2[gr][ch][i * 18 + sb];

		for (int i = 1023; i > 63; i--)
			fifo[ch][i] = fifo[ch][i - 64];

		for (int i = 0; i < 64; i++)
		{
			fifo[ch][i] = 0;
			for (int j = 0; j < 32; j++)
				fifo[ch][i] += s[j] * n[i][j];
		}

		for (int i = 0; i < 8; i++)
			for (int j = 0; j < 32; j++)
			{
				u[i * 64 + j] = fifo[ch][i * 128 + j];
				u[i * 64 + j + 32] = fifo[ch][i * 128 + j + 96];
			}

		for (int i = 0; i < 512; i++)
			w[i] = u[i] * synth_window[i];

		for (int i = 0; i < 32; i++)
		{
			float sum = 0;
			for (int j = 0; j < 16; j++)
				sum += w[j * 32 + i];
			pcm[32 * sb + i] = sum;
		}
	}

	for (int z = 0; z < 576; z++)
		samples2[gr][ch][z] = pcm[z];
}
