#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>      // tolower()
#include "command.h"

//-----------------------------------------------------------------------------
    int check_command_file(char *filename, int *rotInc)
//-----------------------------------------------------------------------------
// Return value n:
//      n=0: no command file available
//      n<0: file name is returned in filename
//      n>0: internal GIF file #n
//      
{
    const struct GifFile {
    const char *name;
    const int rotinc;
	const int rotval;
    } gifFiles[] = {
    { "banana.jpg",              0, 10 },
    { "candle.jpg",              0, 10 },
    { "couple.jpg",              0, 10 },
    { "dog.jpg",                 0, 10 },
    { "dog_and_smileys.jpg",     2, 10 },
    { "eye_smiley.jpg",          0, 10 },
    { "groupwave.jpg",           1, 10 },
    { "joke.jpg",                0, 10 },
    { "lawn_mower.jpg",          0, 10 },
    { "laughing_smiley.jpg",     0, 10 },
    { "massbounce.jpg",          1, 10 },
    { "pov.jpg",                 0, 10 },
    { "pov_banana.jpg",          0, 10 },
    { "pov_candle.jpg",          0, 10 },
    { "smiley_sport.jpg",        0, 10 },
    { "tantrum.jpg",             0, 10 },
    { "text1.jpg",               1, 10 },
    { "text2.jpg",               1, 10 },
    { "text3.jpg",               1, 10 },
    { "text4.jpg",               1, 10 },
    { "text5.jpg",               1, 10 },
    { "tooth_smiley.jpg",        0, 10 },
    { "twilight.jpg",            0, 10 },
    { "wallbash.jpg",            0, 10 },
    { "xmas.jpg",                0, 10 },    
    { "",                        0, 0  }};
    
    int i;
    char tmp_filename[256];
    FILE *fp = fopen(CCF_COMMANDFILE, "r");
    if (fp==NULL) return CCF_ERROR;
    i = fscanf(fp, "%s", tmp_filename);
    if (i!=1) {
        fclose(fp);
        return CCF_ERROR;
    }
    
    // check for internal file name
    for (i=0; i<25; i++) 
    {
        if (strcmp(gifFiles[i].name, tmp_filename)==0) 
        {
            *rotInc = gifFiles[i].rotinc;
            fclose(fp);
            unlink(CCF_COMMANDFILE);
            return i;
        }
    }
    fclose(fp);
    unlink(CCF_COMMANDFILE);
    // convert to linux file name
    if (tmp_filename[1] != ':') return CCF_ERROR;
    if (tmp_filename[2] != '\\') return CCF_ERROR;
    sprintf(filename, "/cygdrive/%c/%s", tolower(tmp_filename[0]), tmp_filename+3);
    
    for (i=0; filename[i]; i++) {
        if (filename[i] == '\\') filename[i]='/';
    }
    return CCF_EXTERNAL_GIF;
}

#if 0
int main(void)
{
    char filename[80];
    filename[0]=0;
    
    while (1) 
    {
        int i=check_command_file(filename);
        if (i!=ERROR) printf("\nFile: %s    i=%d\n", filename, i);
        //else printf(".");
    }
    return 0;
}
#endif