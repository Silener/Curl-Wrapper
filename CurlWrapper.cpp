#include "stdafx.h"

#ifdef _WIN64

#include "CurlWrapper.h"
#include <wchar.h>
#include <WinBase.h>
#include "PathParser.h"
#include "ApplicationDirectories.h"

/////////////////////////////////////////////////////////////////////////////
// CCurlWrapper

#define SFTP_PATH_DELIMITER _T("/")

// Constructor / Destructor
// ----------------

CCurlWrapper::CCurlWrapper( CString strHostName, CString strUsername, CString strPassword )
	: m_strHostName( strHostName )
{
	m_strCurlFormatedCredentials = CSMsg::ComposeString( _T("{USERNAME}:{PASSWORD}"), &MSGKEY( _T("USERNAME"), strUsername )
		, &MSGKEY( _T("PASSWORD"), strPassword ) );

	curl_global_init(CURL_GLOBAL_ALL);
}

CCurlWrapper::~CCurlWrapper()
{
	curl_global_cleanup();
}

// Methods
// ----------------
size_t CCurlWrapper::WriteCallback( void* pFileContent, size_t nSize, size_t nMemoryBlockSize, FILE* pOutputBuffer )
{
	size_t written = fwrite(pFileContent, nSize, nMemoryBlockSize, pOutputBuffer);
	return written;
}

BOOL CCurlWrapper::ReadFilePath( const CString& strURL, OUT CString& strFilePathOut )
{
	CURLcode eCurlResult = CURLE_OK;
	CURL* pCurlHandle = curl_easy_init();
	if( pCurlHandle == NULL )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return FALSE;
	}// if

	CPathParser oPathParser( strURL );

	CString strTempFilePath = CSMsg::ComposeString( _T("{FILE_PATH}{FILE_NAME}")
													, &MSGKEY( _T("FILE_PATH"), CApplicationDirectories::GetApplicationDirectory( TempDirectory ) )
													, &MSGKEY( _T("FILE_NAME"), oPathParser.GetFilename() ) );

	// ������� �� ����, ����� �� ��������, �� �� �������� �����
	strFilePathOut = strTempFilePath;

	FILE* pFile = NULL;
	
	SYS_TRY
	{
		fopen_s( &pFile, strTempFilePath, "w+b");

		if( pFile == NULL )
		{
			CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
				&MSGKEY(_T("0"), __FUNCTION__),
				&MSGKEY(_T("1"), _T("pFile == NULL")));

			return FALSE;
		}// if

		// �������� ���� + ���� �� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, strURL );

		// Credentials �� �������
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Callback ������� �� ������ �� ����������� ����� � �������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);

		// ������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEDATA, pFile);

		curl_easy_setopt( pCurlHandle, CURLOPT_FOLLOWLOCATION, 1L);

		eCurlResult = curl_easy_perform(pCurlHandle);

		if( eCurlResult == CURLE_FAILED_INIT )
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);

			eCurlResult = curl_easy_perform( pCurlHandle );
		}// if

	}// SYS_TRY
	SYS_CATCH(__FUNCTION__)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

		if( pFile != NULL )
			fclose( pFile );

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("curl_easy_perform"), eCurlResult ) ) );

		if( pFile != NULL )
			fclose( pFile );

		curl_easy_cleanup(pCurlHandle);

		return FALSE;
	}// if

	if( pFile != NULL )
		fclose( pFile );

	curl_easy_cleanup(pCurlHandle);

	return TRUE;
}

BOOL CCurlWrapper::ReadRequest( const IN CString& strFilePath, OUT CString& strFilePathOut )
{
	CURLcode eCurlResult = CURLE_OK;

	// ����������� ���� ������������ ����������
	eCurlResult = CheckDirectoryExists( strFilePath, false );

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("CheckDirectoryExists"), eCurlResult ) ) );

		return FALSE;
	}// if

	CString strReadURL;
	strReadURL.Append( m_strHostName );
	strReadURL.Append( strFilePath );

	strReadURL.Trim();

	if( !ReadFilePath( strReadURL, strFilePathOut ) )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("ReadFilePath")));

		return FALSE;
	}// if

	return TRUE;
}

CString CCurlWrapper::GetFunctionError( const CString& strCurlOperation, const CURLcode eCurlResult )
{
	CString strError;

	strError = CSMsg::ComposeString(_T("Operation:{CURL_OPERATION}, ErrorCode:{CURL_ERROR_CODE}, ErrorText:{CURL_ERROR}")
		, &MSGKEY(_T("CURL_OPERATION"), strCurlOperation)
		, &MSGKEY(_T("CURL_ERROR"), curl_easy_strerror(eCurlResult))
		, &MSGKEY(_T("CURL_ERROR_CODE"), CToStr::Parse((long)eCurlResult))
	);

	return strError;
}

size_t CCurlWrapper::ReadCallback( void* pBufferToUpload, size_t nBufferSize, size_t nMemoryBlockSize, void* pInputBuffer )
{
	size_t written = fread( pBufferToUpload, nBufferSize, nMemoryBlockSize, (FILE*)pInputBuffer );
	return written;
}

BOOL CCurlWrapper::WriteRequest( const IN CString& strLocalFilePath, const IN CString& strFilePathToWrite )
{
	CURLcode eCurlResult = CURLE_OK;

	// �� �������� ���� ����� ����������
	eCurlResult = CheckDirectoryExists( strFilePathToWrite, true );

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("CheckDirectoryExists"), eCurlResult ) ) );

		return FALSE;
	}// if

	CURL* pCurlHandle = curl_easy_init();
	if( pCurlHandle == NULL )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return FALSE;
	}// if

	CString strFullURL;
	strFullURL.Append( m_strHostName );
	strFullURL.Append( strFilePathToWrite );
	strFullURL.Trim();

	FILE* pFile = NULL;

	SYS_TRY
	{
		fopen_s( &pFile, strLocalFilePath, "r+b");

		if( pFile == NULL )
		{
			CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
				&MSGKEY(_T("0"), __FUNCTION__),
				&MSGKEY(_T("1"), _T("pFile == NULL")));

			curl_easy_cleanup( pCurlHandle );

			return FALSE;
		}// if

		// �������� �����
		curl_easy_setopt(pCurlHandle, CURLOPT_URL, strFullURL );

		// Credentials �� �������
		curl_easy_setopt(pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// �������, �� �� upload-����
		curl_easy_setopt(pCurlHandle, CURLOPT_UPLOAD, TRUE );

		// Callback ������� �� ������ �� ���������� ����� � ��������� � sftp
		curl_easy_setopt(pCurlHandle, CURLOPT_READFUNCTION, ReadCallback );

		// ������ ����� �� ���������
		curl_easy_setopt(pCurlHandle, CURLOPT_READDATA, pFile);

		eCurlResult = curl_easy_perform( pCurlHandle );

		if( eCurlResult == CURLE_FAILED_INIT )
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);

			eCurlResult = curl_easy_perform( pCurlHandle );
		}// if

	}// SYS_TRY
	SYS_CATCH(__FUNCTION__)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

		if( pFile != NULL )
			fclose( pFile );

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}// SYS_CATCH

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("curl_easy_perform"), eCurlResult ) ) );

		if( pFile != NULL )
			fclose( pFile );

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}// if

	if( pFile != NULL )
		fclose( pFile );

	curl_easy_cleanup( pCurlHandle );

	return TRUE;
}

BOOL CCurlWrapper::CreateDirectory( const IN CString& strDirectory )
{
	curl_slist *pCurlCommandsList = NULL;
	// ����������� ������� �� ��������� �� ����������
	pCurlCommandsList = curl_slist_append( pCurlCommandsList, _T("mkdir ") + strDirectory );

	CURLcode eCurlResult = CURLE_OK;
	CURL* pCurlHandle = curl_easy_init();
	if( pCurlHandle == NULL )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return FALSE;
	}// if

	SYS_TRY
	{
		// �������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, m_strHostName );

		// Credentials �� �������
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// ������ � �������
		curl_easy_setopt( pCurlHandle, CURLOPT_QUOTE, pCurlCommandsList );

		eCurlResult = curl_easy_perform( pCurlHandle );

		if( eCurlResult == CURLE_FAILED_INIT )
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);

			eCurlResult = curl_easy_perform( pCurlHandle );
		}// if

	}// SYS_TRY
	SYS_CATCH(__FUNCTION__)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}// SYS_CATCH

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("curl_easy_perform"), eCurlResult ) ));

		curl_easy_cleanup(pCurlHandle);

		return FALSE;
	}// if

	curl_easy_cleanup(pCurlHandle);

	return TRUE;
}

BOOL CCurlWrapper::RenameRequest( const IN CString& strFilePathFrom, const IN CString& strFilePathTo )
{
	CURLcode eCurlResult = CURLE_OK;

	eCurlResult = CheckDirectoryExists( strFilePathFrom, false );

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__ ),
			&MSGKEY(_T("1"), curl_easy_strerror(eCurlResult)));

		return FALSE;
	}// if

	struct curl_slist* pCurlCommandList = NULL;
	// ����������� ������� �� ������������ �� ����
	pCurlCommandList = curl_slist_append( pCurlCommandList,  CSMsg::ComposeString( _T("rename {FROM} {TO}"), &MSGKEY( _T("FROM"), strFilePathFrom )
		, &MSGKEY( _T("TO"), strFilePathTo ) ) );

	CURL* pCurlHandle = curl_easy_init();
	if( pCurlHandle == NULL )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return FALSE;
	}// if

	SYS_TRY
	{
		// �������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, m_strHostName );

		// Credentials �� �������
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// ������ �� �������
		curl_easy_setopt( pCurlHandle, CURLOPT_QUOTE, pCurlCommandList );

		// �������, �� �� ������ download request, ��� �� ������� ���� �.�. ���� �� ����� ���� ���� ����������
		curl_easy_setopt(pCurlHandle, CURLOPT_NOBODY, TRUE);

		eCurlResult = curl_easy_perform( pCurlHandle );

		if( eCurlResult == CURLE_FAILED_INIT )
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);

			eCurlResult = curl_easy_perform( pCurlHandle );
		}// if

	}// SYS_TRY
	SYS_CATCH(__FUNCTION__)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}// SYS_CATCH

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError( _T("curl_easy_perform"), eCurlResult ) ) );

		curl_easy_cleanup(pCurlHandle);

		return FALSE;

	}// if

	curl_easy_cleanup(pCurlHandle);

	return TRUE;
}

size_t CCurlWrapper::WriteDirectoryListCallback( void* pFileContent, size_t nSize, size_t nMemoryBlockSize, void* pOutputBuffer )
{
	size_t nBufferActualSize = nSize * nMemoryBlockSize;
	CURL_TRANSFER_BUFFER recTransferBuffer;
	
	if( pFileContent == NULL )
		return nBufferActualSize;

	recTransferBuffer.pData = (BYTE*)malloc( nBufferActualSize );
	memcpy( recTransferBuffer.pData, pFileContent, nBufferActualSize );

	CStringA strDecrA = ( char* )recTransferBuffer.pData;

	CStringArray* pFileNamesArray = (CStringArray *)pOutputBuffer;
	if( pFileNamesArray == NULL )
	{
		free(recTransferBuffer.pData);
		return nBufferActualSize;
	}

	CString strNameToAdd;

	if( strDecrA.GetLength() > nBufferActualSize )
		strNameToAdd = strDecrA.Mid( 0, (int)nBufferActualSize );
	else
		strNameToAdd = strDecrA.Mid( 0, strDecrA.GetLength() );

	pFileNamesArray->Add( strNameToAdd );

	free(recTransferBuffer.pData);

	return nBufferActualSize;
}

BOOL CCurlWrapper::GetUnprocessedFiles( const IN CString& strDirectory, OUT CStringArray& oFileNamesArray )
{
	CURLcode eCurlResult = CURLE_OK;

	// ����������� ���� ������������ ����������
	if (CheckDirectoryExists(strDirectory, false) != CURLE_OK)
		return FALSE;

	CURL* pCurlHandle = curl_easy_init();
	if( pCurlHandle == NULL )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return FALSE;
	}// if

	CURL_TRANSFER_BUFFER recCurlOutputBuffer;
	recCurlOutputBuffer.nSize = 0;
	recCurlOutputBuffer.pData = (BYTE*)malloc( recCurlOutputBuffer.nSize );

	CString strDirectoryToSearch;
	strDirectoryToSearch.Append( m_strHostName );
	strDirectoryToSearch.Append( strDirectory );

	strDirectoryToSearch.Trim();

	SYS_TRY
	{
		// �������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, strDirectoryToSearch );

		// Credentials �� �������
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// ���� �� ��������� �����������
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEDATA, &oFileNamesArray );

		curl_easy_setopt( pCurlHandle, CURLOPT_FOLLOWLOCATION, 1L);

		// Callback �� ��������� � �������� �����
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEFUNCTION, WriteDirectoryListCallback );

		// ��������, �� �� �������� ������������
		curl_easy_setopt( pCurlHandle, CURLOPT_DIRLISTONLY, 1 );

		eCurlResult = curl_easy_perform( pCurlHandle );

		// ��� ����� ������ failed init, ���� �� ��� ������� ����������
		if( eCurlResult == CURLE_FAILED_INIT )
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);

			eCurlResult = curl_easy_perform( pCurlHandle );
		}// if

	}// SYS_TRY
	SYS_CATCH(__FUNCTION__)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

		curl_easy_cleanup( pCurlHandle );

		return FALSE;
	}// SYS_CATCH

	if( eCurlResult != CURLE_OK )
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2,
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), GetFunctionError(_T("curl_easy_perform"), eCurlResult)));

		curl_easy_cleanup(pCurlHandle);

		return FALSE;
	}// if

	curl_easy_cleanup(pCurlHandle);

	return TRUE;
}

CURLcode CCurlWrapper::CheckDirectoryExists(const IN CString& strFilePath, const bool bCreateIfNotExists)
{
	CStringArray oStringArray;
	CString strToken;
	int nCurrentPosition = 0;
	CURLcode eCurlResultCode = CURLE_OK;

	// ��������� ������������ �� ������� ������, �� curl �� ������� �� ���������� � ����������..
	strToken = strFilePath.Tokenize(_T("/\\"), nCurrentPosition);
	while (!strToken.IsEmpty())
	{
		// ��� ������� ������ .xml, ����� ��� �� �������� �� fileRename, ������ �� ������ � ��� �� ����, � ��� ������ ���� ����������
		if (strFilePath.GetLength() >= nCurrentPosition)
			oStringArray.Add(strToken);

		strToken = strFilePath.Tokenize(_T("/\\"), nCurrentPosition);
	};

	CString strCurrentDirectory = m_strHostName;
	CString strDirectoryForMaking;

	CURL* pCurlHandle = curl_easy_init();
	if (pCurlHandle == NULL)
	{
		CSMsg::Log(eLog, MSG_ERROR_CALLING_2,
			&MSGKEY(_T("0"), __FUNCTION__),
			&MSGKEY(_T("1"), _T("pCurlHandle == NULL")));

		return CURLE_INTERFACE_FAILED;
	}

	for (int i = 0; i < oStringArray.GetCount(); i++)
	{
		const CString strDirectory = oStringArray.GetAt(i);

		// �������� ����� ���������� ��� ������������� ������ � ����������� ���������� �� ����������
		// ��������: ����� ���������� = XML/IN/BISERA
		// �� ����������� ����� host/XML, ����� host/XML/IN.. � ���� �������

		if (i > 0)
		{
			strCurrentDirectory.Append(SFTP_PATH_DELIMITER);
			strDirectoryForMaking.Append(SFTP_PATH_DELIMITER);
		}//if

		strCurrentDirectory.Append(strDirectory);
		strDirectoryForMaking.Append(strDirectory);

		strCurrentDirectory.Trim();
		strDirectoryForMaking.Trim();

		SYS_TRY
		{
			// ���������� �� ��������
			curl_easy_setopt(pCurlHandle, CURLOPT_URL, strCurrentDirectory);

			// Credentials �� �������
			curl_easy_setopt(pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials);

			// �������, �� �� ������ download request, ��� �� ������� ���� �.�. ���� �� ����� ���� ���� ����������
			curl_easy_setopt(pCurlHandle, CURLOPT_NOBODY, TRUE);

			eCurlResultCode = curl_easy_perform(pCurlHandle);

			if (eCurlResultCode == CURLE_FAILED_INIT)
			{
				curl_global_init(CURL_GLOBAL_DEFAULT);

				eCurlResultCode = curl_easy_perform(pCurlHandle);
			}// if

		}// SYS_TRY

		SYS_CATCH(__FUNCTION__)
		{
			CSMsg::Log(eLog, MSG_ERROR_CALLING_2,
							 &MSGKEY(_T("0"), __FUNCTION__),
							 &MSGKEY(_T("1"), _T("CURLE_FAILED_INIT")));

			curl_easy_cleanup(pCurlHandle);

			return CURLE_FAILED_INIT;
		}// SYS_CATCH

		// ��� �� ������ ���������� � � ��������, �� �� � ���������, �� �������� �� � ��������
		if (eCurlResultCode == CURLE_REMOTE_FILE_NOT_FOUND)
		{
			if (bCreateIfNotExists)
			{
				if (!CreateDirectory(strDirectoryForMaking))
				{
					curl_easy_cleanup(pCurlHandle);

					CSMsg::Log(eLog, MSG_ERROR_CALLING_2,
									 &MSGKEY(_T("0"), __FUNCTION__),
									 &MSGKEY(_T("1"), _T("CreateDirectory")));

					return CURLE_WRITE_ERROR;
				}// if

			}//if

			else
			{
				curl_easy_cleanup(pCurlHandle);

				CSMsg::Log(eLog, MSG_CURL_FILE_PATH_NOT_EXISTS,
								 &MSGKEY(_T("ERROR"), GetFunctionError(_T("curl_easy_perform"), eCurlResultCode)));

				return eCurlResultCode;
			}//else

		}//if

		// �������� �� ����� ���� CURLE_REMOTE_ACCESS_DENIED � CURLE_OK - ��� ��� ����� ������, ����� ���� �� � ��������
		else if (eCurlResultCode != CURLE_REMOTE_ACCESS_DENIED && eCurlResultCode != CURLE_OK)
		{
			curl_easy_cleanup(pCurlHandle);

			CSMsg::Log(eLog, MSG_ERROR_CALLING_2, 
							 &MSGKEY(_T("0"), __FUNCTION__),
							 &MSGKEY(_T("1"), GetFunctionError(_T("curl_easy_perform"), eCurlResultCode)));

			return eCurlResultCode;
		}//else

	}//for

	curl_easy_cleanup(pCurlHandle);

	// ��� ��� �������� �����, ����� ������ � ��
	eCurlResultCode = CURLE_OK;

	return eCurlResultCode;
}

// Overrides
// ----------------

#endif