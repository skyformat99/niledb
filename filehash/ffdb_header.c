/**
 * Copyright (C) <2008> Jefferson Science Associates, LLC
 *                      Under U.S. DOE Contract No. DE-AC05-06OR23177
 *
 *                      Thomas Jefferson National Accelerator Facility
 *
 *                      Jefferson Lab
 *                      Scientific Computing Group,
 *                      12000 Jefferson Ave.,      
 *                      Newport News, VA 23606 
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ----------------------------------------------------------------------------
 * Description:
 *     Pure File Based Hash Database - top level support
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ffdb_header.h"
#include "ffdb_db.h"
#include "ffdb_pagepool.h"


/*
 * Open a database handle
 */
FILEDB_DB*
filedb_dbopen(const char* fname, int flags, int mode, const void* openinfo)
{
  FFDB_HASHINFO* info = (FFDB_HASHINFO*)openinfo;
  fprintf(stdout, "%s: open to dbopen= %s\n", __func__, fname);
  fprintf(stdout, "struct= %d %d %lu %d %d %d\n", info->bsize, info->nbuckets, info->cachesize, info->rearrangepages, info->userinfolen, info->numconfigs);
  return (FILEDB_DB*)ffdb_dbopen(fname, flags, mode, info);
}


/*
 * Close a database handle
 */
int
filedb_close(FILEDB_DB* dbhh)
{
  FFDB_DB* dbh = (FFDB_DB*)dbhh;
  int ret = 0;
  if (dbh != NULL)
  {
    ret = dbh->close(dbh);
    dbh = 0;
  }
  return ret;
}


/*
 * Set user information for the database
 */
unsigned int
filedb_num_configs(const FILEDB_DB* db)
{
  return ffdb_num_configs((FFDB_DB*)db);
}


/*
 * Set user information for the database
 *
 */
int
filedb_set_user_info(FILEDB_DB* db, unsigned char* data, unsigned int len)
{
  int ret;
  ret = ffdb_set_user_info((FFDB_DB*)db, data, len);
  return ret;
}


/*
 * Get user information for the database
 */
int
filedb_get_user_info(const FILEDB_DB* db, unsigned char data[], unsigned int* len)
{
  int ret;
  ret = ffdb_get_user_info((FFDB_DB*)db, data, len);
  return ret;
}


/*
 * Get maximum user information length in bytes allocated
 */
unsigned int
filedb_max_user_info_len(const FILEDB_DB* db)
{
  return ffdb_max_user_info_len((FFDB_DB*)db);
}


/*
 * A routine which reset the database handle under panic mode
 */
void
filedb_dbpanic(FILEDB_DB* dbp)
{
  ffdb_dbpanic((FFDB_DB*)dbp);
}


/*
 * Return all keys to vectors in binary form of strings
 */
void
filedb_get_all_keys(FILEDB_DB* dbhh, void* keyss, unsigned int* num)
{
  FFDB_DB*  dbh  = (FFDB_DB*)dbhh;
  FFDB_DBT** keys = (FFDB_DBT**)keyss;
  int (*cursor) (const struct __ffdb *, ffdb_cursor_t **, unsigned int type) = dbh->cursor;

  FFDB_DBT*  dbkey;
  ffdb_cursor_t* crp;
  int  ret;
  unsigned int  old_num;

  /* Initialize holding location */
  old_num = 0;
  *num    = 1;
  *keys = (FFDB_DBT*)calloc(1, (*num)*sizeof(FFDB_DBT));
  if (*keys == NULL)
  {
    fprintf(stderr, "%s: cannot create initial space", __func__);
    exit(1);
  }

  /* create cursor */
  ret = (*cursor)(dbh, &crp, FFDB_KEY_CURSOR);
  if (ret != 0)
  {
    fprintf(stderr, "%s: create Cursor Error", __func__);
    exit(1);
  }

  /* Create initial space */
  *keys = (FFDB_DBT*)realloc(*keys, (*num)*sizeof(FFDB_DBT));
  if (*keys == NULL)
  {
    fprintf(stderr, "%s: cannot create space", __func__);
    exit(1);
  }

  /* First test */
  dbkey = &((*keys)[old_num]);
  dbkey->data = 0;
  dbkey->size = 0;

  while ((ret = crp->get(crp, dbkey, 0, FFDB_NEXT)) == 0) 
  {
#if 1
    if (old_num == 0)
    {
      printf("%s: key[%d] = 0x%p  sz= %d  v= --", __func__, old_num, dbkey->data, dbkey->size);
      for(int i=0; i < dbkey->size; ++i)
      {
	printf(" %x", ((unsigned char*)(dbkey->data))[i]);
      }
      printf("--\n");
      /* exit(0); */
    }
#endif

    /* Need to create space for next key */
    old_num = *num;
    *num += 1;
    *keys = (FFDB_DBT*)realloc(*keys, (*num)*sizeof(FFDB_DBT));
    if (*keys == NULL)
    {
      fprintf(stderr, "%s: cannot create intermediate", __func__);
      exit(1);
    }

    /* prepare */
    dbkey = &((*keys)[old_num]);
    dbkey->data = 0;
    dbkey->size = 0;
  }

  if (ret != FFDB_NOT_FOUND)
  {
    fprintf(stderr, "%s:  create Cursor Error", __func__);
    exit(1);
  }
  
  /* Remove the test space. The space within the key was not allocated, so do not need an additional free */
  *num -= 1;
  *keys = (FFDB_DBT*)realloc(*keys, (*num)*sizeof(FFDB_DBT));
  if (*keys == NULL)
  {
    fprintf(stderr, "%s: cannot create intermediate", __func__);
    exit(1);
  }

  /* close cursor */
  if (crp != NULL)
    crp->close(crp);
}


/**
 * Return all keys & data to vectors in binary form of strings
 */
void
filedb_get_all_pairs(FILEDB_DB* dbhh, FILEDB_DBT* keyss, FILEDB_DBT* valss, unsigned int* num)
{
  FFDB_DB*  dbh  = (FFDB_DB*)dbhh;
  FFDB_DBT* keys = (FFDB_DBT*)keyss;
  FFDB_DBT* vals = (FFDB_DBT*)valss;
  int (*cursor) (const struct __ffdb *, ffdb_cursor_t **, unsigned int type) = dbh->cursor;

  FFDB_DBT  dbkey, dbdata;
  ffdb_cursor_t* crp;
  int  ret;
  unsigned int  old_num;
    
  /* Initialize holding location */
  old_num = 0;
  *num    = 1;
  keys = (FFDB_DBT*)calloc(1, (*num)*sizeof(FFDB_DBT));
  if (keys == NULL)
  {
    fprintf(stderr, "%s: cannot create intermediate", __func__);
    exit(1);
  }

  vals = (FFDB_DBT*)calloc(1, (*num)*sizeof(FFDB_DBT));
  if (vals == NULL)
  {
    fprintf(stderr, "%s: cannot create intermediate", __func__);
    exit(1);
  }

  /* create cursor */
  ret = (*cursor)(dbh, &crp, FFDB_KEY_CURSOR);
  if (ret != 0)
  {
    fprintf(stderr, "%s: create Cursor Error", __func__);
    exit(1);
  }

  /* First test */
  keys[old_num].data = vals[old_num].data = 0;
  keys[old_num].size = vals[old_num].size = 0;

  while ((ret = crp->get(crp, &keys[old_num], &vals[old_num], FFDB_NEXT)) == 0) 
  {
    /* Need to create space for next key */
    old_num = *num;
    *num += 1;
    keys = (FFDB_DBT*)realloc(keys, (*num)*sizeof(FFDB_DBT));
    if (keys == NULL)
    {
      fprintf(stderr, "%s: cannot create intermediate", __func__);
      exit(1);
    }

    vals = (FFDB_DBT*)realloc(vals, (*num)*sizeof(FFDB_DBT));
    if (vals == NULL)
    {
      fprintf(stderr, "%s: cannot create intermediate", __func__);
      exit(1);
    }

    /* prepare */
    keys[old_num].data = vals[old_num].data = 0;
    keys[old_num].size = vals[old_num].size = 0;
  }

  if (ret != FFDB_NOT_FOUND)
  {
    fprintf(stderr, "%s:  create Cursor Error", __func__);
    exit(1);
  }
    
  /* Remove the test space. The space within the key was not allocated, so do not need an additional free */
  *num -= 1;
  keys = (FFDB_DBT*)realloc(keys, (*num)*sizeof(FFDB_DBT));
  if (keys == NULL)
  {
    fprintf(stderr, "%s: cannot create intermediate", __func__);
    exit(1);
  }

  vals = (FFDB_DBT*)realloc(vals, (*num)*sizeof(FFDB_DBT));
  if (vals == NULL)
  {
    fprintf(stderr, "%s: cannot create intermediate", __func__);
    exit(1);
  }

  /* close cursor */
  if (crp != NULL)
    crp->close(crp);
}


/**
 * get key and data pair from a database pointed by pointer dbh
 *
 * @param dbh database pointer
 * @key key associated with this data. This key must be string form
 * @data data to be stored into the database. It is in string form
 *
 * @return 0 on success. Otherwise failure
 */
int filedb_get_data(FILEDB_DB* dbhh, const FILEDB_DBT* key, FILEDB_DBT* data) 
{
  FFDB_DB*  dbh    = (FFDB_DB*)dbhh;
  FFDB_DBT* dbkey  = (FFDB_DBT*)key;
  FFDB_DBT* dbdata = (FFDB_DBT*)data;

  /* Initialize */
  data->data = 0;
  data->size = 0;

  /* now retrieve data from database*/
  return dbh->get(dbh, dbkey, dbdata, 0);
}  
  

/**
 * Insert key and data pair in string format into the database
 *
 * @param dbh database pointer
 * @key key associated with this data. This key must be string form
 * @data data to be stored into the database. It is in string form
 *
 * @return 0 on success. Otherwise failure
 */
int filedb_insert_data(FILEDB_DB* dbhh, const FILEDB_DBT* key, const FILEDB_DBT* data)
{
  FFDB_DB*  dbh    = (FFDB_DB*)dbhh;
  FFDB_DBT* dbkey  = (FFDB_DBT*)key;
  FFDB_DBT* dbdata = (FFDB_DBT*)data;
          
  /* now it is time to insert */
  return dbh->put(dbh, dbkey, dbdata, 0);
}