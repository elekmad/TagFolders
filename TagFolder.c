#include <TagFolder.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <sys/time.h>

static void TagFolder_generate_filename(const char *str, char *out)
{
    int n, length;
    MD5_CTX c;
    unsigned char digest[16];
    unsigned char time_of_day[(sizeof(time_t) + sizeof(suseconds_t)) * 2];
    struct timeval tv;

    MD5_Init(&c);

    length = strlen(str);
    while (length > 0)
    {
        if (length > 512)
        {
            MD5_Update(&c, str, 512);
        }
        else
        {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }
    gettimeofday(&tv, NULL);
    MD5_Update(&c, &tv, sizeof(tv));

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n)
    {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
}

struct Tag
{
    int id;
    char name[20];
    struct Tag *next;
    TagType type;
};

void Tag_init(Tag *self, const char *name, int id, TagType type)
{
    strncpy(self->name, name, 19);
    self->name[20] = '\0';//Protection because strncpy might skip  the null terminating byte...
    self->id = id;
    self->next = NULL;
    self->type = type;
}

Tag *Tag_new(const char *name, int id, TagType type)
{
    Tag *new_tag = malloc(sizeof(Tag));
    if(new_tag != NULL)
        Tag_init(new_tag, name, id, type);
    return new_tag;
}

Tag *Tag_set_next(Tag *self, Tag *next)
{
    Tag *ret = self->next;
    self->next = next;
    return ret;
}

Tag *Tag_get_next(Tag *self)
{
    return self->next;
}

void Tag_set_type(Tag *self, TagType type)
{
    self->type = type;
}

const char *Tag_get_name(Tag *self)
{
   return self->name;
}

int Tag_get_id(Tag *self)
{
    return self->id;
}

TagType Tag_get_type(Tag *self)
{
    return self->type;
}

void Tag_finalize(Tag *self)
{
    Tag *ret = Tag_set_next(self, NULL);
    if(ret != NULL)
        Tag_free(ret);
}

void Tag_free(Tag *self)
{
    Tag_finalize(self);
    free(self);
}

struct File
{
    int id;
    char name[20];
    struct File *next;
};

void File_init(File *self, const char *name, int id)
{
    strncpy(self->name, name, 19);
    self->name[20] = '\0';//Protection because strncpy might skip  the null terminating byte...
    self->id = id;
    self->next = NULL;
}

File *File_new(const char *name, int id)
{
    File *new_tag = malloc(sizeof(File));
    if(new_tag != NULL)
        File_init(new_tag, name, id);
    return new_tag;
}

File *File_set_next(File *self, File *next)
{
    File *ret = self->next;
    self->next = next;
    return ret;
}

File *File_get_next(File *self)
{
    return self->next;
}

const char *File_get_name(File *self)
{
    return self->name;
}


int File_get_id(File *self)
{
    return self->id;
}

void File_finalize(File *self)
{
    File *ret = File_set_next(self, NULL);
    if(ret != NULL)
        File_free(ret);
}

void File_free(File *self)
{
    File_finalize(self);
    free(self);
}


void TagFolder_init(TagFolder *self)
{
    self->folder[0] = '\0';
    self->current_includes = NULL;
    self->current_excludes = NULL;
    self->db = NULL;
}

TagFolder *TagFolder_new(void)
{
    TagFolder *self = malloc(sizeof(TagFolder));
    if(self != NULL)
        TagFolder_init(self);
    return self;
}

static int TagFolder_set_db(TagFolder *self, sqlite3 *db)
{
    if(self->db != NULL)
        sqlite3_close_v2(self->db);
    self->db = db;
}

static void TagFolder_set_current_include(TagFolder *self, Tag *cur)
{
    if(cur != NULL)
        Tag_set_next(cur, self->current_includes);
    self->current_includes = cur;
}

static void TagFolder_set_current_exclude(TagFolder *self, Tag *cur)
{
    if(cur != NULL)
        Tag_set_next(cur, self->current_excludes);
    self->current_excludes = cur;
}

void TagFolder_finalize(TagFolder *self)
{
    TagFolder_set_db(self, NULL);
    if(self->current_includes != NULL)
        Tag_free(self->current_includes);
    if(self->current_excludes != NULL)
        Tag_free(self->current_excludes);
}

void TagFolder_free(TagFolder *self)
{
    TagFolder_finalize(self);
    free(self);
}

int TagFolder_check_db_structure(TagFolder *);

int TagFolder_setup_folder(TagFolder *self, char *name)
{
    strncpy(self->folder, name, 149);
    self->folder[150] = '\0';//Protection because strncpy might skip  the null terminating byte...
    if(strlen(self->folder) > 0)
    {
        Tag *tags, *ptr, *old_ptr = NULL;
        int rc;
        sqlite3 *db;
        char *req, dbname[160];
        strcpy(dbname, self->folder);
        if(dbname[strlen(dbname) - 1] != '/')
            strcat(dbname, "/");
        strcat(dbname, ".TagFolder.sql");
        rc = sqlite3_open(dbname, &db);
        if( rc )
        {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return -1 ;
        }
        TagFolder_set_db(self, db);
        TagFolder_check_db_structure(self);
        tags = TagFolder_list_tags(self);
        ptr = tags;
        if(tags != NULL)
        {
            while(ptr != NULL)
            {
                switch(Tag_get_type(ptr))
                {
                    case TagTypeInclude :
                        old_ptr = ptr;
                        ptr = Tag_get_next(ptr);
                        break;
                    case TagTypeExclude :
                        {
                            Tag *cur_tag_to_add = ptr;
                            ptr = Tag_get_next(cur_tag_to_add);
                            if(old_ptr == NULL)
                                tags = ptr;//To be able to free tags at the end, we must have only includes.
                            else
                                Tag_set_next(old_ptr, ptr);
                            Tag_set_next(cur_tag_to_add, NULL);
                            TagFolder_set_current_exclude(self, cur_tag_to_add);
                        }
                        break;
                }
            }
            Tag_free(tags);
        }
    }
}

static int TagFolder_begin_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "BEGIN;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

static int TagFolder_commit_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "COMMIT;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

static int TagFolder_rollback_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "ROLLBACK;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

int TagFolder_check_db_structure(TagFolder *self)
{
    int rc;
    sqlite3_stmt *res;
    char *req;
    if(self->db == NULL)
        return -1;

    TagFolder_begin_transaction(self);
//Table Tag
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tag';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tag (id INTEGER PRIMARY KEY, type integer, name varchar);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
//Table File
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='file';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table file (id INTEGER PRIMARY KEY, name varchar, filename char(32));", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
//Table Tag to Tag
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tagtag';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagtag(primid int, secondid int);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);

//Table Tag to File
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tagfile';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagfile(tagid int, fileid int);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
    TagFolder_commit_transaction(self);

    return 0;
}

Tag *TagFolder_list_tags(TagFolder *self)
{
    Tag *ret = NULL;
    sqlite3_stmt *res;
    char *req;
    int rc;
    req = "select name, id, type from tag;";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);

    if( rc )
    {
        fprintf(stderr, "Can't list tags: %s\n", sqlite3_errmsg(self->db));
        return ret ;
    }
    do
    {
        Tag *new_tag;
        rc = sqlite3_step(res);
        if(rc != SQLITE_ROW)
            break;
        new_tag = Tag_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1), sqlite3_column_int(res, 2));
        Tag_set_next(new_tag, ret);
        ret = new_tag;
    }
    while(rc == SQLITE_ROW);
    sqlite3_finalize(res);

    return ret;
}

int TagFolder_create_tag(TagFolder *self, const char *name, TagType type)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[50], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);
    snprintf(req, 499, "select id from tag where name = '%s';", name);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't verif if tag %s already exist: %s\n", name, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc == SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s already exist\n", name);
        TagFolder_rollback_transaction(self);
        return 0;
    }

    snprintf(req, 499, "insert into tag (name, type) values ('%s', %d);", name, (int)type);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't add tag %s : %s\n", name, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_create_file_in_db(TagFolder *self, const char *name)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[50], *errmsg;
    int rc;
    char db_name[33];
//We allow multiple files to be with the same name, generating uniq filename for each.
    TagFolder_generate_filename(name, db_name);

    TagFolder_begin_transaction(self);
    snprintf(req, 499, "insert into file (name, filename) values ('%s', '%s');", name, db_name);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't add file %s in db : %s\n", name, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_tag_a_tag(TagFolder *self, const char *tag_to_tag, const char *tag)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, tag_to_tag_id, tag_id;
    TagFolder_begin_transaction(self);
//Take "tag to tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag_to_tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag_to_tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag_to_tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_to_tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
//Take "tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

//Verif if file is already tagued
    snprintf(req, 499, "select count(primid) from tagtag where primid = %d and secondid = %d", tag_id, tag_to_tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get count : %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Can't get count\n");
        TagFolder_rollback_transaction(self);
        return -1;
    }
    rc = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    if(rc > 0)
    {
        fprintf(stderr, "Tag %s already tagued by %s\n", tag_to_tag, tag);
        TagFolder_rollback_transaction(self);
        return 0 ;
    }


    snprintf(req, 499, "insert into tagtag (primid, secondid) values (%d, %d);", tag_id, tag_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't tag %s with %s in db : %s\n", tag_to_tag, tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_untag_a_tag(TagFolder *self, const char *tag_to_tag, const char *tag)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, tag_to_tag_id, tag_id;
    TagFolder_begin_transaction(self);
//Take "tag to tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag_to_tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag_to_tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag_to_tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_to_tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
//Take "tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    snprintf(req, 499, "delete from tagtag where primid = %d and secondid = %d;", tag_id, tag_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't untag %s from %s in db : %s\n", tag, tag_to_tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_tag_a_file(TagFolder *self, const char *file_to_tag, const char *tag)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, file_to_tag_id, tag_id;
    TagFolder_begin_transaction(self);
//Take "file to tag"'s id
    snprintf(req, 499, "select id from file where name = '%s';", file_to_tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get File %s's id : %s\n", file_to_tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "File %s do not exist in db\n", file_to_tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    file_to_tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
//Take "tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

//Verif if file is already tagued
    snprintf(req, 499, "select count(tagid) from tagfile where tagid = %d and fileid = %d", tag_id, file_to_tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get count : %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Can't get count\n");
        TagFolder_rollback_transaction(self);
        return -1;
    }
    rc = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    if(rc > 0)
    {
        fprintf(stderr, "File %s already tagued by %s\n", file_to_tag, tag);
        TagFolder_rollback_transaction(self);
        return 0 ;
    }

    snprintf(req, 499, "insert into tagfile (tagid, fileid) values (%d, %d);", tag_id, file_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't tag %s with %s in db : %s\n", file_to_tag, tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_untag_a_file(TagFolder *self, const char *file_to_tag, const char *tag)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, file_to_tag_id, tag_id;
    TagFolder_begin_transaction(self);
//Take "file to tag"'s id
    snprintf(req, 499, "select id from file where name = '%s';", file_to_tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get File %s's id : %s\n", file_to_tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "File %s do not exist in db\n", file_to_tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    file_to_tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
//Take "tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        TagFolder_rollback_transaction(self);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    snprintf(req, 499, "delete from tagfile where tagid = %d and fileid = %d;", tag_id, file_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't untag %s from %s in db : %s\n", tag, file_to_tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_select_tag(TagFolder *self, const char *tag)
{
    Tag *cur_tag, *old_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, tag_id;
    TagType type;
//Take "file to tag"'s id
    snprintf(req, 499, "select id, type from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    type = (TagType)sqlite3_column_int(res, 1);
    sqlite3_finalize(res);
    switch(type)
    {
        //Select an include tag means add it into current includes list
        case TagTypeInclude :
            cur_tag = self->current_includes;
            while(cur_tag != NULL && cur_tag->id != tag_id)
                cur_tag = cur_tag->next;

            if(cur_tag == NULL)//We don't already have get this tag
            {
                cur_tag = Tag_new(tag, tag_id, type);
                TagFolder_set_current_include(self, cur_tag);
            }
            break;
        //Select an exclude tag means delete it from current excludes list
        case TagTypeExclude :
            cur_tag = self->current_excludes;
            old_tag = NULL;
            while(cur_tag != NULL && cur_tag->id != tag_id)
            {
                old_tag = cur_tag;
                cur_tag = cur_tag->next;
            }
    
            if(cur_tag != NULL)//Tag to release found
            {
                if(old_tag != NULL)
                    Tag_set_next(old_tag, cur_tag->next);
                else
                    TagFolder_set_current_exclude(self, NULL);
                Tag_set_next(cur_tag, NULL);
                Tag_free(cur_tag);
            }
            break;
    }
    return ret;
}

//Do not free them !
Tag *TagFolder_get_selected_tags(TagFolder *self)
{
   return self->current_includes;
}

int TagFolder_unselect_tag(TagFolder *self, const char *tag)
{
    Tag *cur_tag, *old_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500];
    int rc, tag_id;
    TagType type;
//Take "file to tag"'s id
    snprintf(req, 499, "select id, type from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        return -1;
    }
    tag_id = sqlite3_column_int(res, 0);
    type = (TagType)sqlite3_column_int(res, 1);
    sqlite3_finalize(res);
    switch(type)
    {
        //Unselect an include tag means delete it from current includes list
        case TagTypeInclude :
            cur_tag = self->current_includes;
            old_tag = NULL;
            while(cur_tag != NULL && cur_tag->id != tag_id)
            {
                old_tag = cur_tag;
                cur_tag = cur_tag->next;
            }
    
            if(cur_tag != NULL)//Tag to release found
            {
                if(old_tag != NULL)
                    Tag_set_next(old_tag, cur_tag->next);
                else
                    TagFolder_set_current_include(self, NULL);
                Tag_set_next(cur_tag, NULL);
                Tag_free(cur_tag);
            }
            break;
        //Unselect an exclude tag means add it into current excludes list
        case TagTypeExclude :
            cur_tag = self->current_excludes;
            while(cur_tag != NULL && cur_tag->id != tag_id)
                cur_tag = cur_tag->next;

            if(cur_tag == NULL)//We don't already have get this tag
            {
                cur_tag = Tag_new(tag, tag_id, type);
                TagFolder_set_current_exclude(self, cur_tag);
            }
            break;
    }
    return ret;
}

Tag *TagFolder_get_tags_tagging_specific_file(TagFolder *self, const char *file)
{
    Tag *ret = NULL;
    sqlite3_stmt *res;
    char req[500];
    int rc, file_id;
    snprintf(req, 499, "SELECT id FROM file WHERE name = '%s';", file);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    if( rc )
    {
        fprintf(stderr, "Can't get file %s's id : %s\n", file, sqlite3_errmsg(self->db));
        return ret ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "File %s do not exist in db\n", file);
        return ret;
    }
    file_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
 
    snprintf(req, 499, "SELECT name, id, type FROM tag inner join tagfile on tag.id = tagfile.tagid WHERE fileid = '%d';", file_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    if( rc )
    {
        fprintf(stderr, "Can't list tags for file %s : %s\n", file, sqlite3_errmsg(self->db));
        return ret;
    }
    do
    {
        Tag *new_tag;
        rc = sqlite3_step(res);
        if(rc != SQLITE_ROW)
            break;
        new_tag = Tag_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1), (TagType)sqlite3_column_int(res, 2));
        Tag_set_next(new_tag, ret);
        ret = new_tag;
    }
    while(rc == SQLITE_ROW);
    sqlite3_finalize(res);

    return ret;
}

File *TagFolder_list_current_files(TagFolder *self)
{
    Tag *cur_tag;
    File *ret = NULL;
    sqlite3_stmt *res;
    char req[50000], *ptr = req, *errmsg, *main_name = "result", where_clause[5000], *ptr_where = where_clause;
    int rc, tag_id;
    req[0] = '\0';//Force empty
    where_clause[0] = '\0';
    cur_tag = self->current_includes;
    if(cur_tag != NULL)
    {
        ptr += sprintf(ptr, "select %s.name, %s.id from (select file.name, file.id from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = '%s') as %s", main_name, main_name, cur_tag->name, main_name);
        while(cur_tag != NULL)
        {
            cur_tag = cur_tag->next;
            if(cur_tag == NULL)
                break;
            ptr += sprintf(ptr, " inner join (select file.name, file.id from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = '%s') as %s on %s.id = %s.id", cur_tag->name, cur_tag->name, cur_tag->name, main_name);
        }
    }
    cur_tag = self->current_excludes;
    if(cur_tag != NULL)
    {
        if(req[0] != '\0')//strlen(req) > 0 but we just want to know if not 0, not length, so strlen risk to be longer...
            ptr += sprintf(ptr, " left outer join (select f.id, f.name from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id", cur_tag->name, cur_tag->name, cur_tag->name, main_name);
        else
            ptr += sprintf(ptr, "select %s.name, %s.id from file as %s left outer join (select f.id, f.name from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id", main_name, main_name, main_name, cur_tag->name, cur_tag->name, cur_tag->name, main_name);
        ptr_where += sprintf(ptr_where, " where %s.id is null", cur_tag->name);
        while(cur_tag != NULL)
        {
            cur_tag = cur_tag->next;
            if(cur_tag == NULL)
                break;
            ptr += sprintf(ptr, " left outer join (select f.id, f.name from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id;", cur_tag->name, cur_tag->name, cur_tag->name, main_name);
            ptr_where += sprintf(ptr_where, " and %s.id is null", cur_tag->name);
        }
    }
    if(req[0] == '\0')//strlen(req) == 0 but we just want to know if 0 or not, not length, so strlen risk to be longer...
        strcpy(req, "select name, id from file");
    else
        strcat(req, where_clause);
    fprintf(stderr, "%s\n", req);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get file list : %s\n", sqlite3_errmsg(self->db));
        return ret ;
    }
 
    rc = sqlite3_step(res);
    while(rc == SQLITE_ROW)
    {
        File *new_file = File_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1));
        File_set_next(new_file, ret);
        ret = new_file;
	rc = sqlite3_step(res);
    }
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_delete_tag(TagFolder *self, const char *tag)
{
    int ret = 0;
    char req[500], *errmsg;
    int tag_id, rc;
    sqlite3_stmt *res;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "select id from tag where name = '%s';", tag);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);

    if( rc )
    {
        fprintf(stderr, "Can't get tag %s's id : %s\n", tag, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);

    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s do not exist in db\n", tag);
        TagFolder_rollback_transaction(self);
        return 0;
    }
    tag_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
    snprintf(req, 499, "delete from tag where id = %d;", tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %s from tag table : %s\n", tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from tagtag where primid = %d or secondid = %d;", tag_id, tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %s from tagtag table : %s\n", tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from tagfile where tagid = %d;", tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %s tagfile table : %s\n", tag, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_delete_file(TagFolder *self, const char *file)
{
    int ret = 0;
    char req[500], *errmsg;
    int file_id, rc;
    sqlite3_stmt *res;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "select id from file where name = '%s';", file);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);

    if( rc )
    {
        fprintf(stderr, "Can't get file %s's id : %s\n", file, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);

    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "File %s do not exist in db\n", file);
        TagFolder_rollback_transaction(self);
        return 0;
    }
    file_id = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);
    snprintf(req, 499, "delete from file where id = %d;", file_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete file %s from file table : %s\n", file, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from filefile where fileid = %d;", file_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete file %s filefile table : %s\n", file, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    TagFolder_commit_transaction(self);
    return ret;
}
