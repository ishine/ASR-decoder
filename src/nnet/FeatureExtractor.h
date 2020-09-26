#ifndef   __aaFEATURE_EXTACTOR_Haa__
#define  __aaFEATURE_EXTACTOR_Haa__


namespace tasr{
enum EnumFeatError
{
	FEATEXACTOR_SUCC = 0,	         //�������ɹ�
	FEATEXACTOR_BUFF_OVERFLOW = -1,  //��Ƶ���ݻ��������ݴ洢buff���
	FEATEXACTOR_FAIL = -2	         //��ȡ����ʧ�ܣ������ϲ��������ִ��󣬳��Ǵ���Ĳ������Ϸ�
};

class FrontEnd;
class FeatureExtractor
{
public:
	/*
	��ʼ������������0��ʾ�ɹ�������-1��ʾʧ��
	Input:    �����ļ�·��
	Output:   
	*/

	FeatureExtractor();
	~FeatureExtractor();
	int Init(const char*cfgPath,const char *sysDir);

	
	/*
	���ʼ������
	*/
	void Uninit();
    
	/*
	���ò�����ÿ�仰��ʼ������ã� ׼��Ϊ��һ������������.����0��ʾ�ɹ�������-1��ʾʧ��
	*/
	int Reset();

	/*
	�������ݵĴ洢buff�Ͷ�Ӧ��С[���ֽ�Ϊ��λ]���ռ�����߿��ٺ��ͷţ���СҲ����߾�����
	����0��ʾ�ɹ�������-1��ʾʧ��.
	buff�������ǣ���������������ʱ����ȡ�����������ݷ���featbuff��
	*/
	int SetBuff(void* featbuff,int featbuff_size);

	/*
	��ȡ������ά�������������ļ��������������õõ�
	*/
	int GetFeatureDim();


	/*TASRParamKind */
	int GetTgtKind();

	/*
	  ��ȡ��Ƶ�Ĳ�����, ������ʵ���ϴ������ļ����ã�һ��8000����16000
	*/
	int GetSrcSampleRate();

	/*
	  ��ȡ֡�����ʣ�һ��ֵ��100
	*/
	int GetTgtFrmRate();

	/*
	������ȡ������wavdata�����ͽ�������Ƶ���ݣ�wav_len���䳤�ȣ����ֽ�Ϊ��λ,
	����ȡ��������׷�ӵ�SetBufʱ���õ�featbuff�featFrmNum������ȡ��������������,��֡Ϊ��λ��
	������������֮ǰ����Ҫ�����featbuff�Ƿ��������������򲻽�������������������FEATEXACTOR_BUFF_OVERFLOW��
	����������ȡ���ɹ�����FEATEXACTOR_SUCC��ʧ�ܷ���FEATEXACTOR_FAIL��
	��������������ݲ�����ȡ1֡��������Ҳ����FEATEXACTOR_SUCC��ͬʱfeature_len = 0��
	wav_len����Ϊ0��wavdata����ΪNULL������FEATEXACTOR_SUCC��ͬʱfeature_len = 0��
	*/
	int ExtractFeat(char* wavdata,int wav_len,int& featFrmNum);

	/*
	�ú���ͬExtractFeat��Ψһ������ExtractFeat_Last���յ����������źź����ȡ��
	*/
	int ExtractFeat_Last(char* wavdata,int wav_len,int& featFrmNum);

private:
	//The external memory
	void * m_FeatBuf;
	int    m_FeatBufSize;


	//The actually object
	FrontEnd *m_pFrontEnd;
};
}
#endif