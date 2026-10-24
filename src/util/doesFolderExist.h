
#include "util/QESLog.h"
#ifndef DOESFOLDEREXIST_H
#define DOESFOLDEREXIST_H

#include <iostream>
#include <sys/stat.h>


inline bool doesDirExist(std::string pathName)
{
    bool exists = true;

    struct stat info;
    if( stat( pathName.c_str(), &info ) != 0 )
    {
        //qes::log::writef(qes::log::Level::info,  "cannot access %s\n", inputString.c_str() );
        exists = false;
    } else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
    {
        //qes::log::writef(qes::log::Level::info,  "%s is a directory\n", inputString.c_str() );
        exists = true;
    } else
    {
        //qes::log::writef(qes::log::Level::info,  "%s is no directory\n", inputString.c_str() );
        exists = false;
    }

    return exists;
}


#endif // DOESFOLDEREXIST_H