#include <stdio.h>
#include <stdlib.h>


int setenv(const char *name, const char *value, int overwrite)
{
    char strPutEnv[512];

    char * hasKey = getenv ( name );

    if ( ( overwrite != 0 ) || ( hasKey == NULL ) )
    {
        snprintf( strPutEnv, sizeof( strPutEnv ), "%s=%s", name, value );
        strPutEnv[ sizeof( strPutEnv ) - 1 ] = '\0';
        _putenv( strPutEnv );
    }
    return 0;
}

