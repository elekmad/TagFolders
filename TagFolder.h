#ifndef _TAG_FOLDER_H_
#define _TAG_FOLDER_H_
#include <sqlite3.h>

struct Tag
{
    int id;
    char name[20];
    struct Tag *next;
};
typedef struct Tag Tag;

struct TagFolder
{
    char folder[150];
    sqlite3 *db;
    Tag *current;
};
typedef struct TagFolder TagFolder;

/* TagFolder.c */
void Tag_init(Tag *self, const char *name, int id);
Tag *Tag_new(const char *name, int id);
Tag *Tag_set_next(Tag *self, Tag *next);
void Tag_finalize(Tag *self);
void Tag_free(Tag *self);
void TagFolder_init(TagFolder *self);
int TagFolder_set_db(TagFolder *self, sqlite3 *db);
void TagFolder_finalize(TagFolder *self);
int TagFolder_setup_folder(TagFolder *self, char *name);
int TagFolder_check_db_structure(TagFolder *self);
Tag *TagFolder_list_tags(TagFolder *self);
int TagFolder_create_tag(TagFolder *self, const char *name);
int TagFolder_create_file_in_db(TagFolder *self, const char *name);


#endif
