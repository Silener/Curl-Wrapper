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

	/// <summary> Прочитане на файл от sftp </summary>
	///<param name="strFilePath"> Път до файла за четене </param>
	///<param name="strFilePathOut"> Прочетен път до файла </param>
	BOOL ReadRequest( const IN CString& strFilePath, OUT CString& strFilePathOut );

	/// <summary> Записване на файл в sftp </summary>
	///<param name="strFilePath"> Път на файла за запис пр. "XML/IN/BISERA/transfer.xml" </param>
	BOOL WriteRequest( const IN CString& strLocalFilePath, const IN CString& strFilePathToWrite );

	/// <summary> Преименува файл </summary>
	///<param name="strFilePathFrom"> Път на съществуващия файл пр. "XML/IN/BISERA/transfer.xml" </param>
	///<param name="strFilePathTo"> Път на преименувания файл пр. "XML/IN/BISERA/transfer.xml.worked </param>
	BOOL RenameRequest( const IN CString& strFilePathFrom, const IN CString& strFilePathTo );

	/// <summary> Връща името на първия необработвен файл в подадената директория </summary>
	BOOL GetUnprocessedFiles( const IN CString& strDirectory, OUT CStringArray& oFileNamesArray );

private:

	/// <summary> Прочита файл по подаден адрес </summary>
	BOOL ReadFilePath( const IN CString& strURL, OUT CString& strFilePathOut );

	/// <summary> Създава директория </summary>
	///<param name="strDirectory"> Име на директория </param>
	BOOL CreateDirectory( const IN CString& strDirectory );

	/// <summary> Callback към прочитане на подадени данни за запис </summary>
	static size_t ReadCallback( void* pBufferToUpload, size_t nBufferSize, size_t nMemoryBlockSize, void* pInputBuffer );

	/// <summary> Callback към записване на прочетени данни в буфер </summary>
	static size_t WriteCallback( void* pFileContent, size_t nSize, size_t nMemoryBlockSize, FILE* pOutputBuffer );

	/// <summary> Callback към записване на прочетени данни в буфер </summary>
	static size_t WriteDirectoryListCallback( void *ptr, size_t size, size_t nmemb, void *data );

	/// <summary> Проверява дали даденя директория съществува </summary>
	///<param name="strFilePath"> Път на файла за запис пр. "XML/IN/BISERA/transfer.xml" </param>
	///<param name="bCreateIfNotExists"> Прави опит за създаване на директория, ако я няма ( ползва се при запис на файл ) </param>
	CURLcode CheckDirectoryExists( const IN CString& strFilePath, const bool bCreateIfNotExists );

	/// <summary> Връща грешка от curl резултат </summary>
	CString GetFunctionError( const CString& strCurlOperation, const CURLcode eCurlResult );

	// Overrides
	// ----------------


	// Members
	// ----------------

	/// <summary> Име на хоста пр. "sftp://testserver60-3.csoft.bg/" ) </summary>
	CString m_strHostName;

	/// <summary> Credentials за логване в sftp пр. username:password </summary>
	CString m_strCurlFormatedCredentials;

	// MFC Macros
	// ----------------
};

#endif