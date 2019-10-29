/**

  @file    filesys/linux/osal_file.c
  @brief   Basic file IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  File IO for linux.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_FILESYS_SUPPORT

#include <stdio.h>
#include <errno.h>


/**
****************************************************************************************************

  @name File data structure.

  The osalStream is pointer to a stream, like stream handle. It is defined as pointer to
  dummy structure to provide compiler type checking. This sturcture is never really allocated,
  and OSAL functions cast their own stream structure pointers to osalStream pointers.

****************************************************************************************************
*/
typedef struct osalFile
{
    /** The structure must start with file header structure. This includes generic
	    stream header, which contains parameters common to every stream. 
	 */
	osalStreamHeader hdr;

    /** Operating system's file handle.
	 */
    FILE *handle;

    /** Flags which were given to osal_file_open() function.
	 */
	os_int open_flags;

#if OSAL_MAIN_SUPPORT
    /** Flag indicating that we are using standard input or output.
     */
    os_boolean is_std_stream;
#endif
}
osalFile;


/**
****************************************************************************************************

  @brief Open a file.
  @anchor osal_file_open

  The osal_file_open() function opens a file.

  @param  parameters Path to file.

  @param  option Not used for files, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
		  OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
		  This parameter can be OS_NULL, if no status code is needed. 

  @param  flags Flags for creating the file. Bit fields, combination of:
          - OSAL_STREAM_READ Open stream for reading. To open stream for both reading and
            writing, use OSAL_STREAM_RW.
          - OSAL_STREAM_WRITE Open stream for writing.
          - OSAL_STREAM_RW Open stream for both reading and writing.
          - OSAL_STREAM_APPEND Open stream for appending. Current file content is preserved
            and file pointer is set at end of file.

		  See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the file, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_file_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
    os_char *mode;
    FILE *handle;
    osalFile *myfile;
    osalStatus rval = OSAL_STATUS_FAILED;

#if OSAL_MAIN_SUPPORT
    os_boolean is_std_stream;
#endif

    /* Sekect fopen mode by flags.
     */
    if ((flags & OSAL_STREAM_RW) == OSAL_STREAM_RW)
    {
        mode = (flags & OSAL_STREAM_APPEND) ? "a+" : "w+";
    }
    else if (flags & OSAL_STREAM_WRITE)
    {
        mode = (flags & OSAL_STREAM_APPEND) ? "a" : "w";
    }
    else
    {
        mode = "r";
    }

#if OSAL_MAIN_SUPPORT
    is_std_stream = OS_FALSE;
    if (!os_strcmp(parameters, ".stdin"))
    {
        handle = stdin;
    }
    else if (!os_strcmp(parameters, ".stdout"))
    {
        handle = stdout;
    }
    else
    {
#endif
    /* Open the file.
        */
    handle = fopen(parameters, mode);

    /* If opening file failed, and we are opening file for
        reading, try case insensitive open.
        */
    if (handle == NULL)
    {
        switch (errno)
        {
            case EACCES:
                rval = OSAL_STATUS_NO_ACCESS_RIGHT;
                break;

            case ENOSPC:
                rval = OSAL_STATUS_DISC_FULL;
                break;

            case ENOENT:
                rval = OSAL_FILE_DOES_NOT_EXIST;
                break;
        }
        goto getout;
    }

#if OSAL_MAIN_SUPPORT
    }
#endif

    /* Allocate and clear file structure.
	 */
    myfile = (osalFile*)os_malloc(sizeof(osalFile), OS_NULL);
    os_memclear(myfile, sizeof(osalFile));

    /* Save file handle and open flags.
	 */
    myfile->handle = handle;
    myfile->open_flags = flags;

	/* Save interface pointer.
	 */
    myfile->hdr.iface = &osal_file_iface;

#if OSAL_MAIN_SUPPORT
    myfile->is_std_stream = is_std_stream;
#endif

    /* Success set status code and cast file structure pointer to stream pointer and return it.
	 */
	if (status) *status = OSAL_SUCCESS;
    return (osalStream)myfile;

getout:
	/* Set status code and return NULL pointer.
	 */
	if (status) *status = rval;
	return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close file.
  @anchor osal_file_close

  The osal_file_close() function closes a file, which was opened by osal_file_open()
  function. All resources related to the file are freed. Any attemp to use the file after
  this call may result crash.

  @param   stream Stream pointer representing the file. After this call stream pointer will
		   point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_file_close(
	osalStream stream)
{
    osalFile *myfile;
    FILE *handle;

	/* If called with NULL argument, do nothing.
	 */
	if (stream == OS_NULL) return;

    /* Cast stream pointer to osalFile pointer and get OS handle.
	 */
    myfile = (osalFile*)stream;
    handle = myfile->handle;

    /* If file operating system file is not already closed, close now.
	 */
    if (handle != NULL
#if OSAL_MAIN_SUPPORT
        && !myfile->is_std_stream
#endif
    ) {
        /* Close the file.
		 */
        if (fclose(handle))
		{
            osal_debug_error("closing file failed");
		}
	}

    /* Free memory allocated for file structure.
     */
    os_free(myfile, sizeof(osalFile));
}


/**
****************************************************************************************************

  @brief Flush the file.
  @anchor osal_file_flush

  The osal_file_flush() function flushes data to be written to stream.

  @param   stream Stream pointer representing the file.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_file_flush(
	osalStream stream,
	os_int flags)
{
    osalFile *myfile;
    FILE *handle;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to osalFile pointer and get OS handle.
     */
    myfile = (osalFile*)stream;
    handle = myfile->handle;
    if (handle == NULL) return OSAL_STATUS_FAILED;

    /* Flush the file and return PAL status code.
     */
    return fflush(handle) ? OSAL_STATUS_FAILED : OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to file.
  @anchor osal_file_write

  The osal_file_write() function writes n bytes of data from buffer to file.

  @param   stream Stream pointer representing the file.
  @param   buf Pointer to data to write to the file.
  @param   n Number of bytes to write.
  @param   n_written Pointer to integer into which the function stores the number of bytes 
           actually written to file, which may be less than n if there is not enough space
           left in the disk. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_file_write(
	osalStream stream,
    const os_char *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags)
{
    osalFile *myfile;
    FILE *handle;

    /* If stream pointer is NULL?
     */
    if (stream == OS_NULL) goto getout;

    /* If nothing to write?
     */
    if (n == 0)
    {
        *n_written = 0;
        return OSAL_SUCCESS;
    }

    /* OS file handle. if already close get out.
     */
    myfile = (osalFile*)stream;
    handle = myfile->handle;
    if (handle == OS_NULL) goto getout;

    /* Write data to file.
     */
    *n_written = fwrite(buf, 1, n, handle);
    return *n_written == n ? OSAL_SUCCESS : OSAL_STATUS_FAILED;

getout:
	*n_written = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from file.
  @anchor osal_file_read

  The osal_file_read() function reads up to n bytes of data from file into buffer.

  @param   stream Stream pointer representing the file.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.
           The OSAL_STREAM_PEEK flag causes the function to return data in file, but nothing
           will be removed from the file. See @ref osalStreamFlags "Flags for Stream Functions"
           for list of stream flags.
  @return  The function returns OSAL_SUCCESS (0) if any data was read. Return code
.		   Return value OSAL_END_OF_FILE indicates end of file (nread is set to zero).
           Other return values inficate an error.

****************************************************************************************************
*/
osalStatus osal_file_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalFile *myfile;
    FILE *handle;

    /* If stream pointer is NULL?
     */
    if (stream == OS_NULL) goto getout;

    /* If nothing to write?
     */
    if (n == 0)
    {
        *n_read = 0;
        return OSAL_SUCCESS;
    }

    /* OS file handle. if already close get out.
     */
    myfile = (osalFile*)stream;
    handle = myfile->handle;
    if (handle == OS_NULL) goto getout;

    /* Read data from file.
     */
    *n_read = fread(buf, 1, n, handle);
    if (*n_read) return OSAL_SUCCESS;

    /* Return error or end of file.
     */
    return ferror(handle) ? OSAL_STATUS_FAILED : OSAL_END_OF_FILE;

getout:
    *n_read = 0;
    return OSAL_STATUS_FAILED;
}


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL files. This is structure osalStreamInterface filled with
    function pointers to OSAL files implementation.
 */
const osalStreamInterface osal_file_iface
 = {osal_file_open,
    osal_file_close,
    osal_stream_default_accept,
    osal_file_flush,
	osal_stream_default_seek,
    osal_file_write,
    osal_file_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
    osal_stream_default_select};

#endif

#endif
