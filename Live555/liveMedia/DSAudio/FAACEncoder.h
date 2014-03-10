#ifndef _FAAC_ENCODER_H_
#define _FAAC_ENCODER_H_

#include "faac.h"

class FAACEncoder
{
public:
    FAACEncoder();

    ~FAACEncoder();

    /*
    初始化
    @param samRate 采样率 
    @param channels 通道数
    @param bitsPerSample 采样位数，一般是16(16位USHORT)
    @return void 
    */
    void Init(unsigned int samRate, unsigned int channels, int bitsPerSample);

    /*
    FAAC编码函数
    @param inputBuf 输入缓冲区 
    @param samCount 采样数，详见InputSamples
    @param outBuf 输出缓冲区，最大大小详见MaxOutBytes
    @param bufSize [out] 输出编码后的大小
    @return void 
    */
    void Encode(unsigned char* inputBuf, unsigned int samCount, unsigned char* outBuf, unsigned int& bufSize);
	/*
	在Init后填充，表示每次编码该放入的采样数(samCount)
	*/
    unsigned long InputSamples() { return input_sams_; }
	/*
	在Init后填充，表示编码缓冲区的最大大小(bufSize初始值)
	*/
    unsigned long MaxOutBytes() { return max_output_bytes_; }
	/// 释放函数
    void Destroy();

private:
    faacEncHandle faac_handle_;
    unsigned long input_sams_;
    unsigned long max_output_bytes_;
};

#endif // _FAAC_ENCODER_H_
