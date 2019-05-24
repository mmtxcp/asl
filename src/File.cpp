#undef __STRICT_ANSI__
#include <asl/File.h>
//#include <asl/Directory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#if !defined(ASL_ANSI)
#define UNICODE
#undef WIN32_FIND_DATA
#undef FindFirstFile
#define WIN32_FIND_DATA WIN32_FIND_DATAW
#define FindFirstFile FindFirstFileW
#endif
#define fdopen _fdopen
#define dup _dup

namespace asl {

const char File::SEP='\\';
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>
namespace asl {
const char File::SEP='/';
#endif

#if defined(_WIN32) && !defined(ASL_ANSI)
#define STR_PREFIX(x) L##x
#define fopenX _wfopen
#define CHART wchar_t
#else
#define STR_PREFIX(x) x
#define fopenX fopen
#define CHART char
#endif

#ifdef _WIN32

inline double ft2t(const FILETIME& ft)
{
	LARGE_INTEGER t;
	t.HighPart = ft.dwHighDateTime;
	t.LowPart = ft.dwLowDateTime;
	return t.QuadPart*100e-9 - 11644473600.0;
}

#define nat(x) x

static FileInfo infoFor(const WIN32_FIND_DATA& data)
{
	FileInfo info;
	info.lastModified = ft2t(data.ftLastWriteTime);
	info.creationDate = ft2t(data.ftCreationTime);
	info.size = data.nFileSizeLow + ((Long)data.nFileSizeHigh << 32);
	info.flags = data.dwFileAttributes;
	return info;
}

static FileInfo getFileInfo(const String& path)
{
	WIN32_FIND_DATA data;
	HANDLE hdir = FindFirstFile(nat(path), &data);
	if (hdir == INVALID_HANDLE_VALUE)
	{
		FileInfo info;
		memset(&info, 0, sizeof(FileInfo));
		return info;
	}
	FindClose(hdir);
	return infoFor(data);
}

#else

inline double ft2t(const time_t& ft)
{
	return ft;
}

static FileInfo infoFor(const struct stat& data)
{
	FileInfo info;
	info.lastModified = ft2t(data.st_mtime);
	info.creationDate = ft2t(data.st_ctime);
	info.size = data.st_size;
	info.flags = data.st_mode;
	return info;
}

FileInfo getFileInfo(const String& path)
{
	struct stat data;
	if (stat(path, &data))
	{
		FileInfo info;
		memset(&info, 0, sizeof(FileInfo));
		return info;
	}
	return infoFor(data);
}

#endif

bool File::open(const String& name, File::OpenMode mode)
{
	if(name == "")
		return false;
	const CHART* fopen_mode;
	if(!(mode & TEXT))
	{
		fopen_mode = (mode==READ)? STR_PREFIX("rb"):
			(mode==WRITE)? STR_PREFIX("wb"):
			(mode==APPEND)? STR_PREFIX("ab"):
			STR_PREFIX("rb+");
	}
	else
	{
		mode = (OpenMode) (mode & ~TEXT);
		fopen_mode = (mode==READ)? STR_PREFIX("rt"):
			(mode==WRITE)? STR_PREFIX("wt"):
			(mode==APPEND)? STR_PREFIX("at"):
			STR_PREFIX("rt+");
	}
	_file = fopenX(name, fopen_mode);

	_path = name;

	return _file != 0;
}

bool File::openfd(int fd)
{
	int fd2 = dup(fd);
	_file = fdopen(fd2, "wb");
	return _file != 0;
}

void File::close()
{
	if(_file)
		fclose(_file);
	_file = 0;
	_info = FileInfo();
}

void File::setBuffering(char mode, int size)
{
	int m=(mode=='n')?_IONBF:(mode=='l')?_IOLBF:_IOFBF;
	setvbuf(_file, 0, m, size);
}

Long File::position()
{
#if (defined( _MSC_VER ) && _MSC_VER > 1310)
	return _ftelli64(_file);
#else
	return ftell(_file);
#endif
}


void File::seek(Long offset, SeekMode from)
{
#if (defined( _MSC_VER ) && _MSC_VER > 1310)
	_fseeki64(_file, offset, (from==START)? SEEK_SET :(from==HERE)? SEEK_CUR: SEEK_END);
#else
	fseek(_file, offset, (from==START)? SEEK_SET :(from==HERE)? SEEK_CUR: SEEK_END);
#endif
}

bool File::isDirectory() const
{
	if (_path.endsWith('/')
#ifdef _WIN32
		|| _path.endsWith('\\')
#endif		
		)
		return File(_path.substring(0, _path.length() - 1)).isDirectory();

	if(!_info)
		_info = getFileInfo(_path);
#ifdef _WIN32
	return (_info.flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	return (S_ISDIR(_info.flags)) != 0;
#endif
}


String File::name() const
{
	int n = _path.lastIndexOf(SEP);
#ifdef _WIN32
	int m = _path.lastIndexOf('/');
	n = (m > n)? m : n;
#endif
	return _path.substring(n+1);
}

String File::extension() const
{
	int n = _path.lastIndexOf(SEP);
#ifdef _WIN32
	int m = _path.lastIndexOf('/');
	n = (m > n) ? m : n;
#endif
	int dot = _path.lastIndexOf('.');
	return (dot >= 0 && dot > n) ? _path.substring(dot + 1) : String();
}

bool File::hasExtension(const String& extensions) const
{
	Array<String> exts = extensions.toLowerCase().split('|');
	String ext = extension().toLowerCase();
	for (int i = 0; i < exts.length(); i++)
		if (ext == exts[i])
			return true;
	return false;
}

String File::directory() const
{
	int n = _path.lastIndexOf(SEP);
#ifdef _WIN32
	int m = _path.lastIndexOf('/');
	n = (m > n)? m : n;
#endif
	return (n>=0)? _path.substring(0, n) : String(".");
}

Long File::size() const
{
	if(!_info)
		_info = getFileInfo(_path);
	return _info.size;
}

Date File::creationDate() const
{
	if(!_info)
		_info = getFileInfo(_path);
	return _info.creationDate;
}

Date File::lastModified() const
{
	if(!_info)
		_info = getFileInfo(_path);
	return _info.lastModified;
}

bool File::setLastModified(const Date& t)
{
#ifdef _WIN32
	HANDLE hfile = CreateFileW(_path, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (!hfile)
		return false;
	LARGE_INTEGER ti;
	ti.QuadPart = LONGLONG((t.time() + 11644473600.0) * 1e7);
	FILETIME ft;
	ft.dwHighDateTime = ti.HighPart;
	ft.dwLowDateTime = ti.LowPart;
	int ok = SetFileTime(hfile, 0, 0, &ft);
	CloseHandle(hfile);
	return ok != 0;
#else
	utimbuf times;
	struct stat data;
	if (stat(_path, &data) != 0)
		return false;
	times.actime = data.st_mtime;
	times.modtime = (time_t)t.time();
	return utime(_path, &times) == 0;
#endif
}

Array<byte> File::content()
{
	return firstBytes((int)size());
}

bool File::put(const Array<byte>& data)
{
	if(!_file && !open(_path, WRITE))
		return false;
	return write(data.ptr(), data.length()) == data.length();
}

Array<byte> File::firstBytes(int n)
{
	Array<byte> data(n);
	if (!_file && !open(_path)) {
		data.clear();
		return data;
	}
	data.resize(read(&data[0], n));
	return data;
}

int File::read(void* p, int n)
{
	return (int)fread(p, 1, n, _file);
}

int File::write(const void* p, int n)
{
	return (int)fwrite(p, 1, n, _file);
}

File File::temp(const String& ext)
{
	File file;
	unsigned num = (unsigned)(2e9 * fract(0.01 * now()));
	unsigned p = (unsigned int)(size_t)&file;
#ifdef _WIN32
	TCHAR tmpdir[MAX_PATH];
	DWORD ret = GetTempPath(MAX_PATH, tmpdir);
	String tmpDir = tmpdir;
	if (ret > MAX_PATH || ret == 0)
		return file;
	unsigned pid = (unsigned int)GetCurrentProcessId();
#else
	String tmpDir = "/tmp";
	unsigned pid = (unsigned)getpid();
#endif
	do {
		file._path = String(0, "%s/%04x%08x%08x%s", *tmpDir, pid, p, num++, *ext);
	} while (file.exists());
	file.open(File::WRITE);
	return file;
#if 0
	char nam[500];
	strcpy(nam, P_tmpdir "/XXXXXX");
	#ifdef __ANDROID_API__
	int fd = mkstemp(nam);
	#else
	strcat(nam, ext);
	int fd = mkstemps(nam, ext.length());
	#endif
	File file;
	if(fd < 0)
		return file;
	::close(fd);
	file._path = nam;
	return file;
#endif
}

}
