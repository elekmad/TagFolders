#include <TagFolder.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void Tag_init(Tag *self, const char *name, int id)
{
    strncpy(self->name, name, 19);
    self->name[20] = '\0';//Protection because strncpy might skip  the null terminating byte...
    self->id = id;
    self->next = NULL;
}

Tag *Tag_new(const char *name, int id)
{
    Tag *new_tag = malloc(sizeof(Tag));
    if(new_tag != NULL)
        Tag_init(new_tag, name, id);
    return new_tag;
}

Tag *Tag_set_next(Tag *self, Tag *next)
{
    Tag *ret = self->next;
    self->next = next;
    return ret;
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

void TagFolder_init(TagFolder *self)
{
    self->folder[0] = '\0';
    self->current = NULL;
    self->db = NULL;
}

static int TagFolder_set_db(TagFolder *self, sqlite3 *db)
{
    if(self->db != NULL)
        sqlite3_close(self->db);
    self->db = db;
}

static void TagFolder_set_current(TagFolder *self, Tag *cur)
{
    Tag_set_next(cur, self->current);
    self->current = cur;
}

void TagFolder_finalize(TagFolder *self)
{
    TagFolder_set_db(self, NULL);
    if(self->current != NULL)
        Tag_free(self->current);
}

int TagFolder_check_db_structure(TagFolder *);

int TagFolder_setup_folder(TagFolder *self, char *name)
{
    strncpy(self->folder, name, 149);
    self->folder[150] = '\0';//Protection because strncpy might skip  the null terminating byte...
    if(strlen(self->folder) > 0)
    {
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
        char *create_req = "create table tag (id INTEGER PRIMARY KEY, name varchar);", *errmsg;
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
        char *create_req = "create table file (id INTEGER PRIMARY KEY, name varchar);", *errmsg;
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
    req = "select name, id from tag;";
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
        new_tag = Tag_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1));
        Tag_set_next(new_tag, ret);
        ret = new_tag;
    }
    while(rc == SQLITE_ROW);
    sqlite3_finalize(res);

    return ret;
}

int TagFolder_create_tag(TagFolder *self, const char *name)
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

    snprintf(req, 499, "insert into tag (name) values ('%s');", name);
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
    TagFolder_begin_transaction(self);
    snprintf(req, 499, "select id from file where name = '%s';", name);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't verif if file %s already exist in db: %s\n", name, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc == SQLITE_ROW)
    {
        fprintf(stderr, "File %s already exist in db\n", name);
        TagFolder_rollback_transaction(self);
        return 0;
    }

    snprintf(req, 499, "insert into file (name) values ('%s');", name);
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

int TagFolder_get_tag(TagFolder *self, const char *tag)
{
    Tag *cur_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, tag_id;
//Take "file to tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
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
    sqlite3_finalize(res);
    cur_tag = self->current;
    while(cur_tag != NULL && cur_tag->id != tag_id)
        cur_tag = cur_tag->next;

    if(cur_tag == NULL)//We don't already have get this tag
    {
        cur_tag = Tag_new(tag, tag_id);
        TagFolder_set_current(self, cur_tag);
    }
    return ret;
}

int TagFolder_release_tag(TagFolder *self, const char *tag)
{
    Tag *cur_tag, *old_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc, tag_id;
//Take "file to tag"'s id
    snprintf(req, 499, "select id from tag where name = '%s';", tag);
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
    sqlite3_finalize(res);
    cur_tag = self->current;
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
            TagFolder_set_current(self, NULL);
        Tag_set_next(cur_tag, NULL);
        Tag_free(cur_tag);
    }
    return ret;
}

int TagFolder_list_current_files(TagFolder *self)
{
    Tag *cur_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[50000], *ptr, *errmsg;
    int rc, tag_id;
    cur_tag = self->current;
    if(cur_tag != NULL)
    {
        ptr = req + sprintf(req, "select %s.name from (select file.name, file.id from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = \"%s\") as %s", cur_tag->name, cur_tag->name, cur_tag->name);
        while(cur_tag != NULL)
        {
            cur_tag = cur_tag->next;
            if(cur_tag == NULL)
                break;
            ptr += sprintf(ptr, " inner join (select file.name, file.id from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = \"%s\") as %s on %s.id = %s.id", cur_tag->name, cur_tag->name, cur_tag->name, self->current->name);
        }
    }
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get file list : %s\n", sqlite3_errmsg(self->db));
        return -1 ;
    }
 
    rc = sqlite3_step(res);
    while(rc == SQLITE_ROW)
    {
        printf("File : %s\n", sqlite3_column_text(res, 0));
	rc = sqlite3_step(res);
    }
    sqlite3_finalize(res);
}
