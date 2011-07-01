/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "DB2Stores.h"
#include "Log.h"

#include "DB2format.h"

#include <map>

DB2Storage <ItemEntry> sItemStore(Itemfmt);
//DB2Storage <ItemSparseEntry> sItemSparseStore(ItemSparsefmt);
typedef std::list<std::string> StoreProblemList1;

uint32 DB2FileCount = 0;

static bool LoadDB2_assert_print(uint32 fsize,uint32 rsize, const std::string& filename)
{
    sLog->outError("Size of '%s' setted by format string (%u) not equal size of C++ structure (%u).", filename.c_str(), fsize, rsize);

    // ASSERT must fail after function call
    return false;
}

template<class T>
inline void LoadDB2(uint32& availableDb2Locales, StoreProblemList1& error, DB2Storage<T>& storage, const std::string& db2Path, const std::string& filename)
{
    // compatibility format and C++ structure sizes
    ASSERT(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()) == sizeof(T) || LoadDB2_assert_print(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()), sizeof(T), filename));

    ++DB2FileCount;
    std::string db2Filename = db2Path + filename;
    if (storage.Load(db2Filename.c_str()))
    {
        // sort problematic db2 to (1) non compatible and (2) nonexistent
        FILE * f = fopen(db2Filename.c_str(), "rb");
        if (f)
        {
            char buf[100];
            snprintf(buf, 100,"(exist, but have %d fields instead " SIZEFMTD ") Wrong client version DB2 file?", storage.GetFieldCount(), strlen(storage.GetFormat()));
            error.push_back(db2Filename + buf);
            fclose(f);
        }
        else
            error.push_back(db2Filename);
    }
}

void LoadDB2Stores(const std::string& dataPath)
{
    std::string db2Path = dataPath + "db2/";

    StoreProblemList1 bad_db2_files;
    uint32 availableDb2Locales = 0xFFFFFFFF;

    LoadDB2(availableDb2Locales, bad_db2_files, sItemStore, db2Path, "Item.db2");
    //LoadDB2(availableDb2Locales, bad_db2_files, sItemSparseStore, db2Path, "Item-sparse.db2");

    // error checks
    if (bad_db2_files.size() >= DB2FileCount)
    {
        sLog->outError("Incorrect DataDir value in mangosd.conf or ALL required *.db2 files (%d) not found by path: %sdb2", DB2FileCount, dataPath.c_str());
        exit(1);
    }
    else if (!bad_db2_files.empty())
    {
        std::string str;
        for (std::list<std::string>::iterator i = bad_db2_files.begin(); i != bad_db2_files.end(); ++i)
            str += *i + "\n";

        sLog->outError("Some required *.db2 files (%u from %d) not found or not compatible:\n%s", (uint32)bad_db2_files.size(), DB2FileCount,str.c_str());
        exit(1);
    }

    // Check loaded DBC files proper version
    if (!sItemStore.LookupEntry(68815))                     // last client known item added in 4.0.6a
    {
        sLog->outString("");
        sLog->outError("Please extract correct db2 files from client 4.0.6a 13623.");
        exit(1);
    }

    sLog->outString(">> Initialized %d db2 stores.", DB2FileCount);
    sLog->outString();
}