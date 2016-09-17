#ifndef _TAG_FOLDER_H_
#define _TAG_FOLDER_H_
#include <sqlite3.h>

struct Tag
{
    int id;
    char name[20];
    struct Tag *next;
};

struct TagFolder
{
    char folder[150];
    sqlite3 *db;
    struct Tag *current;
};

#endif
