#include "assert.h"

#include <sqlite3/sqlite3.h>
#include <sqlite_tools.h>

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>

struct large_db
{
	struct Data
	{
		Data();
		Data(int id, const std::string& name, double value)
			: id (id)
			, name(name)
			, value(value)
		{}

		int id;
		std::string name;
		double value;

		SQLT_TABLE(Data,
			SQLT_COLUMN_PRIMARY_KEY(id),
			SQLT_COLUMN(name),
			SQLT_COLUMN(value)
		);
	};

	SQLT_DATABASE_WITH_NAME(large_db, "large_db.sqlite",
		SQLT_DATABASE_TABLE(Data)
	);
};

std::string rand_str()
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	std::string str;
	str.resize(10);
	for (int i = 0; i < 10; ++i)
		str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	return str;
}

double rand_flt()
{
	return float(rand()) / float(RAND_MAX) * 54312;
}

int main()
{
	static const int COUNT = 1e6;
	srand(34512);

	// Generate random data set.
	std::vector<large_db::Data> data;
	data.reserve(COUNT);
	for (size_t i = 0; i < COUNT; i++)
		data.emplace_back(rand(), rand_str(), rand_flt());

	char *errMsg;
	int result = SQLT::dropAllTables<large_db>(&errMsg);
	SQLT_ASSERT(result == SQLITE_OK);

	auto start = std::chrono::system_clock::now();

	sqlite3 *db;
	result = SQLT::open<large_db>(&db);
	if (result == SQLITE_OK)
	{
		// When performing more than a few SQL queries we should wrap the calls in a transaction.
		result = SQLT::begin<large_db>(db);                    assert(result == SQLITE_OK);
		result = SQLT::createAllTables<large_db>(db, &errMsg); assert(result == SQLITE_OK);
		result = SQLT::insert(db, data);                       assert(result == SQLITE_OK);
		result = SQLT::commit<large_db>(db);                   assert(result == SQLITE_OK);
	}

	SQLT::close<large_db>(db);

	auto end = std::chrono::system_clock::now();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	fprintf(stderr, "Inserted %d elements in %lld milliseconds.\n", COUNT, milliseconds.count());

	return 0;
}
