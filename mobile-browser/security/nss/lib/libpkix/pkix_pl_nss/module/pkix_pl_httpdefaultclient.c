/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the PKIX-C library.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are
 * Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems, Inc.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * pkix_pl_httpdefaultclient.c
 *
 * HTTPDefaultClient Function Definitions
 *
 */

#include "pkix_pl_httpdefaultclient.h"

static void *plContext = NULL;

/*
 * The interface specification for an http client requires that it register
 * a function table of type SEC_HttpClientFcn, which is defined as a union
 * of tables, of which only version 1 is defined at present.
 *
 * Note: these functions violate the PKIX calling conventions, in that they
 * return SECStatus rather than PKIX_Error*, and that they do not provide a
 * plContext argument. They are implemented here as calls to PKIX functions,
 * but the plContext value is circularly defined - a true kludge. Its value
 * is saved at the time of the call to pkix_pl_HttpDefaultClient_Create for
 * subsequent use, but since that initial call comes from the
 * pkix_pl_HttpDefaultClient_CreateSessionFcn, it's not really getting saved.
 */
static SEC_HttpClientFcnV1 vtable = {
        pkix_pl_HttpDefaultClient_CreateSessionFcn,
        pkix_pl_HttpDefaultClient_KeepAliveSessionFcn,
        pkix_pl_HttpDefaultClient_FreeSessionFcn,
        pkix_pl_HttpDefaultClient_RequestCreateFcn,
        pkix_pl_HttpDefaultClient_SetPostDataFcn,
        pkix_pl_HttpDefaultClient_AddHeaderFcn,
        pkix_pl_HttpDefaultClient_TrySendAndReceiveFcn,
        pkix_pl_HttpDefaultClient_CancelFcn,
        pkix_pl_HttpDefaultClient_FreeFcn
};

static SEC_HttpClientFcn httpClient;

static const char *eohMarker = "\r\n\r\n";
static const PKIX_UInt32 eohMarkLen = 4; /* strlen(eohMarker) */
static const char *crlf = "\r\n";
static const PKIX_UInt32 crlfLen = 2; /* strlen(crlf) */
static const char *httpprotocol = "HTTP/";
static const PKIX_UInt32 httpprotocolLen = 5; /* strlen(httpprotocol) */

/* --Private-HttpDefaultClient-Functions------------------------- */

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_HdrCheckComplete
 * DESCRIPTION:
 *
 *  This function determines whether the headers in the current receive buffer
 *  in the HttpDefaultClient pointed to by "client" are complete. If so, the
 *  input data is checked for status code, content-type and content-length are
 *  extracted, and the client is set up to read the body of the response.
 *  Otherwise, the client is set up to continue reading header data.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "bytesRead"
 *      The UInt32 number of bytes received in the latest read.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_HdrCheckComplete(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_UInt32 bytesRead,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_UInt32 alreadyScanned = 0;
        PKIX_UInt32 searchOffset = 0;
        PKIX_UInt32 comp = 0;
        PKIX_UInt32 headerLength = 0;
        PKIX_UInt32 contentLength = 0;
        char *eoh = NULL;
        char *statusLineEnd = NULL;
        char *space = NULL;
        char *nextHeader = NULL;
        const char *httpcode = NULL;
        char *thisHeaderEnd = NULL;
        char *value = NULL;
        char *colon = NULL;
	char *copy = NULL;
        char *body = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_HdrCheckComplete");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        *pKeepGoing = PKIX_FALSE;

        /* Does buffer contain end-of-header marker? */
        alreadyScanned = client->alreadyScanned;
        /*
         * If this is the initial buffer, we have to scan from the beginning.
         * If we scanned, failed to find eohMarker, and read some more, we
         * only have to scan from where we left off.
         */
        if (alreadyScanned > eohMarkLen) {
                searchOffset = alreadyScanned - eohMarkLen;
                PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, eoh, PL_strnstr,
                        (&(client->rcvBuf[searchOffset]),
                        eohMarker,
                        client->capacity - searchOffset));
        } else {
                PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, eoh, PL_strnstr,
                        (client->rcvBuf, eohMarker, bytesRead));
        }

        alreadyScanned += bytesRead;
        client->alreadyScanned = alreadyScanned;

        if (eoh == NULL) { /* did we see end-of-header? */

                /* No. Continue to read header data */
                client->connectStatus = HTTP_RECV_HDR;
                *pKeepGoing = PKIX_TRUE;
                goto cleanup;

        }

        /* Yes. Calculate how many bytes in header (not counting eohMarker) */
        headerLength = (eoh - client->rcvBuf);

        /* allocate space to copy header (and for the NULL terminator) */
        PKIX_CHECK(PKIX_PL_Malloc(headerLength + 1, (void **)&copy, plContext),
                PKIX_MALLOCFAILED);

        /* copy header data before we corrupt it (by storing NULLs) */
        PKIX_CHECK(PKIX_PL_Memcpy
                (client->rcvBuf, headerLength, (void **)&copy, plContext),
                PKIX_MEMCPYFAILED);

	/* Store the NULL terminator */
	copy[headerLength] = '\0';

	client->rcvHeaders = copy;

        /* Did caller want a pointer to header? */
        if (client->rcv_http_headers != NULL) {

                /* store pointer for caller */
                *(client->rcv_http_headers) = copy;
        }

        /* Check that message status is okay. */

        PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, statusLineEnd, PL_strnstr,
                (client->rcvBuf, crlf, client->capacity));

        if (statusLineEnd == NULL) {
                client->connectStatus = HTTP_ERROR;
                PKIX_PL_NSSCALL(HTTPDEFAULTCLIENT, PORT_SetError,
                        (SEC_ERROR_OCSP_BAD_HTTP_RESPONSE));
                goto cleanup;
        }

        *statusLineEnd = '\0';

        PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, space, strchr,
                ((const char *)client->rcvBuf, ' '));
        
        if (space == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, comp, PORT_Strncasecmp,
                ((const char *)client->rcvBuf,
                httpprotocol,
                httpprotocolLen));
        
        if (comp != 0) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        httpcode = space + 1;

        PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, space, strchr,
                (httpcode, ' '));

        if (space == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        *space = '\0';

        PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, client->responseCode, atoi,
                (httpcode));

        if (client->responseCode != 200) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }

        /* Find the content-type and content-length */
        nextHeader = statusLineEnd + crlfLen;
        *eoh = '\0';
        do {
                thisHeaderEnd = NULL;
                value = NULL;
                PKIX_PL_NSSCALLRV(HTTPDEFAULTCLIENT, colon, strchr,
                        (nextHeader, ':'));

                if (colon == NULL) {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }

                *colon = '\0';
                value = colon + 1;
                if (*value != ' ') {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }
                value++;
                PKIX_PL_NSSCALLRV
                        (HTTPDEFAULTCLIENT, thisHeaderEnd, strstr,
                        (value, crlf));
                if (thisHeaderEnd != NULL) {
                        *thisHeaderEnd = '\0';
                }
                PKIX_PL_NSSCALLRV
                        (HTTPDEFAULTCLIENT, comp, PORT_Strcasecmp,
                        (nextHeader, "content-type"));
                if (comp == 0) {
                        client->rcvContentType = value;
                } else {
                        PKIX_PL_NSSCALLRV
                                (HTTPDEFAULTCLIENT,
                                comp,
                                PORT_Strcasecmp,
                                (nextHeader, "content-length"));
                        if (comp == 0) {
                                contentLength = atoi(value);
                        }
                }
                if (thisHeaderEnd != NULL) {
                        nextHeader = thisHeaderEnd + crlfLen;
                } else {
                        nextHeader = NULL;
                }
        } while ((nextHeader != NULL) && (nextHeader < (eoh + crlfLen)));
                
        /* Did caller provide a pointer to return content-type? */
        if (client->rcv_http_content_type != NULL) {
                *(client->rcv_http_content_type) = client->rcvContentType;
        }

        if (client->rcvContentType == NULL) {
                client->connectStatus = HTTP_ERROR;
                goto cleanup;
        }
#if 0
/* XXX move this code into ocsp checker */
 else {
                PKIX_PL_NSSCALLRV
                        (HTTPDEFAULTCLIENT,
                        comp,
                        PORT_Strcasecmp,
                        (client->rcvContentType, "application/ocsp-response"));
                if (comp != 0) {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }
        }
#endif

        /*
         * The headers have passed validation. Now figure out whether the
         * message is within the caller's size limit (if one was specified).
         */

        client->rcv_http_data_len = contentLength;
        if (client->maxResponseLen > 0) {
                if (client->maxResponseLen < contentLength) {
                        client->connectStatus = HTTP_ERROR;
                        goto cleanup;
                }
        }

        /* allocate a buffer of size contentLength  for the content */
        PKIX_CHECK(PKIX_PL_Malloc(contentLength, (void **)&body, plContext),
                PKIX_MALLOCFAILED);

        /* How many bytes remain in current buffer, beyond the header? */
        headerLength += eohMarkLen;
        client->currentBytesAvailable -= headerLength;

        /* copy any remaining bytes in current buffer into new buffer */
        if (client->currentBytesAvailable > 0) {
                PKIX_CHECK(PKIX_PL_Memcpy
                        (&(client->rcvBuf[headerLength]),
                        client->currentBytesAvailable,
                        (void **)&body,
                        plContext),
                        PKIX_MEMCPYFAILED);
        }

        PKIX_CHECK(PKIX_PL_Free(client->rcvBuf, plContext),
                PKIX_FREEFAILED);
        client->rcvBuf = body;

        /*
         * Do we have all of the message body, or do we need to read some more?
         */

        if (client->currentBytesAvailable < contentLength) {
                client->bytesToRead =
                        contentLength - client->currentBytesAvailable;
		client->alreadyScanned = 0;
                client->connectStatus = HTTP_RECV_BODY;
        } else {
                client->connectStatus = HTTP_COMPLETE;
                *pKeepGoing = PKIX_FALSE;
                goto cleanup;
        }

        *pKeepGoing = PKIX_TRUE;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: PKIX_PL_HttpDefaultClient_Create
 * DESCRIPTION:
 *
 *  This function creates a new HttpDefaultClient, and stores the result at
 *  "pClient".
 *
 *  The HttpClient API does not include a plContext argument in its
 *  function calls. Its value at the time of this Create call must be the
 *  same as when the client is invoked.
 *
 * PARAMETERS:
 *  "host"
 *      The name of the server with which we hope to exchange messages. Must
 *      be non-NULL.
 *  "portnum"
 *      The port number to be used for our connection to the server.
 *  "pClient"
 *      The address at which the created HttpDefaultClient is to be stored.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in
 *      a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_Create(
        const char *host,
        PRUint16 portnum,
        PKIX_PL_HttpDefaultClient **pClient,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "PKIX_PL_HttpDefaultClient_Create");
        PKIX_NULLCHECK_ONE(pClient);

        /* allocate an HttpDefaultClient */
        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_HTTPDEFAULTCLIENT_TYPE,
                sizeof (PKIX_PL_HttpDefaultClient),
                (PKIX_PL_Object **)&client,
                plContext),
                PKIX_COULDNOTCREATEHTTPDEFAULTCLIENTOBJECT);

        client->connectStatus = HTTP_NOT_CONNECTED;
        client->portnum = portnum;
        client->timeout = PR_INTERVAL_NO_WAIT; /* PR_INTERVAL_NO_TIMEOUT; */
        client->bytesToWrite = 0;
        client->bytesToRead = 0;
        client->send_http_data_len = 0;
        client->rcv_http_data_len = 0;
        client->capacity = 0;
        client->alreadyScanned = 0;
        client->currentBytesAvailable = 0;
        client->responseCode = 0;
        client->maxResponseLen = 0;
        client->GETLen = 0;
        client->POSTLen = 0;
        client->pRcv_http_data_len = NULL;
        client->callbackList = NULL;
        client->GETBuf = NULL;
        client->POSTBuf = NULL;
        client->rcvBuf = NULL;
        client->host = host;
        client->path = NULL;
        client->rcvContentType = NULL;
        client->rcvHeaders = NULL;
        client->send_http_method = HTTP_POST_METHOD;
        client->send_http_content_type = NULL;
        client->send_http_data = NULL;
        client->rcv_http_response_code = NULL;
        client->rcv_http_content_type = NULL;
        client->rcv_http_headers = NULL;
        client->rcv_http_data = NULL;
        client->socket = NULL;

        /*
         * The HttpClient API does not include a plContext argument in its
         * function calls. Save it here.
         */
        client->plContext = plContext;

        *pClient = client;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(client);
        }

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_HTTPDEFAULTCLIENT_TYPE, plContext),
                    PKIX_OBJECTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)object;

        PKIX_PL_Free(client->rcvHeaders, plContext);
        client->rcvHeaders = NULL;

        if (client->GETBuf != NULL) {
                PKIX_PL_NSSCALL
                        (HTTPDEFAULTCLIENT, PR_smprintf_free, (client->GETBuf));
                client->GETBuf = NULL;
        }

        if (client->POSTBuf != NULL) {
                PKIX_PL_Free(client->POSTBuf, plContext);
                client->POSTBuf = NULL;
        }

        if (client->rcvBuf != NULL) {
                PKIX_PL_Free(client->rcvBuf, plContext);
                client->rcvBuf = NULL;
        }

        PKIX_DECREF(client->socket);

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_RegisterSelf
 *
 * DESCRIPTION:
 *  Registers PKIX_PL_HTTPDEFAULTCLIENT_TYPE and its related
 *  functions with systemClasses[]
 *
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_pl_HttpDefaultClient_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_RegisterSelf");

        entry.description = "HttpDefaultClient";
        entry.destructor = pkix_pl_HttpDefaultClient_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_HTTPDEFAULTCLIENT_TYPE] = entry;

        httpClient.version = 1;
        httpClient.fcnTable.ftable1 = vtable;
        (void)SEC_RegisterDefaultHttpClient(&httpClient);

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/* --Private-HttpDefaultClient-I/O-Functions---------------------------- */
/*
 * FUNCTION: pkix_pl_HttpDefaultClient_ConnectContinue
 * DESCRIPTION:
 *
 *  This function determines whether a socket Connect initiated earlier for the
 *  HttpDefaultClient "client" has completed, and stores in "pKeepGoing" a flag
 *  indicating whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_ConnectContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PRErrorCode status;
        PKIX_Boolean keepGoing = PKIX_FALSE;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_ConnectContinue");
        PKIX_NULLCHECK_ONE(client);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->connectcontinueCallback
                (client->socket, &status, plContext),
                PKIX_SOCKETCONNECTCONTINUEFAILED);

        if (status == 0) {
                client->connectStatus = HTTP_CONNECTED;
                keepGoing = PKIX_TRUE;
        } else if (status != PR_IN_PROGRESS_ERROR) {
                PKIX_ERROR(PKIX_UNEXPECTEDERRORINESTABLISHINGCONNECTION);
        }

        *pKeepGoing = keepGoing;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_Send
 * DESCRIPTION:
 *
 *  This function creates and sends HTTP-protocol headers and, if applicable,
 *  data, for the HttpDefaultClient "client", and stores in "pKeepGoing" a flag
 *  indicating whether processing can continue without further input, and at
 *  "pBytesTransferred" the number of bytes sent.
 *
 *  If "pBytesTransferred" is zero, it indicates that non-blocking I/O is in use
 *  and that transmission has not completed.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "pBytesTransferred"
 *      The address at which the number of bytes sent is stored. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_Send(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_Int32 lenToWrite = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        char *dataToWrite = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Send");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        /* Do we have anything waiting to go? */
        if ((client->GETBuf) || (client->POSTBuf)) {

                if (client->GETBuf) {
                        dataToWrite = client->GETBuf;
                        lenToWrite = client->GETLen;
                } else {
                        dataToWrite = client->POSTBuf;
                        lenToWrite = client->POSTLen;
                }

                callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

                PKIX_CHECK(callbackList->sendCallback
                        (client->socket,
                        dataToWrite,
                        lenToWrite,
                        &bytesWritten,
                        plContext),
                        PKIX_SOCKETSENDFAILED);

                client->rcvBuf = NULL;
                client->capacity = 0;
                client->alreadyScanned = 0;
                client->currentBytesAvailable = 0;

                /*
                 * If the send completed we can proceed to try for the
                 * response. If the send did not complete we will have
                 * to poll for completion later.
                 */
                if (bytesWritten >= 0) {
                        client->connectStatus = HTTP_RECV_HDR;
                        *pKeepGoing = PKIX_TRUE;
                } else {
                        client->connectStatus = HTTP_SEND_PENDING;
                        *pKeepGoing = PKIX_FALSE;
                }

        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_SendContinue
 * DESCRIPTION:
 *
 *  This function determines whether the sending of the HTTP message for the
 *  HttpDefaultClient "client" has completed, and stores in "pKeepGoing" a
 *  flag indicating whether processing can continue without further input, and
 *  at "pBytesTransferred" the number of bytes sent.
 *
 *  If "pBytesTransferred" is zero, it indicates that non-blocking I/O is in use
 *  and that transmission has not completed.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "pBytesTransferred"
 *      The address at which the number of bytes sent is stored. Must be
 *      non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_SendContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        PKIX_UInt32 *pBytesTransferred,
        void *plContext)
{
        PKIX_Int32 bytesWritten = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_SendContinue");
        PKIX_NULLCHECK_THREE(client, pKeepGoing, pBytesTransferred);

        *pKeepGoing = PKIX_FALSE;

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->pollCallback
                (client->socket, &bytesWritten, NULL, plContext),
                PKIX_SOCKETPOLLFAILED);

        /*
         * If the send completed we can proceed to try for the
         * response. If the send did not complete we will have
         * continue to poll.
         */
        if (bytesWritten >= 0) {
                       client->connectStatus = HTTP_RECV_HDR;
                *pKeepGoing = PKIX_TRUE;
        }

        *pBytesTransferred = bytesWritten;

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_RecvHdr
 * DESCRIPTION:
 *
 *  This function receives HTTP headers for the HttpDefaultClient "client", and
 *  stores in "pKeepGoing" a flag indicating whether processing can continue
 *  without further input.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvHdr(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_UInt32 bytesToRead = 0;
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RecvHdr");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        /*
         * rcvbuf, capacity, alreadyScanned, and currentBytesAvailable were
         * initialized when we wrote the headers. We begin by reading
         * HTTP_OCSP_BUFSIZE bytes, repeatedly increasing the buffersize and
         * reading again if necessary, until we have read the end-of-header
         * marker, "\r\n\r\n", or have reached our maximum.
         */
        client->capacity += HTTP_OCSP_BUFSIZE;
        PKIX_CHECK(PKIX_PL_Realloc
                (client->rcvBuf,
                client->capacity,
                (void **)&(client->rcvBuf),
                plContext),
                PKIX_REALLOCFAILED);

        bytesToRead = client->capacity - client->alreadyScanned;

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->recvCallback
                (client->socket,
                (void *)&(client->rcvBuf[client->alreadyScanned]),
                bytesToRead,
                &bytesRead,
                plContext),
                PKIX_SOCKETRECVFAILED);

        if (bytesRead > 0) {

                client->currentBytesAvailable += bytesRead;

                PKIX_CHECK(pkix_pl_HttpDefaultClient_HdrCheckComplete
                        (client, bytesRead, pKeepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTHDRCHECKCOMPLETEFAILED);

        } else {

                client->connectStatus = HTTP_RECV_HDR_PENDING;
                *pKeepGoing = PKIX_FALSE;

        }

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_RecvHdrContinue
 * DESCRIPTION:
 *
 *  This function determines whether the receiving of the HTTP headers for the
 *  HttpDefaultClient "client" has completed, and stores in "pKeepGoing" a flag
 *  indicating whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvHdrContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_RecvHdrContinue");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->pollCallback
                (client->socket, NULL, &bytesRead, plContext),
                PKIX_SOCKETPOLLFAILED);

        if (bytesRead > 0) {
                client->currentBytesAvailable += bytesRead;

                PKIX_CHECK(pkix_pl_HttpDefaultClient_HdrCheckComplete
                        (client, bytesRead, pKeepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTHDRCHECKCOMPLETEFAILED);

        } else {

                *pKeepGoing = PKIX_FALSE;

        }

cleanup:
        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_RecvBody
 * DESCRIPTION:
 *
 *  This function processes the contents of the first buffer of a received
 *  HTTP-protocol message for the HttpDefaultClient "client", and stores in
 *  "pKeepGoing" a flag indicating whether processing can continue without
 *  further input.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvBody(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;

        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RecvBody");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->recvCallback
                (client->socket,
                (void *)&(client->rcvBuf[client->currentBytesAvailable]),
                client->bytesToRead,
                &bytesRead,
                plContext),
                PKIX_SOCKETRECVFAILED);

        if (bytesRead > 0) {

		/* We got something. Did we get it all? */
                client->currentBytesAvailable += bytesRead;

		if (client->bytesToRead > (PKIX_UInt32)bytesRead) {
                	client->bytesToRead -= (PKIX_UInt32)bytesRead;
			*pKeepGoing = PKIX_TRUE;
		} else {
	                client->connectStatus = HTTP_COMPLETE;
	                *pKeepGoing = PKIX_FALSE;
		}

        } else {

                client->connectStatus = HTTP_RECV_BODY_PENDING;
                *pKeepGoing = PKIX_TRUE;
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_RecvBodyContinue
 * DESCRIPTION:
 *
 *  This function checks for completion of a read of a response HTTP-protocol
 *  message for the HttpDefaultClient "client", and stores in "pKeepGoing" a
 *  flag indicating whether processing can continue without further input.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "pKeepGoing"
 *      The address at which the Boolean state machine flag is stored to
 *      indicate whether processing can continue without further input.
 *      Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_RecvBodyContinue(
        PKIX_PL_HttpDefaultClient *client,
        PKIX_Boolean *pKeepGoing,
        void *plContext)
{
        PKIX_Int32 bytesRead = 0;
        PKIX_PL_Socket_Callback *callbackList = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_RecvBodyContinue");
        PKIX_NULLCHECK_TWO(client, pKeepGoing);

        callbackList = (PKIX_PL_Socket_Callback *)client->callbackList;

        PKIX_CHECK(callbackList->pollCallback
                (client->socket, NULL, &bytesRead, plContext),
                PKIX_SOCKETPOLLFAILED);


        if (bytesRead > 0) {

		/* We got something. Did we get it all? */
                client->currentBytesAvailable += bytesRead;
                client->bytesToRead -= bytesRead;

		if (client->bytesToRead > 0) {
                	client->connectStatus = HTTP_RECV_BODY;
			*pKeepGoing = PKIX_TRUE;
		} else {
	                client->connectStatus = HTTP_COMPLETE;
	                *pKeepGoing = PKIX_FALSE;
		}
        }

        *pKeepGoing = PKIX_FALSE;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * FUNCTION: pkix_pl_HttpDefaultClient_Dispatch
 * DESCRIPTION:
 *
 *  This function is the state machine dispatcher for the HttpDefaultClient
 *  pointed to by "client". Results are returned by changes to various fields
 *  in the context.
 *
 * PARAMETERS:
 *  "client"
 *      The address of the HttpDefaultClient object. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a HttpDefaultClient Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_HttpDefaultClient_Dispatch(
        PKIX_PL_HttpDefaultClient *client,
        void *plContext)
{
        PKIX_UInt32 bytesTransferred = 0;
        PKIX_Boolean keepGoing = PKIX_TRUE;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Dispatch");
        PKIX_NULLCHECK_ONE(client);

        while (keepGoing) {
                switch (client->connectStatus) {
                case HTTP_CONNECT_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_ConnectContinue
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTCONNECTCONTINUEFAILED);
                    break;
                case HTTP_CONNECTED:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_Send
                        (client, &keepGoing, &bytesTransferred, plContext),
                        PKIX_HTTPDEFAULTCLIENTSENDFAILED);
                    break;
                case HTTP_SEND_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_SendContinue
                        (client, &keepGoing, &bytesTransferred, plContext),
                        PKIX_HTTPDEFAULTCLIENTSENDCONTINUEFAILED);
                    break;
                case HTTP_RECV_HDR:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvHdr
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVHDRFAILED);
                    break;
                case HTTP_RECV_HDR_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvHdrContinue
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVHDRCONTINUEFAILED);
                    break;
                case HTTP_RECV_BODY:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvBody
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVBODYFAILED);
                    break;
                case HTTP_RECV_BODY_PENDING:
                    PKIX_CHECK(pkix_pl_HttpDefaultClient_RecvBodyContinue
                        (client, &keepGoing, plContext),
                        PKIX_HTTPDEFAULTCLIENTRECVBODYCONTINUEFAILED);
                    break;
                case HTTP_ERROR:
                case HTTP_COMPLETE:
                    keepGoing = PKIX_FALSE;
                    break;
                case HTTP_NOT_CONNECTED:
                default:
                    PKIX_ERROR(PKIX_HTTPDEFAULTCLIENTINILLEGALSTATE);
                }
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);
}

/*
 * --HttpClient vtable functions
 * See comments in ocspt.h for the function (wrappers) that return SECStatus.
 * The functions that return PKIX_Error* are the libpkix implementations.
 */

PKIX_Error *
pkix_pl_HttpDefaultClient_CreateSession(
        const char *host,
        PRUint16 portnum,
        SEC_HTTP_SERVER_SESSION *pSession,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_CreateSession");
        PKIX_NULLCHECK_TWO(host, pSession);

        PKIX_CHECK(pkix_pl_HttpDefaultClient_Create
                (host, portnum, &client, plContext),
                PKIX_HTTPDEFAULTCLIENTCREATEFAILED);

        *pSession = (SEC_HTTP_SERVER_SESSION)client;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_KeepAliveSession(
        SEC_HTTP_SERVER_SESSION session,
        PRPollDesc **pPollDesc,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_KeepAliveSession");
        PKIX_NULLCHECK_TWO(session, pPollDesc);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)session,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_SESSIONNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)session;

        /* XXX Not implemented */

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_FreeSession(
        SEC_HTTP_SERVER_SESSION session,
        void *plContext)
{

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_FreeSession");
        PKIX_DECREF(session);

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_RequestCreate(
        SEC_HTTP_SERVER_SESSION session,
        const char *http_protocol_variant, /* usually "http" */
        const char *path_and_query_string,
        const char *http_request_method, 
        const PRIntervalTime timeout, 
        SEC_HTTP_REQUEST_SESSION *pRequest,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;
        PKIX_PL_Socket *socket = NULL;
        PKIX_PL_Socket_Callback *callbackList = NULL;
        PRFileDesc *fileDesc = NULL;
        PRErrorCode status = 0;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_RequestCreate");
        PKIX_NULLCHECK_TWO(session, pRequest);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)session,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_SESSIONNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)session;

        /* We only know how to do http */
        if (PORT_Strncasecmp(http_protocol_variant, "http", 4) != 0) {
                PKIX_ERROR(PKIX_UNRECOGNIZEDPROTOCOLREQUESTED);
        }

        if (PORT_Strncasecmp(http_request_method, "POST", 4) == 0) {
                client->send_http_method = HTTP_POST_METHOD;
        } else if (PORT_Strncasecmp(http_request_method, "GET", 3) == 0) {
                client->send_http_method = HTTP_GET_METHOD;
        } else {
                /* We only know how to do POST and GET */
                PKIX_ERROR(PKIX_UNRECOGNIZEDREQUESTMETHOD);
        }

        client->path = path_and_query_string;

        client->timeout = timeout;

#if 0
	PKIX_CHECK(pkix_HttpCertStore_FindSocketConnection
                (timeout,
                "variation.red.iplanet.com", /* (char *)client->host, */
                2001,   /* client->portnum, */
                &status,
                &socket,
                plContext),
		PKIX_HTTPCERTSTOREFINDSOCKETCONNECTIONFAILED);
#else
	PKIX_CHECK(pkix_HttpCertStore_FindSocketConnection
                (timeout,
                (char *)client->host,
                client->portnum,
                &status,
                &socket,
                plContext),
		PKIX_HTTPCERTSTOREFINDSOCKETCONNECTIONFAILED);
#endif

        client->socket = socket;

        PKIX_CHECK(pkix_pl_Socket_GetCallbackList
                (socket, &callbackList, plContext),
                PKIX_SOCKETGETCALLBACKLISTFAILED);

        client->callbackList = (void *)callbackList;

        PKIX_CHECK(pkix_pl_Socket_GetPRFileDesc
                (socket, &fileDesc, plContext),
                PKIX_SOCKETGETPRFILEDESCFAILED);

        client->pollDesc.fd = fileDesc;
        client->pollDesc.in_flags = 0;
        client->pollDesc.out_flags = 0;

        client->send_http_data = NULL;
        client->send_http_data_len = 0;
        client->send_http_content_type = NULL;

        client->connectStatus =
                 ((status == 0) ? HTTP_CONNECTED : HTTP_CONNECT_PENDING);

        /* Request object is the same object as Session object */
        PKIX_INCREF(client);
        *pRequest = client;

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_SetPostData(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_data, 
        const PRUint32 http_data_len,
        const char *http_content_type,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_SetPostData");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        client->send_http_data = http_data;
        client->send_http_data_len = http_data_len;
        client->send_http_content_type = http_content_type;

        /* Caller is allowed to give NULL or empty string for content_type */
        if ((client->send_http_content_type == NULL) ||
            (*(client->send_http_content_type) == '\0')) {
                client->send_http_content_type = "application/ocsp-request";
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_AddHeader(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_header_name, 
        const char *http_header_value,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_AddHeader");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        PKIX_ERROR(PKIX_ADDHEADERFUNCTIONNOTSUPPORTED);

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_TrySendAndReceive(
        SEC_HTTP_REQUEST_SESSION request,
        PRUint16 *http_response_code, 
        const char **http_response_content_type, 
        const char **http_response_headers, 
        const char **http_response_data, 
        PRUint32 *http_response_data_len, 
        PRPollDesc **pPollDesc,
        SECStatus *pSECReturn,
        void *plContext)        
{
        PKIX_PL_HttpDefaultClient *client = NULL;
        PKIX_UInt32 postLen = 0;
        PRPollDesc *pollDesc = NULL;
        char *sendbuf = NULL;
        void *appendDest = NULL;

        PKIX_ENTER
                (HTTPDEFAULTCLIENT,
                "pkix_pl_HttpDefaultClient_TrySendAndReceive");

        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        if (!pPollDesc && client->timeout == 0) {
            PKIX_ERROR_FATAL(PKIX_NULLARGUMENT);
        }

        if (pPollDesc) {
            pollDesc = *pPollDesc;
        }

        /* if not continuing from an earlier WOULDBLOCK return... */
        if (pollDesc == NULL) {

                if (!((client->connectStatus == HTTP_CONNECTED) ||
                     (client->connectStatus == HTTP_CONNECT_PENDING))) {
                        PKIX_ERROR(PKIX_HTTPCLIENTININVALIDSTATE);
                }

                /* Did caller provide a value for response length? */
                if (http_response_data_len != NULL) {
                        client->pRcv_http_data_len = http_response_data_len;
                        client->maxResponseLen = *http_response_data_len;
                }

                client->rcv_http_response_code = http_response_code;
                client->rcv_http_content_type = http_response_content_type;
                client->rcv_http_headers = http_response_headers;
                client->rcv_http_data = http_response_data;

                /* prepare the message */
                if (client->send_http_method == HTTP_POST_METHOD) {
                        PKIX_PL_NSSCALLRV
                            (HTTPDEFAULTCLIENT, sendbuf, PR_smprintf,
                            ("POST %s HTTP/1.0\r\nHost: %s:%d\r\n"
                            "Content-Type: %s\r\nContent-Length: %u\r\n\r\n",
                            client->path,
                            client->host,
                            client->portnum,
                            client->send_http_content_type,
                            client->send_http_data_len));

                        PKIX_PL_NSSCALLRV
                                (HTTPDEFAULTCLIENT, postLen, PORT_Strlen,
                                (sendbuf));

                        client->POSTLen = postLen + client->send_http_data_len;

                        /* allocate postBuffer big enough for header + data */
                        PKIX_CHECK(PKIX_PL_Malloc
                                (client->POSTLen,
                                (void **)&(client->POSTBuf),
                                plContext),
                                PKIX_MALLOCFAILED);

                        /* copy header into postBuffer */
                        PKIX_CHECK(PKIX_PL_Memcpy
                                (sendbuf,
                                postLen,
                                (void **)&(client->POSTBuf),
                                plContext),
                                PKIX_MEMCPYFAILED);

                        /* append data after header */
                        appendDest = (void *)&(client->POSTBuf[postLen]);
                        PKIX_CHECK(PKIX_PL_Memcpy
                                ((void *)(client->send_http_data),
                                 client->send_http_data_len,
                                 (void **)&appendDest,
                                plContext),
                                PKIX_MEMCPYFAILED);

                        /* PR_smprintf_free original header buffer */
                        PKIX_PL_NSSCALL
                                (HTTPDEFAULTCLIENT, PR_smprintf_free,
                                (sendbuf));
                        
                } else if (client->send_http_method == HTTP_GET_METHOD) {
                        PKIX_PL_NSSCALLRV
                            (HTTPDEFAULTCLIENT, sendbuf, PR_smprintf,
                            ("GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n",
                            client->path,
                            client->host,
                            client->portnum));

                        client->GETBuf = sendbuf;

                        PKIX_PL_NSSCALLRV
                                (HTTPDEFAULTCLIENT, client->GETLen, PORT_Strlen,
                                (sendbuf));
                }

        }

        /* continue according to state */
        PKIX_CHECK(pkix_pl_HttpDefaultClient_Dispatch(client, plContext),
                PKIX_HTTPDEFAULTCLIENTDISPATCHFAILED);

        switch (client->connectStatus) {
                case HTTP_CONNECT_PENDING:
                case HTTP_SEND_PENDING:
                case HTTP_RECV_HDR_PENDING:
                case HTTP_RECV_BODY_PENDING:
                        pollDesc = &(client->pollDesc);
                        *pSECReturn = SECWouldBlock;
                        break;
                case HTTP_ERROR:
                        /* Did caller provide a pointer for length? */
                        if (client->pRcv_http_data_len != NULL) {
                                /* Was error "response too big?" */
                                if (client->maxResponseLen >=
                                        client->rcv_http_data_len) {
                                        /* Yes, report needed space */
                                        *(client->pRcv_http_data_len) =
                                                 client->rcv_http_data_len;
                                } else {
                                        /* No, report problem other than size */
                                        *(client->pRcv_http_data_len) = 0;
                                }
                        }

                        pollDesc = NULL;
                        *pSECReturn = SECFailure;
                        break;
                case HTTP_COMPLETE:
                        *(client->rcv_http_response_code) =
                                client->responseCode;
                        if (client->pRcv_http_data_len != NULL) {
                                *http_response_data_len =
                                         client->rcv_http_data_len;
                        }
                        if (client->rcv_http_data != NULL) {
                                *(client->rcv_http_data) = client->rcvBuf;
                        }
                        pollDesc = NULL;
                        *pSECReturn = SECSuccess;
                        break;
                case HTTP_NOT_CONNECTED:
                case HTTP_CONNECTED:
                case HTTP_RECV_HDR:
                case HTTP_RECV_BODY:
                default:
                        pollDesc = NULL;
                        *pSECReturn = SECFailure;
                        PKIX_ERROR(PKIX_HTTPCLIENTININVALIDSTATE);
                        break;
        }

        if (pPollDesc) {
            *pPollDesc = pollDesc;
        }

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_Cancel(
        SEC_HTTP_REQUEST_SESSION request,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Cancel");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        /* ... */

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

PKIX_Error *
pkix_pl_HttpDefaultClient_Free(
        SEC_HTTP_REQUEST_SESSION request,
        void *plContext)
{
        PKIX_PL_HttpDefaultClient *client = NULL;

        PKIX_ENTER(HTTPDEFAULTCLIENT, "pkix_pl_HttpDefaultClient_Free");
        PKIX_NULLCHECK_ONE(request);

        PKIX_CHECK(pkix_CheckType
                ((PKIX_PL_Object *)request,
                PKIX_HTTPDEFAULTCLIENT_TYPE,
                plContext),
                PKIX_REQUESTNOTANHTTPDEFAULTCLIENT);

        client = (PKIX_PL_HttpDefaultClient *)request;

        /* XXX Not implemented */

        PKIX_DECREF(client);

cleanup:

        PKIX_RETURN(HTTPDEFAULTCLIENT);

}

SECStatus
pkix_pl_HttpDefaultClient_CreateSessionFcn(
        const char *host,
        PRUint16 portnum,
        SEC_HTTP_SERVER_SESSION *pSession)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_CreateSession
                (host, portnum, pSession, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_KeepAliveSessionFcn(
        SEC_HTTP_SERVER_SESSION session,
        PRPollDesc **pPollDesc)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_KeepAliveSession
                (session, pPollDesc, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_FreeSessionFcn(
        SEC_HTTP_SERVER_SESSION session)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_FreeSession
                (session, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_RequestCreateFcn(
        SEC_HTTP_SERVER_SESSION session,
        const char *http_protocol_variant, /* usually "http" */
        const char *path_and_query_string,
        const char *http_request_method, 
        const PRIntervalTime timeout, 
        SEC_HTTP_REQUEST_SESSION *pRequest)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_RequestCreate
                (session,
                http_protocol_variant,
                path_and_query_string,
                http_request_method,
                timeout,
                pRequest,
                plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_SetPostDataFcn(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_data, 
        const PRUint32 http_data_len,
        const char *http_content_type)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_SetPostData
                (request,
                http_data,
                http_data_len,
                http_content_type,
                plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_AddHeaderFcn(
        SEC_HTTP_REQUEST_SESSION request,
        const char *http_header_name, 
        const char *http_header_value)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_AddHeader
                (request, http_header_name, http_header_value, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_TrySendAndReceiveFcn(
        SEC_HTTP_REQUEST_SESSION request,
        PRPollDesc **pPollDesc,
        PRUint16 *http_response_code, 
        const char **http_response_content_type, 
        const char **http_response_headers, 
        const char **http_response_data, 
        PRUint32 *http_response_data_len) 
{
        SECStatus rv = SECFailure;

        PKIX_Error *err = pkix_pl_HttpDefaultClient_TrySendAndReceive
                (request,
                http_response_code, 
                http_response_content_type, 
                http_response_headers, 
                http_response_data, 
                http_response_data_len,
                pPollDesc,
                &rv,
                plContext);

        if (err == NULL) {
                return rv;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return rv;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_CancelFcn(
        SEC_HTTP_REQUEST_SESSION request)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_Cancel(request, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}

SECStatus
pkix_pl_HttpDefaultClient_FreeFcn(
        SEC_HTTP_REQUEST_SESSION request)
{
        PKIX_Error *err = pkix_pl_HttpDefaultClient_Free(request, plContext);

        if (err == NULL) {
                return SECSuccess;
        } else {
                PKIX_PL_Object_DecRef((PKIX_PL_Object *)err, plContext);
                return SECFailure;
        }
}
