// SPDX-License-Identifier: GPL-2.0-or-later
/*!
 * snippet_scan.c
 *
 * The SCANOSS Agent for Fossology tool
 *
 * Copyright (C) 2018-2021 SCANOSS.COM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*!
 * \file snippet_scan.c
 * \date
 * \brief Scanoss agent for Fossology. Scans  for licenses on osskb.org
 */
#include "snippet_scan.h"
#include <sys/stat.h>
#include <stdio.h>
int runScan(char *path, unsigned char **detected);
extern char *baseTMP;
int Verbose = 0;
PGconn *db_conn = NULL; ///< The connection to Database
extern int Agent_pk;
/**Forwarded definition for a linked list to map deprecated licenses names*/
struct node
{
  char *newName;
  char *oldName;
  struct node *next;
};

struct node *find(char *data);
void loadDeprecated();
/***************************************************************************/


/*!
 * \brief Open a file of the repository given its primary key
 * \param pFileKey the key of the file to be retrieved
 * \return Pointer to the file
*/ 
FILE *openFileByKey(long pFileKey)
{
  char sqlbuf[200];
  PGresult *result;
  sprintf(sqlbuf, "SELECT pfile_sha1 || '.' || pfile_md5 || '.' || pfile_size AS pfile_path FROM  pfile WHERE pfile_pk = %ld;", pFileKey);
  result = PQexec(db_conn, sqlbuf);
  if (fo_checkPQresult(db_conn, result, sqlbuf, __FILE__, __LINE__))
  {

    exit(-1);
  }
  char path[500];
  sprintf(path, "%s", PQgetvalue(result, 0, 0));
  return fo_RepFread("files", path);
}


/**
* \brief Retrieves the license id (license_ref.rf_pk) given its short name
*/
int getLicenseId(unsigned char *name)
{
  PGresult *result;
  char sqlbuf[500];
  sprintf(sqlbuf, "select lr.rf_pk from license_ref lr where lr.rf_shortname like '%s';", name);
  result = PQexec(db_conn, sqlbuf);

  if (fo_checkPQresult(db_conn, result, sqlbuf, __FILE__, __LINE__))
  {
    return 0;
  }

  if (PQgetvalue(result, 0, 0) != NULL)
  {
    return atoi(PQgetvalue(result, 0, 0));
  }
  else
    return 0;
}


/*!
* \brief Dumps the content of a file in the repository to a temporary file
  \param path Path to the temporay file
  \param content Buffer containing the file
  \param size Size of the file to be stored
*/
void dumpToFile(const char *path, unsigned char *content, long size)
{
  FILE *fptr;
  fptr = fopen(path, "w");

  if (fptr == NULL)
  {
    LOG_ERROR("Snippet scan: Could not create temp file")
  }
  else
  {
    fwrite(content, size, 1, fptr);
    fclose(fptr);
  }
}


/*!
 * \brief Scans a file given its key on database
 * \param key Key of the file to be scanned
 */
void scanTempFile(long key)
{
  PGresult *result;
  char aux[1000];
 
  FILE *f = openFileByKey(key);    /* Open the file from the repository by its key */
  sprintf(aux, "%s/scanoss.tmp", baseTMP); /* Create a name for the temp file, including the temp folder */
  if (f != NULL)
  {
    fseek(f, 0, SEEK_END);    /* Read the file and dump the content into the temp file*/
    int size = ftell(f);
    unsigned char *contents = calloc(1, size);
    memset(contents, '\0', size);
    rewind(f);
    size_t readSize = fread(contents, size, 1, f);
    dumpToFile(aux, contents, size);
    char * detectedLicenses[10];
    int licCount = runScan(aux, detectedLicenses);    /* Scan for licenses on the temp file */
    int i = 0;
    while(i<licCount){    /*For each item detected... */
      char * detectedLicense=detectedLicenses[i];   /*... get it name */
      if(detectedLicenses!=NULL){
        if(detectedLicense) {
          char auxSql[1000];
          int detLic= getLicenseId(detectedLicense); /* ... from name, get the key of license that matches short_name at license_ref */
          if(detLic!=0){    /** If the key is valid, insert the result on DB Table */
            sprintf(auxSql,"INSERT INTO license_file(rf_fk, agent_fk, rf_timestamp, pfile_fk) VALUES(%d,%d, now(), %ld);",detLic,Agent_pk,key);
            result = PQexec(db_conn,auxSql);
          } else {/* No license found by its spdx id, lets check if it is a deprecated license */
            
            struct node *dep = find(detectedLicense);
            if(dep) {
              int oldLic= getLicenseId(dep->oldName);
              if(oldLic!=0){ /* It is a deprecated identifier */
                sprintf(auxSql,"INSERT INTO license_file(rf_fk, agent_fk, rf_timestamp, pfile_fk) VALUES(%d,%d, now(), %ld);",oldLic,Agent_pk,key);
                result = PQexec(db_conn,auxSql);
              } 
              else 
                {
                /* It is a deprecated license but it is not included on DB */
                }
            }
          }
          free(detectedLicenses[i]);
        }
        i++;
      }
    }
    free(contents); 
   }
else {
  LOG_FATAL("Snippet_scan: The scan throws an invalid result");
  //returned null value. This should never happend
}
fclose(f);
fo_scheduler_heart(1);
}

/*!
 * \brief Runs a scan command for a given temp file name
 * \param path Path to temporary file
 * \param detected A list of detected license names
 * \return Licenses detected count.
 * \note This function allocates memory on each item of "detected" array. Ensure to free after it use,
 */
int runScan(char *path, unsigned char **detected)
{
  FILE *Fin;
  char Cmd[MAXCMD];
  unsigned char lic[1000]; 
  memset(Cmd, '\0', MAXCMD);
  int retLicensesCount = 0;

  sprintf(Cmd, "scanner  %s -o %s.json", path, path); /* Create the command to run */
  Fin = popen(Cmd, "r");  /* Run the command */
  if (!Fin)
  {
    LOG_ERROR("Snippet scan: failed to run scan ");
  }

  char dump;
  while (!feof(Fin)) /* Discard sdout messages. Results are stored in a specific json file */
  {
    dump = fgetc(Fin);
  }
  /* Try to get entries for licenses only. 
   * SCANOSS Result output contains a lot of useful information 
   * that is spent in this application. Parse using JQ
  */
 
  for (int licCount = 0; licCount < 10; licCount++)
  {
    memset(Cmd, '\0', MAXCMD);
    //sprintf(Cmd, "cat  %s.json |jq -r 'to_entries[] | \"\\(.value[0]|.licenses[%d]|.name )\"'", path, licCount);
    sprintf(Cmd, "cat  %s.json |jq -r 'to_entries[] |select(.value[]|.licenses!=null)| \"\\(.value[0]|.licenses[%d]|.name )\"'", path, licCount);
    Fin = popen(Cmd, "r");

    if (!Fin)
    {
      LOG_ERROR("Snippet scan: failed to dump scan results");
    }
    else
    {
      memset(lic, '\0', 1000);
      if (!fscanf(Fin, "%s", lic) || (lic == NULL) || (!strcmp(lic, "(null)")) || (!strcmp(lic, "null")) || (!strcmp(lic, "")) || (lic[0] == 255))
        break;
      asprintf(&detected[retLicensesCount], "%s", lic);// asprintf allocs memory. Free after use to avoid leaks
      retLicensesCount++;
    }
  }
  detected[retLicensesCount] = NULL;
  return retLicensesCount;
}

/**
 * \brief Get all pfile need to processed use upload_pk
 * \param upload_pk The upload_pk send from scheduler
 * \return 0 on OK, -1 on failure.
 */
int ProcessUpload(long upload_pk)
{

  char sqlbuf[1024];
  PGresult *result;
  loadDeprecated();
  int numrows;
  int i;
  char *uploadtree_tablename;
  mkdir(baseTMP, 0700);

  if (!upload_pk) // when upload_pk is empty
  {
    LOG_ERROR("Snippet scan: Missing upload key");
    return -1;
  }

  uploadtree_tablename = GetUploadtreeTableName(db_conn, upload_pk);
   if (NULL == uploadtree_tablename)
    uploadtree_tablename = strdup("uploadtree_a");
  /*  retrieve the records to process */
  snprintf(sqlbuf, sizeof(sqlbuf),
           "SELECT * from uploadtree_a, upload  where upload_fk = upload_pk  and upload_pk = '%ld' ", upload_pk);
  result = PQexec(db_conn, sqlbuf);
  if (fo_checkPQresult(db_conn, result, sqlbuf, __FILE__, __LINE__))
  {
    LOG_ERROR("Snippet scan: Error retrieving jobs");
    exit(-1);
  }

  numrows = PQntuples(result);
  long parent = 0;
  long realParent = 0;
  long fileMode = 0;
  long pFileFK = 0;
  /*  for each record, get it name and real parent */
  for (i = 0; i < numrows; i++)
  {

    parent = atoi(PQgetvalue(result, i, 1));
    realParent = atoi(PQgetvalue(result, i, 2));
    fileMode = atol(PQgetvalue(result, i, 5));
    /*Nothing to be done on folders entries*/
    if (parent != realParent && fileMode == 536888320)
      ;

    else
    {
      /* Ensure that it is a real file*/
      if (fileMode != 536888320)
      {
        pFileFK = atoi(PQgetvalue(result, i, 4));
        if (pFileFK != 0)
          scanTempFile(pFileFK);
      }
    }
  }
  PQclear(result);

  return (0);
}

/***********************************************
 Usage():
 Command line options allow you to write the agent so it works
 stand alone, in addition to working with the scheduler.
 This simplifies code development and testing.
 So if you have options, have a Usage().
 Here are some suggested options (in addition to the program
 specific options you may already have).
 ***********************************************/
void Usage(char *Name)
{
  printf("Usage: %s [file|folder]\n", Name);
  printf("  -i   :: initialize the database, then exit.\n");
  printf("  -v   :: verbose (-vv = more verbose)\n");
  printf("  -c   :: Specify the directory for the system configuration.\n");
  printf("  -C   <file path/folder path> :: run from command line.\n");
  printf("  -V   :: print the version info, then exit.\n");
} /* Usage() */

struct node *head = NULL;
struct node *current = NULL;
/* Linked list to search for deprecated licenses namee*/

// insert link at the first location
void insertMapping(char *oldName, char *newName)
{
  struct node *linkItem = (struct node *)malloc(sizeof(struct node));

  asprintf(&linkItem->oldName, "%s", oldName);
  asprintf(&linkItem->newName, "%s", newName);

  linkItem->next = head;

  head = linkItem;
}

int isEmpty()
{
  return head == NULL;
}
/**
 *  \brief Finds the new name of a license given its deprecated one
*/
struct node *find(char *data)
{
  struct node *current = head;
  if (head == NULL)
  {
    return NULL;
  }

  while (strcmp(current->newName, data))
  {
    if (current->next == NULL)
    {
      return NULL;
    }
    else
    {
      current = current->next;
    }
  }
  return current;
}

void loadDeprecated(){
  insertMapping("GPL-2.0+", "GPL-2.0-or-later");
  insertMapping("GPL-1.0+", "GPL-1.0-or-later");
  insertMapping("StandardML-NJ", "SMLNJ");
  insertMapping("wxWindows", "WXwindows");
}
