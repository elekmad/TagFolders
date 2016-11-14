#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <unistd.h>
#include <TagFolder.h>

TagFolder folder;

int main(int argc, char **argv)
{
    char *line, prompt[50];
    TagFolder_init(&folder);
    TagFolder_setup_folder(&folder, argv[1]);
    chdir(argv[1]);

    snprintf(prompt, 49, "TagFolder:%s$ ", argv[1]);
    while(1)
    {
        line = readline(prompt);
        add_history(line);
        if(strncasecmp(line, "exit", strlen("exit")) == 0)
            break;
        else if(strncasecmp(line, "create_tag", strlen("create_tag")) == 0)
        {
            char *tag = line + strlen("create_tag");
            while(*tag == ' ')
                tag++;
            TagFolder_create_tag(&folder, tag, TagTypeInclude);
        }
        else if(strncasecmp(line, "select_tag", strlen("select_tag")) == 0)
        {
            char *tag = line + strlen("select_tag");
            while(*tag == ' ')
                tag++;
            TagFolder_select_tag(&folder, atoi(tag));
        }
        else if(strncasecmp(line, "unselect_tag", strlen("unselect_tag")) == 0)
        {
            char *tag = line + strlen("unselect_tag");
            while(*tag == ' ')
                tag++;
            TagFolder_unselect_tag(&folder, atoi(tag));
        }
        else if(strncasecmp(line, "tag_a_file", strlen("tag_a_file")) == 0)
        {
            char *tag = line + strlen("tag_a_file"), buf[50], *file;
            while(*tag == ' ')
                tag++;
            strcpy(buf, tag);
            file = strtok(buf, " ");
            tag = strtok(NULL, " ");
            TagFolder_tag_a_file(&folder, atoi(file), atoi(tag));
        }
        else if(strncasecmp(line, "import_a_file", strlen("import_a_file")) == 0)
        {
            char *path = line + strlen("import_a_file"), buf[50], *name, db_name[50];
            while(*path == ' ')
                path++;
            strcpy(buf, path);
            path = strtok(buf, " ");
            name = strtok(NULL, " ");
            
            TagFolder_create_file_in_db(&folder, name, db_name);
            symlink(path, db_name);
        }
        else if(strncasecmp(line, "list_files", strlen("list_files")) == 0)
        {
            File *files, *cur_file;
            files = TagFolder_list_current_files(&folder);
            if(files != NULL)
            {
                cur_file = files;
                while(cur_file != NULL)
                {
                    printf("File : %s %d\n", File_get_name(cur_file), File_get_id(cur_file));
                    cur_file = File_get_next(cur_file);
                }
                File_free(files);
            }
        }
        else if(strncasecmp(line, "list_tags", strlen("list_tags")) == 0)
        {
            Tag *list = TagFolder_list_tags(&folder), *cur;
            if(list != NULL)
            {
                cur = list;
                while(cur != NULL)
                {
                    printf("Tag : %s %d\n", Tag_get_name(cur), Tag_get_id(cur));
                    cur = Tag_get_next(cur);
                }
                Tag_free(list);
            }
        }
        else if(strncasecmp(line, "list_selected_tags", strlen("list_selected_tags")) == 0)
        {
            Tag *list = TagFolder_get_selected_tags(&folder), *cur;
            if(list != NULL)
            {
                cur = list;
                while(cur != NULL)
                {
                    printf("Selected Tag : %s %d\n", Tag_get_name(cur), Tag_get_id(cur));
                    cur = Tag_get_next(cur);
                }
            }
        }
    }
    TagFolder_finalize(&folder);
    return 0;
  }
