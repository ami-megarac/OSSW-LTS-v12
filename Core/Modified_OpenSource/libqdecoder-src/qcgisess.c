/******************************************************************************
 * qDecoder - http://www.qdecoder.org
 *
 * Copyright (c) 2000-2012 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * $Id: qcgisess.c 639 2012-05-07 23:44:19Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qcgisess.c CGI Session API
 */

#ifdef ENABLE_FASTCGI
#include "fcgi_stdio.h"
#else
#include <stdio.h>
#endif
#include <stdlib.h>
#include "coreTypes.h"
//#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include "qdecoder.h"
#include "internal.h"
#include "racsessioninfo.h"
#include "dbgout.h"
#include <ctype.h>

#ifdef _WIN32
#define SESSION_DEFAULT_REPOSITORY  "C:\\Windows\\Temp"
#else
#define SESSION_DEFAULT_REPOSITORY  "/tmp"
#endif

#define SESSION_DEFAULT_TIMEOUT_INTERVAL    (30 * 60)

#ifndef _DOXYGEN_SKIP

#define COOKIE_RANDOM_PORTION_LEN	12
#define MAX_PATH			4128
static bool _clear_repo(const char *session_repository_path);
static bool _clear_session(const char *session_repository_path, const char *sessionkey);
static int _is_valid_session(const char *filepath);
static bool _update_timeout(const char *filepath, time_t timeout_interval);
static char *_genuniqid(void);

#endif

/**
 * Initialize session
 *
 * @param request   a pointer of request structure returned by qcgireq_parse()
 * @param dirpath   directory path where session data will be kept
 *
 * @return  a pointer of malloced session data list (qentry_t type)
 *
 * @note
 * The returned qentry_t list must be de-allocated by calling qentry_t->free().
 * And if you want to append or remove some user session data, use qentry_t->*()
 * functions then finally call qcgisess_save() to store updated session data.
 */
qentry_t *qcgisess_init(qentry_t *request, const char *dirpath)
{
    char *ishttps = getenv("HTTPS");
    // check content flag
    if (qcgires_getcontenttype(request) != NULL) {/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Memory cleared properly in end_handler. */
        DEBUG_CODER("Should be called before qRequestSetContentType().");
        return NULL;
    }

    qentry_t *session = qEntry();/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    if (session == NULL) return NULL;
    // check session status & get session id
    bool new_session;
    char *sessionkey;
    char *fileValidation = NULL;
    char *gen_uniq_id = NULL;
    if (request->getstr(request, SESSION_ID, false) != NULL)/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    {
        sessionkey = request->getstr(request, SESSION_ID, true);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
        new_session = false;
    }
    else
    { // new session
        gen_uniq_id = _genuniqid();/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
        //make sure that unique_id should be valid file name.
        fileValidation = strchr(gen_uniq_id, '/');
        if (fileValidation)
        {
            return NULL;
        }
        else
        {
            sessionkey = gen_uniq_id;
            new_session = true;
        }
    }
    // make storage path for session
    char session_repository_path[PATH_MAX] = {0};
    char session_storage_path[MAX_PATH] = {0};
    char session_timeout_path[MAX_PATH] = {0};
    time_t session_timeout_interval = (time_t)SESSION_DEFAULT_TIMEOUT_INTERVAL; // seconds
    int retval = 0;

    if (dirpath != NULL) strncpy(session_repository_path, dirpath,
                                     sizeof(session_repository_path));
    else strncpy(session_repository_path, SESSION_DEFAULT_REPOSITORY,
                     sizeof(session_repository_path));
    retval = snprintf(session_storage_path, sizeof(session_storage_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_STORAGE_EXTENSION);
    if(retval < 0 || retval >= (signed)sizeof(session_storage_path))
    {
	    TCRIT("Buffer overflow\n");
	    return NULL;
    }
    retval = snprintf(session_timeout_path, sizeof(session_timeout_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_TIMEOUT_EXTENSION);
    if(retval < 0 || retval >= (signed)sizeof(session_timeout_path))
    {
	    TCRIT("Buffer overflow\n");
	    return NULL;
    }

    // validate exist session
    if (new_session == false) {
	int donotdestroy = session->getint(session, DO_NOT_DESTROY);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */

		if(donotdestroy==1) {
			//TODO: Take security measures
		} else {

    	    int valid = _is_valid_session(session_timeout_path);
	        if (valid <= 0) { // expired or not found
            	if (valid < 0) {
                    session->load(session, session_storage_path);
                    uint32 racsession_id = session->getint(session, "racsession_id");
                    racsessinfo_unregister_session(racsession_id, SESSION_UNREGISTER_REASON_LOGOUT);
                    _q_unlink(session_storage_path);/* Fortify [Path Manipulation]:: False Positive */  /* Fortify [Race Condition: File System Access]:: False Positive *//* Reason for Race Condition: File System Access False Positve - The file does not change between the calls of stat() and _q_unlink() */
                    _q_unlink(session_timeout_path);/* Fortify [Path Manipulation]:: False Positive */  /* Fortify [Race Condition: File System Access]:: False Positive *//* Reason for Race Condition: File System Access False Positve - The file does not change between the calls of stat() and _q_unlink() */
                }

            	// remake storage path
                free(sessionkey);
                gen_uniq_id = _genuniqid();/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
                //make sure that unique_id should be valid file name.
                fileValidation = strchr(gen_uniq_id, '/');
                if (fileValidation)
                {
                    return NULL;
                }
                else
                {
                    sessionkey = gen_uniq_id;
                }
	            snprintf(session_storage_path, sizeof(session_storage_path),
            	         "%s/%s%s%s",
        	             session_repository_path,
    	                 SESSION_PREFIX, sessionkey, SESSION_STORAGE_EXTENSION);
	            snprintf(session_timeout_path, sizeof(session_timeout_path),
            	         "%s/%s%s%s",
        	             session_repository_path,
    	                 SESSION_PREFIX, sessionkey, SESSION_TIMEOUT_EXTENSION);
	
	            // set flag
            	new_session = true;
        	}
		}
    }

    // if new session, set session id
    if (new_session == true) {
        if (ishttps == NULL) {
            qcgires_setcookie(request, SESSION_ID, sessionkey, 0, "/", NULL, false);
        }
        else {
        /*only For https connection secure flasg has to be added */
            if(strncmp(ishttps, "on", 2) == 0) {
                qcgires_setcookie(request, SESSION_ID, sessionkey, 0, "/", NULL, true);
            }
        }
        // force to add session_in to query list
        request->putstr(request, SESSION_ID, sessionkey, true);

        // save session informations
        char created_sec[10+1];
        snprintf(created_sec, sizeof(created_sec), "%ld", (long int)time(NULL));
        session->putstr(session, INTER_SESSIONID, sessionkey, false);
        session->putstr(session, INTER_SESSION_REPO, session_repository_path, false);
        session->putstr(session, INTER_CREATED_SEC, created_sec, false);
        session->putint(session, INTER_CONNECTIONS, 1, false);

        // set timeout interval
        qcgisess_settimeout(session, session_timeout_interval);
    } else { // read session properties

        // read exist session informations
        session->load(session, session_storage_path);

        // update session informations
        int conns = session->getint(session, INTER_CONNECTIONS);
        session->putint(session, INTER_CONNECTIONS, ++conns, true);

        // set timeout interval
        qcgisess_settimeout(session, session->getint(session, INTER_INTERVAL_SEC));
    }

    free(sessionkey);

    // set globals
    return session;
}

/**
 * Set the auto-expiration seconds about user session
 *
 * @param session   a pointer of session structure
 * @param seconds   expiration seconds
 *
 * @return  true if successful, otherwise returns false
 *
 * @note Default timeout is defined as SESSION_DEFAULT_TIMEOUT_INTERVAL. 1800 seconds
 */
bool qcgisess_settimeout(qentry_t *session, time_t seconds)
{
    if (seconds <= 0) return false;
    session->putint(session, INTER_INTERVAL_SEC, (int)seconds, true);
    return true;
}

/**
 * Get user session id
 *
 * @param session   a pointer of session structure
 *
 * @return  a pointer of session identifier
 *
 * @note Do not free manually
 */
const char *qcgisess_getid(qentry_t *session)
{
    return session->getstr(session, INTER_SESSIONID, false);
}

/**
 * Get user session created time
 *
 * @param session   a pointer of session structure
 *
 * @return  user session created time in UTC time seconds
 */
time_t qcgisess_getcreated(qentry_t *session)
{
    const char *created = session->getstr(session, INTER_CREATED_SEC, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    return (time_t)atol(created);
}

/**
 * Update session data
 *
 * @param session   a pointer of session structure
 *
 * @return  true if successful, otherwise returns false
 */
bool qcgisess_save(qentry_t *session)
{
    const char *sessionkey = session->getstr(session, INTER_SESSIONID, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    const char *session_repository_path = session->getstr(session, INTER_SESSION_REPO, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    int session_timeout_interval = session->getint(session, INTER_INTERVAL_SEC);
    if (sessionkey == NULL || session_repository_path == NULL) return false;

    char session_storage_path[PATH_MAX];
    char session_timeout_path[PATH_MAX];
    snprintf(session_storage_path, sizeof(session_storage_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_STORAGE_EXTENSION);
    snprintf(session_timeout_path, sizeof(session_timeout_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_TIMEOUT_EXTENSION);

    if (session->save(session, session_storage_path) == false) {
        DEBUG_CODER("Can't save session file %s", session_storage_path);
        return false;
    }
    if (_update_timeout(session_timeout_path, session_timeout_interval) == false) {
        DEBUG_CODER("Can't update file %s", session_timeout_path);
        return false;
    }

    _clear_repo(session_repository_path);
    return true;
}

/**
 * Destroy user session
 *
 * @param session   a pointer of session structure
 *
 * @return  true if successful, otherwise returns false
 *
 * @note
 * If you only want to de-allocate session structure, just call qentry_t->free().
 * This will remove all user session data permanantely and also free the session structure.
 */
bool qcgisess_destroy(qentry_t *session)
{
    const char *sessionkey, *session_repository_path;
    char session_storage_path[PATH_MAX], session_timeout_path[PATH_MAX], session_KVM_path[PATH_MAX];

	if(!session)
		return false;
	
    sessionkey = session->getstr(session, INTER_SESSIONID, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    session_repository_path = session->getstr(session, INTER_SESSION_REPO, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */

    if (sessionkey == NULL || session_repository_path == NULL) {
        if (session != NULL) session->free(session);
        return false;
    }

    snprintf(session_storage_path, sizeof(session_storage_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_STORAGE_EXTENSION);
    snprintf(session_timeout_path, sizeof(session_timeout_path),
             "%s/%s%s%s",
             session_repository_path,
             SESSION_PREFIX, sessionkey, SESSION_TIMEOUT_EXTENSION);

    snprintf(session_KVM_path, sizeof(session_KVM_path),
             "%s/%s%s",
			 session_repository_path,
			 sessionkey, SESSION_KVM_EXTENSION);

    _q_unlink(session_storage_path);
    _q_unlink(session_timeout_path);
    _q_unlink(session_KVM_path);

    if (session != NULL) session->free(session);
    return true;
}

#ifndef _DOXYGEN_SKIP

static bool _clear_repo(const char *session_repository_path)
{
#ifdef _WIN32
    return false;
#else
    // clear old session data
    DIR *dp;
    qentry_t *session;
    if ((dp = opendir(session_repository_path)) == NULL) {	/* Fortify [Path Manipulation]:: False Positive *//* Reason for False Positive - not accepting user inputs and not using invalid data while using the file */
        DEBUG_CODER("Can't open session repository %s", session_repository_path);
        return false;
    }

    struct dirent *dirp;
    while ((dirp = readdir(dp)) != NULL) {
        if (strstr(dirp->d_name, SESSION_PREFIX) &&
            strstr(dirp->d_name, SESSION_TIMEOUT_EXTENSION)) {
            char timeoutpath[PATH_MAX] = {'\0'};
            snprintf(timeoutpath, sizeof(timeoutpath),
                     "%s/%s", session_repository_path, dirp->d_name);
            if (_is_valid_session(timeoutpath) <= 0) { // expired
                // remove timeout
                _q_unlink(timeoutpath);/* Fortify [Path Manipulation]:: False Positive */  /* Fortify [Race Condition: File System Access]:: False Positive *//* Reason for Race Condition: File System Access False Positve - The file does not change between the calls of stat() and _q_unlink() */

                // remove properties
                timeoutpath[strlen(timeoutpath) - strlen(SESSION_TIMEOUT_EXTENSION)] = '\0';
                /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
				/**
				 * Reason for False Positive: 
				 * Avoid buffer overflow once the total size of two strings without "the terminating null byte" equal or large the size of dest string.
				 */
                if( strlen(timeoutpath) + strlen(SESSION_STORAGE_EXTENSION) >= sizeof(timeoutpath)){ /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
                    DEBUG_CODER("Buffer Overflow");
                    return false;
                } else {
                    /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
					/**
					 * Reason for False Positive: 
					 * "strcat()" did not apend the terminating null byte to dest string.
					 * Using "strncat()" instead of it.
					 */
                    strncat(timeoutpath, SESSION_STORAGE_EXTENSION, PATH_MAX - strlen(timeoutpath) - 1); /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
                }
                session = qEntry();
                session->load(session, timeoutpath);
                uint32 racsession_id = session->getint(session, "racsession_id");
                racsessinfo_unregister_session(racsession_id, SESSION_UNREGISTER_REASON_LOGOUT);
				if (session != NULL) session->free(session);
                _q_unlink(timeoutpath);/* Fortify [Path Manipulation]:: False Positive */  /* Fortify [Race Condition: File System Access]:: False Positive *//* Reason for Race Condition: File System Access False Positve - The file does not change between the calls of stat() and _q_unlink() */

            }
        }
    }
    closedir(dp);

    return true;
#endif
}

bool clear_repo(const char *session_repository_path)
{
    if(session_repository_path)
    {
       _clear_repo(session_repository_path);
       return true ;
    }
    return false ;
}

static bool _clear_session(const char *session_repository_path, const char *sessionkey)
{
    // clear specific session data
    DIR *dp;
    qentry_t *session;
    if ((dp = opendir(session_repository_path)) == NULL) {	/* Fortify [Path Manipulation]:: False Positive *//* Reason for False Positive - not accepting user inputs and not using invalid data while using the file */
        DEBUG_CODER("Can't open session repository %s", session_repository_path);
        return false;
    }

    struct dirent *dirp;
    while ((dirp = readdir(dp)) != NULL) {

        if (strstr(dirp->d_name, SESSION_PREFIX) &&
            strstr(dirp->d_name, SESSION_TIMEOUT_EXTENSION)) {

            char timeoutpath[PATH_MAX] = {'\0'};
            snprintf(timeoutpath, sizeof(timeoutpath),
                     "%s/%s", session_repository_path, dirp->d_name);

	    char session_KVM_path[PATH_MAX];
	    snprintf(session_KVM_path, sizeof(session_KVM_path),
		     "%s/%s%s", session_repository_path, sessionkey, SESSION_KVM_EXTENSION);

            if (strstr(dirp->d_name, sessionkey)) { // search specific session

                // remove timeout
                _q_unlink(timeoutpath);
		
		// remove kvm
		_q_unlink(session_KVM_path);

                // remove properties
                timeoutpath[strlen(timeoutpath) - strlen(SESSION_TIMEOUT_EXTENSION)] = '\0';
                /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
				/**
				 * Reason for False Positive: 
				 * Avoid buffer overflow once the total size of two strings without "the terminating null byte" equal or large the size of dest string.
				 */
                if( strlen(timeoutpath) + strlen(SESSION_STORAGE_EXTENSION) >= sizeof(timeoutpath)){ /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
                    DEBUG_CODER("Buffer Overflow");
                    return false;
                } else {
                    /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
					/**
					 * Reason for False Positive: 
					 * "strcat()" did not apend the terminating null byte to dest string.
					 * Using "strncat()" instead of it.
					 */
                    strncat(timeoutpath, SESSION_STORAGE_EXTENSION, PATH_MAX - strlen(timeoutpath) - 1); /* Fortify [Buffer Overflow: Off-by-One]:: False Positive */
                }
                session = qEntry();
                session->load(session, timeoutpath);
                uint32 racsession_id = session->getint(session, "racsession_id");
                racsessinfo_unregister_session(racsession_id, SESSION_UNREGISTER_REASON_LOGOUT);
                if (session != NULL) session->free(session);
                _q_unlink(timeoutpath);/* Fortify [Path Manipulation]:: False Positive */  /* Fortify [Race Condition: File System Access]:: False Positive *//* Reason for Race Condition: File System Access False Positve - The file does not change between the calls of stat() and _q_unlink() */

            }
        }
    }
    closedir(dp);

    return true;
}

bool clear_session(const char *session_repository_path, const char *sessionkey)
{
    if(session_repository_path)
    {
       _clear_session(session_repository_path, sessionkey);
       return true ;
    }
    return false ;
}

/**
 * clear specific user session
 *
 * @param session   a pointer of session structure
 *
 * @return  true if successful, otherwise returns false
 */
bool qcgisess_clear(qentry_t *session)
{
    const char *sessionkey = session->getstr(session, INTER_SESSIONID, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    const char *session_repository_path = session->getstr(session, INTER_SESSION_REPO, false);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    if (sessionkey == NULL || session_repository_path == NULL) return false;

    char session_storage_path[PATH_MAX];
    snprintf(session_storage_path, sizeof(session_storage_path),
            "%s/%s%s%s",
            session_repository_path,
            SESSION_PREFIX, sessionkey, SESSION_STORAGE_EXTENSION);

    if (session->save(session, session_storage_path) == false) {
        DEBUG_CODER("Can't save session file %s", session_storage_path);
        return false;
    }   

    clear_session(session_repository_path, sessionkey);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive Session memory cleared properly in end_handler. */
    return true;
}

char * genuniqid(void)
{
    return _genuniqid() ;
}

int is_valid_session(const char *filepath)
{
	return _is_valid_session(filepath);
}

/** @brief parse URI by spiliting query string parameters and copy its value to buffer.
 * param[in] uri should be null terminated.
 * @returns 0 in case of success
 *         -1 in case of an error
 */

int get_auth_str(char *uri, int uri_size, char *csrftoken)
{
    char *start;
    int tempret = 0;
    char uri_tmp[MAX_LINEBUF];

    memset(uri_tmp, 0, MAX_LINEBUF);
    tempret = snprintf(uri_tmp, uri_size, "%s", uri);/* False Positive [Buffer Overflow: Format String] *//* format string argument and format specifier for the destination are same and condition to check buffer overflow/ underflow is present - Reason */
    if(tempret < 0 || tempret >= MAX_LINEBUF)
        return -1;

    if((start = strstr(uri_tmp, "?")) != NULL)
        start++;
    else // no query string parameters found
        return -1;

    char *query = start, *p;
    char **tokens = &query;

    // Assume value contains no as same special character as below delimiter
    while ((p = strsep (tokens, "&"))) {
        char *val=p, *var;
        if ((var = strsep(&val, "=")))
        {
            if(strncasecmp("CSRFTOKEN", var, 9) == 0)
            {
                tempret = snprintf(csrftoken, MAX_LINEBUF, "%s", val);
                if(tempret < 0 || tempret >= MAX_LINEBUF)
                    return -1;
            }
        }
    }
    return 0;
}

/** @brief Verfied if csrftoken is valid from a web session
 * @returns 0 in case of authentication failure
 *          1 in case of success
 */

int is_valid_authorization(char *qsession, char *csrftoken)
{
    if(qsession == NULL || csrftoken == NULL)
        return 0;

    char session_storage_path[PATH_MAX];
    char line[MAX_LINEBUF] = {0};
    int tempret = 0;

    tempret = snprintf(session_storage_path, sizeof(session_storage_path),
            "%s/%s%s%s",
            SESSION_DEFAULT_REPOSITORY,
            SESSION_PREFIX,
            qsession,
            SESSION_STORAGE_EXTENSION);
    if(tempret < 0 || tempret >= (long)sizeof(session_storage_path))
        return 0;

    FILE *fp = fopen(session_storage_path, "r");
    if (fp == NULL) return 0;

    while((fgets(line, MAX_LINEBUF, fp)) != NULL) 
    {
        if((strstr(line, "CSRFTOKEN")) != NULL)
        {
            char *val=&line[0], *var;
            if ((var = strsep(&val, "=")))
            {
                _q_strtrim(val);     
                _q_urldecode(val);
                if(strcmp(val, csrftoken) == 0) // valid session
				{
					fclose(fp);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive  closing the file properly in both failure and success cases. */
                    return 1;
				}
            }
        }
    }    

    fclose(fp);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive  closing the file properly in both failure and success cases. */

    // CSRFTOKEN not found
    return 0;
}

/** @brief Verfied if qsession is valid from a web session
 * @returns 0 in case of authentication failure
 *          1 in case of success
 */

int is_valid_Qsession(char *qsession)
{
    if(qsession == NULL)
        return 0;

    char session_storage_path[PATH_MAX];
    char line[MAX_LINEBUF] = {0};
    int tempret = 0;
    
    char *delim = ";=";
    char *dupstr = strdup(qsession);
    char *sepstr = dupstr;
    char *substr;

    do
    {
        substr = strsep(&sepstr, delim);
        if (strstr(substr, "QSESSIONID") != NULL) {
            substr = strsep(&sepstr, delim);
            break;
        }
    } while (substr);

    tempret = snprintf(session_storage_path, sizeof(session_storage_path),
            "%s/%s%s%s",
            SESSION_DEFAULT_REPOSITORY,
            SESSION_PREFIX,
            substr,
            SESSION_STORAGE_EXTENSION);
    if(tempret < 0 || tempret >= (long)sizeof(session_storage_path)) {
    	free(dupstr);
        return 0;
    }

    FILE *fp = fopen(session_storage_path, "r");
    if (fp == NULL) {free(dupstr); return 0;}

    while((fgets(line, MAX_LINEBUF, fp)) != NULL) 
    {
        if((strstr(line, "_Q_SESSIONID=")) != NULL)
        {
            char *val=&line[0], *var;
            if ((var = strsep(&val, "=")))
            {
                _q_strtrim(val);     
                _q_urldecode(val);
                if(strcmp(val, substr) == 0) // valid session
				{
					free(dupstr);
					fclose(fp);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive  closing the file properly in both failure and success cases. */
                    return 1;
				}
            }
        }
    }    

	free(dupstr);
    fclose(fp);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive  closing the file properly in both failure and success cases. */

    // Qsession not found
    return 0;
}

// session not found 0, session expired -1, session valid 1
static int _is_valid_session(const char *filepath)
{
    time_t timeout, timenow;
    double timediff;

    if ((timeout = (time_t)_q_countread(filepath)) == 0) return 0;

    timenow = time(NULL);
    timediff = difftime(timeout, timenow); // return timeout - timenow

    if (timediff >= 0) return 1; // valid
    return -1; // expired
}

// success > 0, write fail 0
static bool _update_timeout(const char *filepath, time_t timeout_interval)
{
    if (timeout_interval <= 0) return false;

    if (_q_countsave(filepath, (time(NULL) + timeout_interval)) == false) {
        return false;
    }
    return true;
}

static char *_genuniqid(void)
{
#ifdef _WIN32
    unsigned int sec = time(NULL);
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned int usec = ft.dwLowDateTime % 1000000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned int sec =(unsigned int) tv.tv_sec;
    unsigned int usec =(unsigned int) tv.tv_usec;
#endif
    unsigned int port = 0;
    const char *remote_port = getenv("REMOTE_PORT");

	char TempRandomString[COOKIE_RANDOM_PORTION_LEN];
	char tmpchar;

	memset(TempRandomString, 0, COOKIE_RANDOM_PORTION_LEN);

    if (remote_port != NULL) {
        port = atoi(remote_port);
    }
	//TODO: Make cookie stronger - DONE

	FILE* rd;
	int i;

	rd = fopen("/dev/random", "r");

	if(rd != NULL) {
		for(i=0; i<COOKIE_RANDOM_PORTION_LEN; i++) {
			while(1) {
				tmpchar = fgetc(rd);
				if(isalnum(tmpchar)) break;
			}
			TempRandomString[i] = tmpchar;
		}
		
		fclose(rd);/* Fortify [Memory Leak]:: False Positive *//* Reason for False Positive  closing the file properly in both failure and success cases. */
	}

    char *uniqid = (char *)malloc(5+5+4+4+COOKIE_RANDOM_PORTION_LEN+1);
    /* Fortify [String Termination Error]:: False Positive */
    /* Reason for False Positive - once buffer overlow happened, we will make last char as null */
    if (snprintf(uniqid, 5+5+4+4+COOKIE_RANDOM_PORTION_LEN+1, "%05x%05x%04x%04x%12s",
                 usec%0x100000, sec%0x100000, getpid()%0x10000, port%0x10000, TempRandomString)
        >= 5+5+4+4+COOKIE_RANDOM_PORTION_LEN+1) uniqid[5+5+4+4+COOKIE_RANDOM_PORTION_LEN] = '\0';;
	return uniqid;
	
}

#endif /* _DOXYGEN_SKIP */