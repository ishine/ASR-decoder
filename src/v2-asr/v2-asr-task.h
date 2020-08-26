#ifndef __V2_ASR_TASK_H__
#define __V2_ASR_TASK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include "src/v2-asr/v2-asr-work-thread.h"
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

class V2ASRServiceTask:public TaskBase
{
public:
	typedef TaskBase::int32 int32;
public:
	V2ASRServiceTask(int32 connfd, std::string task_name = ""):
		TaskBase(task_name), _connfd(connfd) {}

	int32 Run(void *data);

	int32 Stop()
	{
		const char *str = "no idle thread.";
		send(_connfd, str, strlen(str), 0);
		printf("send |%d| : %s ok\n",_connfd, str);
		close(_connfd);
		return 0;
	}
	void GetInfo()
	{
		LOG_COM << "Task name is : " << _task_name ;
		LOG_COM << "Task connetid is : " << _connfd;
	}

private:
	int32 _connfd; // connect fd
	void SetConnFd(int32 connfd); // set socket id
};

int32 V2ASRServiceTask::Run(void *data)
{
	signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing when socket forcefully disconnected
	V2ASRWorkThread * asr_work_thread = static_cast<V2ASRWorkThread *>(data);
	C2SPackageAnalysis &ser_c2s = asr_work_thread->_ser_c2s_package_analysys;
	S2CPackageAnalysis &ser_s2c = asr_work_thread->_ser_s2c_package_analysys;
	OnlineClgLatticeFastDecoder &online_clg_decoder = asr_work_thread->_online_clg_decoder;
	//size_t chunk=0;
	online_clg_decoder.InitDecoding(0, true);
	int n=0; // read timeout times
	int total_wav_len = 0;
	float total_decoder_time = 0.0;
	struct timeval decoder_start, decoder_end;
	while(1)
	{

		if(true != ser_c2s.C2SRead(_connfd))
		{
			// read data
			if(errno == EAGAIN || errno == EINPROGRESS)
			{
				LOG_COM << "|" << _connfd << "| timeout and continue";
				n++;
				if(n>2)
				{
					LOG_COM << "|" << _connfd << "| timeout and disconnet";
					break;
				}
				continue;
			}
			LOG_COM << "C2SRead error." << std::endl;
			break;
		}
		ser_c2s.Print("ser_c2s");
		n=0;
		uint data_len;
		char *data = ser_c2s.GetData(&data_len);
		total_wav_len += data_len;
		// first audio type
		// todo convert to pcm format.
		//
		VLOG_COM(0) << "from |" << _connfd << "| receive \"" << data_len << "\"";
		// 8k ,16bit
		// whether to end
		bool eos=false;
		if(ser_c2s.IsEnd())
		{
			eos = true;
		}
		// get audio data type len
		uint dtype_len = ser_c2s.GetDtypeLen();

		std::string msg;
		int32 ret = 0;
		gettimeofday(&decoder_start, NULL);
		if(eos == true)
		{
			ret = online_clg_decoder.ProcessData(data, data_len, 2, dtype_len);
		}
		else
		{
			ret = online_clg_decoder.ProcessData(data, data_len, 0, dtype_len);
		}
		gettimeofday(&decoder_end, NULL);
		total_decoder_time += (decoder_end.tv_sec- decoder_start.tv_sec) +
			(decoder_end.tv_usec - decoder_start.tv_usec)/1000000;
		// according to ser_c2s request , return package.
		uint nbest = ser_c2s.GetNbest();
		
		if(nbest != 0 || eos == true)
		{// return recognition result
			if(nbest == 0)
				LOG_WARN << "Send last package and nbest request is 0,it's error.";
			nbest = nbest>1?nbest:1;
			if(eos != true && nbest != 1)
			{
				LOG_WARN << "Middle result is nbest: " << nbest ;
				nbest = 1;
			}
			// get decoder result
			if(nbest == 1)
			{
				std::string best_result;
				online_clg_decoder.GetBestPathTxt(best_result, eos);
				ser_s2c.SetNbest(best_result);
			}
			else
			{
				std::vector<std::string> nbest_result;
				online_clg_decoder.GetNbestTxt(nbest_result, nbest, eos);
				for(size_t i =0 ; i < nbest_result.size(); ++i)
				{
					ser_s2c.SetNbest(nbest_result[i]);
				}
			}
			// send result from service to client.
			if(eos == true)
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CEND))
				{
					LOG_COM << "S2CWrite all end error.";
					break;
				}
			}
			else if(ret == 1)
			{
				// end point judge cut audio.
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CMIDDLEEND))
				{
					LOG_COM << "S2CWrite middle end error.";
					break;
				}
				online_clg_decoder.InitDecoding(0, false);
			}
			else
			{
				if(true != ser_s2c.S2CWrite(_connfd, S2CPackageAnalysis::S2CNOEND))
				{
					LOG_COM << "S2CWrite error.";
					break;
				}
			}
		}
		// send ok.
		ser_s2c.Print("ser_s2c");
		// reset service to client package info.
		ser_s2c.Reset();
		if(eos == true)
		{
			uint smaple_rate = 0;
			if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_16)
				smaple_rate=16000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_8)
				smaple_rate=8000;
			else if(ser_c2s.GetSampleRate() == C2SPackageAnalysis::K_32)
				smaple_rate=32000;
			else
				break;
			float wav_time = total_wav_len*1.0/smaple_rate;
			float rt = total_decoder_time/wav_time;
			LOG_COM << "decoder rt is : " << rt ;
			break; // end
		}
	}
	close(_connfd);
	LOG_COM << "close " << _connfd << " ok.";
	return 0;
}

#include "src/util/namespace-end.h"

#endif