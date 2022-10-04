#pragma once

#ifdef _WIN64

#include "curl.h"

/////////////////////////////////////////////////////////////////////////////
// CCurlWrapper

struct CURL_TRANSFER_BUFFER
{
	BYTE* pData;
	size_t nSize;

	CURL_TRANSFER_BUFFER(){}
};

class CSSYSCOREsEXP CCurlWrapper
{
	// Constants
	// ----------------


	// Constructor / Destructor
	// ----------------
public:
	CCurlWrapper( CString strHostName, CString strUsername, CString strPassword );
	virtual ~CCurlWrapper();

	// Methods
	// ----------------

	/// <summary> ��������� �� ���� �� sftp </summary>
	///<param name="strFilePath"> ��� �� ����� �� ������ </param>
	///<param name="strFilePathOut"> �������� ��� �� ����� </param>
	BOOL ReadRequest( const IN CString& strFilePath, OUT CString& strFilePathOut );

	/// <summary> ��������� �� ���� � sftp </summary>
	///<param name="strFilePath"> ��� �� ����� �� ����� ��. "XML/IN/BISERA/transfer.xml" </param>
	BOOL WriteRequest( const IN CString& strLocalFilePath, const IN CString& strFilePathToWrite );

	/// <summary> ���������� ���� </summary>
	///<param name="strFilePathFrom"> ��� �� ������������� ���� ��. "XML/IN/BISERA/transfer.xml" </param>
	///<param name="strFilePathTo"> ��� �� ������������� ���� ��. "XML/IN/BISERA/transfer.xml.worked </param>
	BOOL RenameRequest( const IN CString& strFilePathFrom, const IN CString& strFilePathTo );

	/// <summary> ����� ����� �� ������ ������������ ���� � ���������� ���������� </summary>
	BOOL GetUnprocessedFiles( const IN CString& strDirectory, OUT CStringArray& oFileNamesArray );

private:

	/// <summary> ������� ���� �� ������� ����� </summary>
	BOOL ReadFilePath( const IN CString& strURL, OUT CString& strFilePathOut );

	/// <summary> ������� ���������� </summary>
	///<param name="strDirectory"> ��� �� ���������� </param>
	BOOL CreateDirectory( const IN CString& strDirectory );

	/// <summary> Callback ��� ��������� �� �������� ����� �� ����� </summary>
	static size_t ReadCallback( void* pBufferToUpload, size_t nBufferSize, size_t nMemoryBlockSize, void* pInputBuffer );

	/// <summary> Callback ��� ��������� �� ��������� ����� � ����� </summary>
	static size_t WriteCallback( void* pFileContent, size_t nSize, size_t nMemoryBlockSize, FILE* pOutputBuffer );

	/// <summary> Callback ��� ��������� �� ��������� ����� � ����� </summary>
	static size_t WriteDirectoryListCallback( void *ptr, size_t size, size_t nmemb, void *data );

	/// <summary> ��������� ���� ������ ���������� ���������� </summary>
	///<param name="strFilePath"> ��� �� ����� �� ����� ��. "XML/IN/BISERA/transfer.xml" </param>
	///<param name="bCreateIfNotExists"> ����� ���� �� ��������� �� ����������, ��� � ���� ( ������ �� ��� ����� �� ���� ) </param>
	CURLcode CheckDirectoryExists( const IN CString& strFilePath, const bool bCreateIfNotExists );

	/// <summary> ����� ������ �� curl �������� </summary>
	CString GetFunctionError( const CString& strCurlOperation, const CURLcode eCurlResult );

	// Overrides
	// ----------------


	// Members
	// ----------------

	/// <summary> ��� �� ����� ��. "sftp://testserver60-3.csoft.bg/" ) </summary>
	CString m_strHostName;

	/// <summary> Credentials �� ������� � sftp ��. username:password </summary>
	CString m_strCurlFormatedCredentials;

	// MFC Macros
	// ----------------
};

#endif