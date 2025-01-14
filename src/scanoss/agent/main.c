// SPDX-License-Identifier: GPL-2.0-or-later
/*!
 * main.c
 *
 * The SCANOSS Agent for Fossology tool
 *
 * Copyright (C) 2018-2022 SCANOSS.COM
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
#include "snippet_scan.h"
#include <stdio.h>
#ifdef COMMIT_HASH_S
char BuildVersion[] = "scanoss build version: " VERSION_S " r(" COMMIT_HASH_S ").\n";
#else
char BuildVersion[] = "scanoss build version: NULL.\n";
#endif
char *baseTMP = "/tmp/scanoss";
int Agent_pk;
char ApiUrl[400];
char accToken[100];
char ApiPort[6];


void logme(char *msg)
{
  #ifdef _SCANOSS_LOGME
  FILE *fptr;

  // use appropriate location if you are using MacOS or Linux
  fptr = fopen("/home/fossy/snippet_scan.txt", "a");

  if (fptr == NULL)
  {
    printf("Error!");
    exit(1);
  }

  fprintf(fptr, "->%s", msg);
  fclose(fptr);
  #endif
}
void loadAgentConfiguration(PGconn *pg_conn)
{
  PGresult *result;
  char sqlA[] = "select conf_value from sysconfig where variablename='ScAPIURL';";
 
  result = PQexec(db_conn, sqlA);
  if (fo_checkPQresult(db_conn, result, sqlA, __FILE__, __LINE__))
  {
    logme("Error retrieving APIURL");
    exit(-1);
  }

  sprintf(ApiUrl, "%s", PQgetvalue(result, 0, 0));
  char sqlB[] = "select conf_value from sysconfig where variablename='ScToken';";

  result = PQexec(db_conn, sqlB);
  if (fo_checkPQresult(db_conn, result, sqlB, __FILE__, __LINE__))
  {
    logme("Error retrieving Token");
    exit(-1);
  }
  sprintf(accToken, "%s", PQgetvalue(result, 0, 0));

  char sqlC[] = "select conf_value from sysconfig where variablename='ScPort';";
 
  result = PQexec(db_conn, sqlC);
  if (fo_checkPQresult(db_conn, result, sqlC, __FILE__, __LINE__))
  {
    logme("Error retrieving Port");
    exit(-1);
  }
  sprintf(ApiPort, "%s", PQgetvalue(result, 0, 0));

}



int createTables(PGconn* pgConn)
{
  char sql[8192];
  PGresult* result;

  if (!fo_tableExists(pgConn, "scanoss_fileinfo")) {
   
    snprintf(sql, sizeof(sql), "\
          CREATE TABLE scanoss_fileinfo (\
	          pfile_fk int4 NOT NULL,\
            matchtype text NULL,\
            lineranges text NULL,\
            purl varchar NULL,\
            url varchar NULL,\
            filepath varchar NULL,\
            fileinfo_pk serial4 NOT NULL\
          );");
     
    result = PQexec(pgConn, sql);
    if (fo_checkPQcommand(pgConn, result, sql, __FILE__, __LINE__)) {

    // Can 't create table scanoss_fileinfo
  }
  }
  char sqlPort[]="INSERT INTO sysconfig (variablename, conf_value, ui_label, vartype, group_name, group_order, description, validation_function, option_value)\
  VALUES('ScPort', '443', 'SCANOSS port', 2, 'SCANOSS', 3, 'Host port for the API (defult 443)', NULL, NULL);";

  char sqlHost[]="INSERT INTO sysconfig (variablename, conf_value, ui_label, vartype, group_name, group_order, description, validation_function, option_value) \
  VALUES('ScAPIURL', 'https://scanoss.com/api', 'SCANOSS API URL', 2, 'SCANOSS', 1, '(leave blank for default https://osskb.org/scan/direct)', NULL, NULL);";
  char sqlToken[]="INSERT INTO sysconfig ( variablename, conf_value, ui_label, vartype, group_name, group_order, description, validation_function, option_value) \
  VALUES( 'ScToken', 'fD4P8wbfWS9XHvsrzcDrB0zu', 'SCANOSS access token', 2, 'SCANOSS', 2, 'Set token to access full scanning service.', NULL, NULL);";
  result = PQexec(pgConn, sqlPort);
  if (fo_checkPQcommand(pgConn, result, sqlPort, __FILE__, __LINE__)) {
     
    // Can 't create table scanoss_fileinfo
  }
  result = PQexec(pgConn, sqlHost);
   if (fo_checkPQcommand(pgConn, result, sqlHost, __FILE__, __LINE__)) {
     
    // Can 't create table scanoss_fileinfo
  }
  result = PQexec(pgConn, sqlToken);
    if (fo_checkPQcommand(pgConn, result, sqlToken, __FILE__, __LINE__)) {
     
    // Can 't create table scanoss_fileinfo
  }



}










/*!
 * \brief main function for the scanoss
 * \param argc the number of command line arguments
 * \param argv the command line arguments
 * \return 0 on a successful program execution
 */
int main(int argc, char *argv[])
{
  int c;
  char *agent_desc = "scanoss";

  int ars_pk = 0;

  int upload_pk = 0; /* the upload primary key */
  int user_pk = 0;   // the user  primary key
  char *AgentARSName = "scanoss_ars";
  int rv;
  char sqlbuf[1024];
  char *COMMIT_HASH;
  char *VERSION;
  char agent_rev[MAXCMD];
  int CmdlineFlag = 0; /* run from command line flag, 1 yes, 0 not */

  fo_scheduler_connect(&argc, argv, &db_conn);

  COMMIT_HASH = fo_sysconfig("scanoss", "COMMIT_HASH");
  VERSION = fo_sysconfig("scanoss", "VERSION");
  sprintf(agent_rev, "%s.%s", VERSION, COMMIT_HASH);

  Agent_pk = fo_GetAgentKey(db_conn, basename(argv[0]), 0, agent_rev, agent_desc);
  createTables(db_conn);
  loadAgentConfiguration(db_conn);
  /* Process command-line */
  char filename[200];
  while ((c = getopt(argc, argv, "ic:CvVh")) != -1)
  {
    switch (c)
    {
    case 'i':
      PQfinish(db_conn); /* DB was opened above, now close it and exit */
      exit(0);
    case 'v':
      break;
    case 'c':
      break; /* handled by fo_scheduler_connect() */
    case 'C':
      CmdlineFlag = 1;
      strcpy(filename, argv[2]);
      break;
    case 'V':
      printf("%s", BuildVersion);
      PQfinish(db_conn);
      return (0);
    default:
      Usage(argv[0]);
      PQfinish(db_conn);
      exit(-1);
    }
  }

  if (CmdlineFlag == 0)  /* If no args, run from scheduler! */
  {
    user_pk = fo_scheduler_userID(); /* get user_pk for user who queued the agent */
    while (fo_scheduler_next())
    {
      upload_pk = atoi(fo_scheduler_current());
      if (GetUploadPerm(db_conn, upload_pk, user_pk) < PERM_WRITE)  /* Check Permissions */
      {
        LOG_ERROR("You have no update permissions on upload %d", upload_pk);
        continue;
      }
      rv = fo_tableExists(db_conn, AgentARSName);
      if (!rv)
      {
        rv = fo_CreateARSTable(db_conn, AgentARSName);
        if (!rv)
          return (0);
      }
      memset(sqlbuf, 0, sizeof(sqlbuf));
      snprintf(sqlbuf, sizeof(sqlbuf),
               "select ars_pk from scanoss_ars,agent \
               where agent_pk=agent_fk and ars_success=true \
               and upload_fk='%d' and agent_fk='%d'",
               upload_pk, Agent_pk);

      ars_pk = fo_WriteARS(db_conn, ars_pk, upload_pk, Agent_pk, AgentARSName, 0, 0);
      if (upload_pk == 0)
        continue;

     
      if (ProcessUpload(upload_pk) != 0) /* process the upload_pk code */
        return -1;
      ars_pk = fo_WriteARS(db_conn, ars_pk, upload_pk, Agent_pk, AgentARSName, NULL, 1);
    }
  }
  else
  {
    /* Run the scanner from command line */
    char Cmd[MAXCMD];
    char buf[300];
    sprintf(Cmd, "scanner  %s  |jq -r 'to_entries[] |select(.value[]|.licenses!=null)| \" \\(.key) -> \\( .value[]|.licenses[]|.name )\"'", filename);
    FILE *Fin = popen(Cmd, "r");
    if (!Fin)
    {
      printf("Could not open process");
    }
    while (fgets(buf, 300, Fin) != NULL)
    {
      printf("%s", buf);
    }
  }
  PQfinish(db_conn);
  fo_scheduler_disconnect(0);
  return (0);
} /* main() */
