#ifndef _STORAGE_H_
#define _STORAGE_H_

#define PROPERTIES_STORAGE_KEY "props"

char * storage_get(char * store, char * key, char * value, size_t* len);
int storage_len(char * store, char * key, size_t* len);
int storage_set(char * store, char * key, char * value);
void storage_enable(void);

#endif
