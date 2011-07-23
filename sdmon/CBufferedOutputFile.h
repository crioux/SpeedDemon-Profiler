#ifndef __INC_CBUFFEREDOUTPUTFILE_H
#define __INC_CBUFFEREDOUTPUTFILE_H

class CBufferedOutputFile
{
private:
	bool m_bIsValid;
	void *m_pBuffer;
	size_t m_nBufferTail;
	HANDLE m_hFile;

public:

	CBufferedOutputFile();
	CBufferedOutputFile(const TCHAR *fname);
	~CBufferedOutputFile();

	bool IsValid(void);
	bool Open(const TCHAR *fname);
	void Close(void);

	void Write(const void *data, size_t length);
	void Flush(void);
};


#endif