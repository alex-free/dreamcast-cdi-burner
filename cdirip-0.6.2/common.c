#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "common.h"


long fread_as_little (void *buf, long length, FILE *fsource)
{
     char *tmp;
     long i;

     tmp = buf;

#ifdef BIG_ENDIAN
     for (i = length; i > 0; i--)
         if (!fread(&(tmp[i-1]), 1, 1, fsource)) return 0; // error
#else
     for (i = 0; i < length; i++)
         if (!fread(&(tmp[i]), 1, 1, fsource)) return 0; // error
#endif

return 1; // success
}

long fread_as_big (void *buf, long length, FILE *fsource)
{
     char *tmp;
     long i;

     tmp = buf;

#ifdef BIG_ENDIAN
     for (i = 0; i < length; i++)
         if (!fread(&(tmp[i]), 1, 1, fsource)) return 0; // error
#else
     for (i = length; i > 0; i--)
         if (!fread(&(tmp[i-1]), 1, 1, fsource)) return 0; // error
#endif

return 1; // success
}

long fwrite_as_little (void *buf, long length, FILE *fdest)
{
     char *tmp;
     long i;

     tmp = buf;

#ifdef BIG_ENDIAN
     for (i = length; i > 0; i--)
         if (!fwrite(&(tmp[i-1]), 1, 1, fdest)) return 0; // error
#else
     for (i = 0; i < length; i++)
         if (!fwrite(&(tmp[i]), 1, 1, fdest)) return 0; // error
#endif

return 1; // success
}

long fwrite_as_big (void *buf, long length, FILE *fdest)
{
     char *tmp;
     long i;

     tmp = buf;

#ifdef BIG_ENDIAN
     for (i = 0; i < length; i++)
         if (!fwrite(&(tmp[i]), 1, 1, fdest)) return 0; // error
#else
     for (i = length; i > 0; i--)
         if (!fwrite(&(tmp[i-1]), 1, 1, fdest)) return 0; // error
#endif

return 1; // success
}


void error_exit(long errcode, char *string)
{

char errmessage[256];

#ifdef _WIN32
HWND hWnd = NULL;
#endif

    if (errcode != ERR_GENERIC)
       printf("\n%s: %s\n", string, strerror(errno));  // string is used as Filename

    switch(errcode)
          {
          case ERR_OPENIMAGE:
                   strcpy(errmessage, "File not found\n\nYou may have typed a wrong name or path to source CDI image");
                   break;
          case ERR_SAVETRACK:
                   strcpy(errmessage, "Could not save track");
                   break;
          case ERR_READIMAGE:
                   strcpy(errmessage, "Error reading image");
                   break;
          case ERR_PATH:
                   strcpy(errmessage, "Could not find destination path");
                   break;
          case ERR_GENERIC:
          default: strcpy(errmessage, string);          // string is used as Error message
          }

#ifdef _WIN32
    MessageBoxA(hWnd, errmessage, NULL, MB_OK | MB_ICONERROR);
    ExitProcess(0);
#else
    printf(errmessage);
    exit(EXIT_FAILURE);
#endif

}
