#pragma once
#include <format>
#include <string>
#include <vector>
#include <cstdint>
#include <exception>
#include <functional>
#include <type_traits>

#include <sqlite3.h>

enum CQLITE_FLAGS
{
	CQLITE_FLAG_URI           = 0x1,
	CQLITE_FLAG_READ          = 0x2,
	CQLITE_FLAG_WRITE         = 0x4,
	CQLITE_FLAG_MUTEX         = 0x8,
	CQLITE_FLAG_CREATE        = 0x10,
	CQLITE_FLAG_MEMORY        = 0x20,
	CQLITE_FLAG_NO_FOLLOW     = 0x40,
	CQLITE_FLAG_SHARED_CACHE  = 0x80,
	CQLITE_FLAG_PRIVATE_CACHE = 0x100
};

class CQLite;

class CQLiteLock
{
	sqlite3_mutex* mutex;

	CQLiteLock(const CQLiteLock&) = delete;

public:
	CQLiteLock()
		: mutex(nullptr)
	{
	}
	CQLiteLock(CQLiteLock&& lock)
		: mutex(lock.mutex)
	{
		lock.mutex = nullptr;
	}

	explicit CQLiteLock(CQLite& cqlite);

	virtual ~CQLiteLock();

	auto& operator = (CQLiteLock&& lock)
	{
		if (mutex)
			sqlite3_mutex_leave(mutex);

		mutex = lock.mutex;
		lock.mutex = nullptr;

		return *this;
	}
};

class CQLiteQuery
{
	friend CQLite;

	sqlite3_stmt* stmt;

	CQLiteQuery(const CQLiteQuery&) = delete;

public:
	CQLiteQuery()
		: stmt(nullptr)
	{
	}
	CQLiteQuery(CQLiteQuery&& query)
		: stmt(query.stmt)
	{
		query.stmt = nullptr;
	}

	virtual ~CQLiteQuery()
	{
		Release();
	}

	constexpr auto GetHandle() const
	{
		return stmt;
	}

	void Release()
	{
		if (stmt)
		{
			sqlite3_finalize(stmt);

			stmt = nullptr;
		}
	}

	auto& operator = (CQLiteQuery&& query)
	{
		if (stmt)
			sqlite3_finalize(stmt);

		stmt = query.stmt;
		query.stmt = nullptr;

		return *this;
	}

	bool operator == (const CQLiteQuery& query) const
	{
		return stmt == query.stmt;
	}
	bool operator != (const CQLiteQuery& query) const
	{
		return !operator==(query);
	}
};

class CQLiteQueryResult
{
	friend CQLite;

	sqlite3_stmt* stmt;
	size_t        column_count;
	int64_t       last_insert_row_id;
	int64_t       number_of_rows_modified;

	CQLiteQueryResult(CQLiteQueryResult&&) = delete;
	CQLiteQueryResult(const CQLiteQueryResult&) = delete;

public:
	CQLiteQueryResult()
		: stmt(nullptr),
		column_count(0),
		last_insert_row_id(0),
		number_of_rows_modified(0)
	{
	}

	constexpr auto GetCount() const
	{
		return column_count;
	}

	constexpr auto GetLastInsertRowId() const
	{
		return last_insert_row_id;
	}

	constexpr auto GetNumberOfRowsModified() const
	{
		return number_of_rows_modified;
	}

	bool IsBlob(size_t index) const
	{
		if (index >= column_count)
			return false;

		return sqlite3_column_type(stmt, (int)index) == SQLITE_BLOB;
	}
	bool IsNull(size_t index) const
	{
		if (index >= column_count)
			return false;

		return sqlite3_column_type(stmt, (int)index) == SQLITE_NULL;
	}
	bool IsText(size_t index) const
	{
		if (index >= column_count)
			return false;

		return sqlite3_column_type(stmt, (int)index) == SQLITE_TEXT;
	}
	bool IsFloat(size_t index) const
	{
		if (index >= column_count)
			return false;

		return sqlite3_column_type(stmt, (int)index) == SQLITE_FLOAT;
	}
	bool IsInteger(size_t index) const
	{
		if (index >= column_count)
			return false;

		return sqlite3_column_type(stmt, (int)index) == SQLITE_INTEGER;
	}

	bool GetKey(size_t index, std::string& value) const
	{
		if (index >= column_count)
			return false;

		value = sqlite3_column_name(stmt, (int)index);

		return true;
	}
	bool GetKey(size_t index, std::wstring& value) const
	{
		if (index >= column_count)
			return false;

		value = (const wchar_t*)sqlite3_column_name16(stmt, (int)index);

		return true;
	}
	bool GetKey(size_t index, std::string_view& value) const
	{
		if (index >= column_count)
			return false;

		value = sqlite3_column_name(stmt, (int)index);

		return true;
	}
	bool GetKey(size_t index, std::wstring_view& value) const
	{
		if (index >= column_count)
			return false;

		value = (const wchar_t*)sqlite3_column_name16(stmt, (int)index);

		return true;
	}

	template<typename T>
	requires(std::is_floating_point_v<T>)
	bool GetValue(size_t index, T& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_FLOAT)
			return false;

		value = (T)sqlite3_column_double(stmt, (int)index);

		return true;
	}
	template<typename T>
	requires(std::is_enum_v<T> || std::is_integral_v<T>)
	bool GetValue(size_t index, T& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_INTEGER)
			return false;

		if constexpr (sizeof(T) <= sizeof(int))
			value = (T)sqlite3_column_int(stmt, (int)index);
		else
			value = (T)sqlite3_column_int64(stmt, (int)index);

		return true;
	}
	bool GetValue(size_t index, bool& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_INTEGER)
			return false;

		value = sqlite3_column_int64(stmt, (int)index) != 0;

		return true;
	}
	bool GetValue(size_t index, std::string& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_TEXT)
			return false;

		value = (const char*)sqlite3_column_text(stmt, (int)index);

		return true;
	}
	bool GetValue(size_t index, std::wstring& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_TEXT)
			return false;

		value = (const wchar_t*)sqlite3_column_text16(stmt, (int)index);

		return true;
	}
	bool GetValue(size_t index, std::string_view& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_TEXT)
			return false;

		value = (const char*)sqlite3_column_text(stmt, (int)index);

		return true;
	}
	bool GetValue(size_t index, std::wstring_view& value) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_TEXT)
			return false;

		value = (const wchar_t*)sqlite3_column_text16(stmt, (int)index);

		return true;
	}
	bool GetValue(size_t index, void* buffer, size_t size) const
	{
		if (index >= column_count)
			return false;

		if (sqlite3_column_type(stmt, (int)index) != SQLITE_TEXT)
			return false;

		auto blob_size   = (size_t)sqlite3_column_bytes(stmt, (int)index);
		auto blob_buffer = sqlite3_column_blob(stmt, (int)index);

		if (blob_size > size)
			blob_size = size;

		memcpy(buffer, blob_buffer, blob_size);

		return true;
	}

	auto GetValueSize(size_t index) const
	{
		return (size_t)((index < column_count) ? sqlite3_column_bytes(stmt, (int)index) : 0);
	}
};

class CQLiteException
	: public std::exception
{
	std::string message;

	int         error_code;
	std::string error_string;
	std::string error_function;

public:
	CQLiteException()
		: error_code(0)
	{
	}
	CQLiteException(CQLiteException&& exception)
		: message(std::move(exception.message)),
		error_code(exception.error_code),
		error_string(std::move(exception.error_string)),
		error_function(std::move(exception.error_function))
	{
		exception.error_code = 0;
	}
	CQLiteException(const CQLiteException& exception)
		: message(exception.message),
		error_code(exception.error_code),
		error_string(exception.error_string),
		error_function(exception.error_function)
	{
	}
	CQLiteException(std::string_view function, int code, std::string_view string)
		: message(std::format("{} returned {}: {}", function, code, string)),
		error_code(code),
		error_string(string),
		error_function(function)
	{
	}

	virtual const char* what() const noexcept
	{
		return message.c_str();
	}

	auto& operator = (CQLiteException&& exception)
	{
		message        = std::move(exception.message);
		error_code     = exception.error_code;
		error_string   = std::move(exception.error_string);
		error_function = std::move(exception.error_function);

		exception.error_code = 0;

		return *this;
	}
	auto& operator = (const CQLiteException& exception)
	{
		message        = exception.message;
		error_code     = exception.error_code;
		error_string   = exception.error_string;
		error_function = exception.error_function;

		return *this;
	}
};

typedef std::function<bool(const CQLiteQueryResult& result)> CQLiteExecuteCallback;

class CQLite
{
	sqlite3*    db;
	std::string path;
	int         flags;

	CQLite(const CQLite&) = delete;

public:
	CQLite()
		: CQLite(":memory:", CQLITE_FLAG_CREATE | CQLITE_FLAG_MEMORY | CQLITE_FLAG_READ | CQLITE_FLAG_WRITE)
	{
	}
	CQLite(CQLite&& cqlite)
		: db(cqlite.db),
		path(std::move(cqlite.path)),
		flags(cqlite.flags)
	{
		cqlite.db = nullptr;
	}
	CQLite(std::string_view path, int flags)
		: db(nullptr),
		path(path),
		flags(flags)
	{
	}

	virtual ~CQLite()
	{
		if (IsOpen())
			Close();
	}

	constexpr bool  IsOpen() const
	{
		return db != nullptr;
	}

	constexpr auto& GetPath() const
	{
		return path;
	}

	constexpr auto  GetFlags() const
	{
		return flags;
	}

	constexpr auto  GetHandle() const
	{
		return db;
	}

	// @throw CQLiteException
	void Open()
	{
		if (IsOpen())
			throw CQLiteException("IsOpen", true, "CQLite already open");

		int flags = 0;

		if (this->flags & CQLITE_FLAG_URI)           flags |= SQLITE_OPEN_URI;
		if (this->flags & CQLITE_FLAG_READ)          flags |= SQLITE_OPEN_READWRITE;
		if (!(this->flags & CQLITE_FLAG_WRITE))      flags  = (flags & ~SQLITE_OPEN_READWRITE) | SQLITE_OPEN_READONLY;
		if (this->flags & CQLITE_FLAG_MUTEX)         flags |= SQLITE_OPEN_FULLMUTEX;
		else                                         flags |= SQLITE_OPEN_NOMUTEX;
		if (this->flags & CQLITE_FLAG_CREATE)        flags |= SQLITE_OPEN_CREATE;
		if (this->flags & CQLITE_FLAG_MEMORY)        flags |= SQLITE_OPEN_MEMORY;
		if (this->flags & CQLITE_FLAG_NO_FOLLOW)     flags |= SQLITE_OPEN_NOFOLLOW;
		if (this->flags & CQLITE_FLAG_SHARED_CACHE)  flags |= SQLITE_OPEN_SHAREDCACHE;
		if (this->flags & CQLITE_FLAG_PRIVATE_CACHE) flags |= SQLITE_OPEN_PRIVATECACHE;

		if (int error = sqlite3_open_v2(path.c_str(), &db, flags, nullptr))
			throw CQLiteException("sqlite3_open_v2", error, sqlite3_errstr(error));
	}
	void Close()
	{
		if (IsOpen())
		{
			sqlite3_close(db);

			db = nullptr;
		}
	}

	// @throw CQLiteException
	bool Execute(std::string_view query)
	{
		CQLiteExecuteCallback callback;

		return Execute(query, callback);
	}
	// @throw CQLiteException
	bool Execute(std::string_view query, const CQLiteExecuteCallback& callback)
	{
		return Execute(Prepare(query), callback);
	}
	// @throw CQLiteException
	bool Execute(const CQLiteQuery& query, const CQLiteExecuteCallback& callback)
	{
		if (!IsOpen())
			throw CQLiteException("IsOpen", false, "CQLite not open");

		int error;

		while ((error = sqlite3_step(query.GetHandle())) != SQLITE_DONE)
		{
			if (error != SQLITE_ROW)
			{
				sqlite3_reset(query.GetHandle());

				throw CQLiteException("sqlite3_step", error, sqlite3_errmsg(db));
			}

			CQLiteQueryResult result;
			result.stmt                    = query.GetHandle();
			result.column_count            = (size_t)sqlite3_column_count(query.GetHandle());
			result.last_insert_row_id      = sqlite3_last_insert_rowid(db);
			result.number_of_rows_modified = sqlite3_changes64(db);

			if (!callback(result))
			{
				sqlite3_reset(query.GetHandle());

				return false;
			}
		}

		sqlite3_reset(query.GetHandle());

		return true;
	}

	// @throw CQLiteException
	template<typename ... T>
	CQLiteQuery Prepare(std::string_view statement, const T& ... values) const
	{
		if (!IsOpen())
			throw CQLiteException("IsOpen", false, "CQLite not open");

		sqlite3_stmt* stmt;

		if (int error = sqlite3_prepare_v2(db, statement.data(), -1, &stmt, nullptr))
			throw CQLiteException("sqlite3_prepare_v2", error, sqlite3_errmsg(db));

		CQLiteQuery query;

		if (stmt)
		{
			query.stmt = stmt;

			int i = 0;

			try
			{
				(Bind(stmt, ++i, values), ...);
			}
			catch (...)
			{
				sqlite3_finalize(stmt);

				throw;
			}
		}

		return query;
	}

	auto& operator = (CQLite&& cqlite)
	{
		Close();

		db    = cqlite.db;
		path  = std::move(cqlite.path);
		flags = cqlite.flags;

		cqlite.db = nullptr;

		return *this;
	}

private:
	// @throw CQLiteException
	template<typename T>
	requires(std::is_floating_point_v<T>)
	static void Bind(sqlite3_stmt* stmt, int i, T value)
	{
		sqlite3_bind_double(stmt, i, (double)value);
	}
	// @throw CQLiteException
	template<typename T>
	requires(std::is_enum_v<T> || std::is_integral_v<T>)
	static void Bind(sqlite3_stmt* stmt, int i, T value)
	{
		if constexpr (sizeof(T) <= sizeof(int))
			sqlite3_bind_int(stmt, i, (int)value);
		else
			sqlite3_bind_int64(stmt, i, (sqlite3_int64)value);
	}
	template<typename T>
	static void Bind(sqlite3_stmt* stmt, int i, T* value)
	{
		if (value == nullptr)
			sqlite3_bind_null(stmt, i);
		else
			sqlite3_bind_int64(stmt, i, (sqlite3_int64)value);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, bool value)
	{
		sqlite3_bind_int(stmt, i, value ? 1 : 0);
	}
	// @throw CQLiteException
	template<typename T>
	static void Bind(sqlite3_stmt* stmt, int i, const T* value)
	{
		if (value == nullptr)
			sqlite3_bind_null(stmt, i);
		else
			sqlite3_bind_int64(stmt, i, (sqlite3_int64)value);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, const char* value)
	{
		sqlite3_bind_text(stmt, i, value, -1, SQLITE_STATIC);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, const wchar_t* value)
	{
		sqlite3_bind_text16(stmt, i, value, -1, SQLITE_STATIC);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, std::string_view value)
	{
		sqlite3_bind_text(stmt, i, value.data(), (int)value.length(), SQLITE_STATIC);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, std::wstring_view value)
	{
		sqlite3_bind_text16(stmt, i, value.data(), (int)value.length(), SQLITE_STATIC);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, const std::string& value)
	{
		sqlite3_bind_text(stmt, i, value.c_str(), (int)value.length(), SQLITE_STATIC);
	}
	// @throw CQLiteException
	static void Bind(sqlite3_stmt* stmt, int i, const std::wstring& value)
	{
		sqlite3_bind_text16(stmt, i, value.c_str(), (int)value.length(), SQLITE_STATIC);
	}
};

inline CQLiteLock::CQLiteLock(CQLite& cqlite)
	: mutex(nullptr)
{
	if (auto db = cqlite.GetHandle())
		if (mutex = sqlite3_db_mutex(db))
			sqlite3_mutex_enter(mutex);
}
inline CQLiteLock::~CQLiteLock()
{
	if (mutex)
		sqlite3_mutex_leave(mutex);
}
