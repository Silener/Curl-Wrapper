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

	// Взимаме си пътя, който ще ползваме, за да прочетем файла
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

		// Задаваме хост + пътя на файла
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, strURL );

		// Credentials за логване
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Callback функция за писане на прочетените данни в изходния буфер
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);

		// Изходен буфер
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

	// Проверяваме дали директориява съществува
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

	// Ще проверим дали файла съществува
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

		// Задаваме адрес
		curl_easy_setopt(pCurlHandle, CURLOPT_URL, strFullURL );

		// Credentials за логване
		curl_easy_setopt(pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Казваме, че ще upload-ваме
		curl_easy_setopt(pCurlHandle, CURLOPT_UPLOAD, TRUE );

		// Callback функция за четене на подадените данни и записване в sftp
		curl_easy_setopt(pCurlHandle, CURLOPT_READFUNCTION, ReadCallback );

		// Самите данни за записване
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
	// Построяване команда за създаване на директория
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
		// Задаваме адрес
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, m_strHostName );

		// Credentials за логване
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Списък с команди
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
	// Построяваме команда за преименуване на файл
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
		// Задаваме адрес
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, m_strHostName );

		// Credentials за логване
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Списък от команди
		curl_easy_setopt( pCurlHandle, CURLOPT_QUOTE, pCurlCommandList );

		// Казваме, че ще правим download request, без да вземаме файл т.е. само да видим дали пътя съществува
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

	// Проверяваме дали директориява съществува
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
		// Задаваме адрес
		curl_easy_setopt( pCurlHandle, CURLOPT_URL, strDirectoryToSearch );

		// Credentials за логване
		curl_easy_setopt( pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials );

		// Къде ще записваме прочетеното
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEDATA, &oFileNamesArray );

		curl_easy_setopt( pCurlHandle, CURLOPT_FOLLOWLOCATION, 1L);

		// Callback за записване в изхорния буфер
		curl_easy_setopt( pCurlHandle, CURLOPT_WRITEFUNCTION, WriteDirectoryListCallback );

		// Указваме, че ще листваме директорията
		curl_easy_setopt( pCurlHandle, CURLOPT_DIRLISTONLY, 1 );

		eCurlResult = curl_easy_perform( pCurlHandle );

		// Ако върне грешка failed init, може да сме затрили глобалните
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

	// Разделяме директориите на отделни токени, че curl не разбира от директория в директория..
	strToken = strFilePath.Tokenize(_T("/\\"), nCurrentPosition);
	while (!strToken.IsEmpty())
	{
		// Ако намерим формат .xml, значи сме от контекст на fileRename, където се подава и име на файл, а тук търсим само директории
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

		// Добавяме всяка директория към съществуващия стринг и проверяваме директория по директория
		// Например: пълна директория = XML/IN/BISERA
		// Ще проверяваме първо host/XML, после host/XML/IN.. и така нататък

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
			// Директория за проверка
			curl_easy_setopt(pCurlHandle, CURLOPT_URL, strCurrentDirectory);

			// Credentials за логване
			curl_easy_setopt(pCurlHandle, CURLOPT_USERPWD, m_strCurlFormatedCredentials);

			// Казваме, че ще правим download request, без да вземаме файл т.е. само да видим дали пътя съществува
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

		// Ако не намери директория и е зададено, че ще я създаваме, се опитваме да я създадем
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

		// Очакваме да върне само CURLE_REMOTE_ACCESS_DENIED и CURLE_OK - ако има друга грешка, значи нещо се е объркало
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

	// Ако сме стигнали дотук, значи всичко е ок
	eCurlResultCode = CURLE_OK;

	return eCurlResultCode;
}

// Overrides
// ----------------

#endif