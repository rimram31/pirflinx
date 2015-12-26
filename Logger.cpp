#include "Logger.h"
#include <iostream>     /* standard I/O functions                         */
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>

#ifndef WIN32
	#include <syslog.h>
	#include <errno.h>
#endif


#define MAX_LOG_LINE_BUFFER 100
#define MAX_LOG_LINE_LENGTH 2048

extern bool g_bRunAsDaemon;
extern bool g_bUseSyslog;

CLogger::_tLogLineStruct::_tLogLineStruct(const _eLogLevel nlevel, const std::string &nlogmessage)
{
	level=nlevel;
	logmessage=nlogmessage;
}

CLogger::CLogger(void)
{
	m_verbose_level=VBL_ALL;
}


CLogger::~CLogger(void)
{
	if (m_outputfile.is_open())
		m_outputfile.close();
}

void CLogger::SetOutputFile(const char *OutputFile)
{
	if (m_outputfile.is_open())
		m_outputfile.close();

	if (OutputFile==NULL)
		return;

	try {
#ifdef _DEBUG
		m_outputfile.open(OutputFile, std::ios::out | std::ios::trunc);
#else
		m_outputfile.open(OutputFile, std::ios::out | std::ios::app);
#endif
	} catch(...)
	{
		std::cerr << "Error opening output log file..." << std::endl;
	}
}

void CLogger::SetVerboseLevel(_eLogFileVerboseLevel vLevel)
{
	m_verbose_level=vLevel;
}

void CLogger::Log(const _eLogLevel level, const char* logline, ...)
{
	bool bDoLog = false;
	if (m_verbose_level == VBL_ALL)
		bDoLog = true;
	else if ((m_verbose_level == VBL_STATUS_ERROR) && ((level == LOG_STATUS) || (level == LOG_ERROR)))
		bDoLog = true;
	else if ((m_verbose_level == VBL_ERROR) && (level == LOG_ERROR))
		bDoLog = true;

	if (!bDoLog)
		return;

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	char szDate[100];
#if !defined WIN32
	// Get a timestamp
	struct timeval tv;
	gettimeofday(&tv, NULL);

	struct tm timeinfo;
	localtime_r(&tv.tv_sec, &timeinfo);

	// create a time stamp string for the log message
	snprintf(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
		timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
		timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, (int)tv.tv_usec / 1000);
#else
	// Get a timestamp
	SYSTEMTIME time;
	::GetLocalTime(&time);
	// create a time stamp string for the log message
	sprintf_s(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d.%03d ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#endif

	std::stringstream sstr;
	
	if ((level==LOG_NORM)||(level==LOG_STATUS))
	{
		sstr << szDate << " " << cbuffer;
	}
	else {
		sstr << szDate << " Error: " << cbuffer;
	}

	if (!g_bRunAsDaemon)
	{
		//output to console
		std::cout << sstr.str() << std::endl;
	}
    
	if (!m_outputfile.is_open())
		return;

	//output to file

	m_outputfile << sstr.str() << std::endl;
	m_outputfile.flush();
}

bool strhasEnding(std::string const &fullString, std::string const &ending)
{
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}

void CLogger::LogNoLF(const _eLogLevel level, const char* logline, ...)
{
	bool bDoLog = false;
	if (m_verbose_level == VBL_ALL)
		bDoLog = true;
	else if ((m_verbose_level == VBL_STATUS_ERROR) && ((level == LOG_STATUS) || (level == LOG_ERROR)))
		bDoLog = true;
	else if ((m_verbose_level == VBL_ERROR) && (level == LOG_ERROR))
		bDoLog = true;

	if (!bDoLog)
		return;

	va_list argList;
	char cbuffer[MAX_LOG_LINE_LENGTH];
	va_start(argList, logline);
	vsnprintf(cbuffer, sizeof(cbuffer), logline, argList);
	va_end(argList);

	std::string message=cbuffer;
	if (strhasEnding(message,"\n"))
	{
		message=message.substr(0,message.size()-1);
	}

	if (!g_bRunAsDaemon)
	{
		if ((level == LOG_NORM) || (level == LOG_STATUS))
		{
			std::cout << cbuffer;
			std::cout.flush();
		}
		else
		{
			std::cerr << cbuffer;
			std::cerr.flush();
		}
	}

	if (!m_outputfile.is_open())
		return;

	//output to file

	if ((level==LOG_NORM)||(level==LOG_STATUS))
		m_outputfile << cbuffer;
	else
		m_outputfile << "Error: " << cbuffer;
	m_outputfile.flush();
}
