#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <TagFolder.h>
#include <String.h>

TagFolder folder;

int main(int argc, char **argv)
{
    int ret = 0;
    TagFolder_init(&folder);
    if(TagFolder_setup_folder(&folder, "dir") == -1)
    {
        fprintf(stderr, "Folder dir should be ok for setup\n");
        ret = -1;
    }
    TagFolder_finalize(&folder);
    return ret;
}
