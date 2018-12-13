/*
 * Copyright (c) 2018, Brecht Sanders
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 */

 /**
 * @file      pcre2_finder.h
 * @brief     pcre2_finder header file with main functions
 * @author    Brecht Sanders
 * @date      2018
 * @copyright BSD
 *
 * This header file defines the functions needed search multiple patterns in a stream of (text) data.
 * It allows transforming the input stream into a modified output stream.
 * Multiple expressions are searched in multiple passes, which is useful when expressions overlap with each other.
 * This library depends on PCRE2 (https://www.pcre.org/) and is also released under a BSD license.
 */

#ifndef INCLUDED_PCRE2_FINDER_H
#define INCLUDED_PCRE2_FINDER_H

#include <stdlib.h>
#include <stdio.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#if defined(STATIC) || defined(BUILD_PCRE2_FINDER_STATIC)
#define PCRE2_STATIC 1
#endif
#include <pcre2.h>

/*! \cond PRIVATE */
#ifdef _WIN32
#if defined(BUILD_PCRE2_FINDER_DLL)
#define DLL_EXPORT_PCRE2_FINDER __declspec(dllexport)
#elif !defined(STATIC) && !defined(BUILD_PCRE2_FINDER_STATIC)
#define DLL_EXPORT_PCRE2_FINDER __declspec(dllimport)
#else
#define DLL_EXPORT_PCRE2_FINDER
#endif
#else
#define DLL_EXPORT_PCRE2_FINDER
#endif
/*! \endcond */

/*! \brief version number constants
 * \sa     pcre2_finder_get_version()
 * \sa     pcre2_finder_get_version_string()
 * \name   PCRE2_FINDER_VERSION_*
 * \{
 */
/*! \brief major version number */
#define PCRE2_FINDER_VERSION_MAJOR 0
/*! \brief minor version number */
#define PCRE2_FINDER_VERSION_MINOR 1
/*! \brief micro version number */
#define PCRE2_FINDER_VERSION_MICRO 0
/*! @} */

/*! \cond PRIVATE */
#define PCRE2_FINDER_VERSION_STRINGIZE_(major, minor, micro) #major"."#minor"."#micro
#define PCRE2_FINDER_VERSION_STRINGIZE(major, minor, micro) PCRE2_FINDER_VERSION_STRINGIZE_(major, minor, micro)
/*! \endcond */

/*! \brief string with dotted version number \hideinitializer */
#define PCRE2_FINDER_VERSION_STRING PCRE2_FINDER_VERSION_STRINGIZE(PCRE2_FINDER_VERSION_MAJOR, PCRE2_FINDER_VERSION_MINOR, PCRE2_FINDER_VERSION_MICRO)

/*! \brief string with name of library */
#define PCRE2_FINDER_NAME "pcre2_finder"

/*! \brief string with name and version of library \hideinitializer */
#define PCRE2_FINDER_FULLNAME PCRE2_FINDER_NAME " " PCRE2_FINDER_VERSION_STRING

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief get pcre2_finder version
 * \param  pmajor        pointer to integer that will receive major version number
 * \param  pminor        pointer to integer that will receive minor version number
 * \param  pmicro        pointer to integer that will receive micro version number
 * \sa     pcre2_finder_get_version_string()
 */
DLL_EXPORT_PCRE2_FINDER void pcre2_finder_get_version (int* pmajor, int* pminor, int* pmicro);

/*! \brief get pcre2_finder version string
 * \return version string
 * \sa     pcre2_finder_get_version()
 */
DLL_EXPORT_PCRE2_FINDER const char* pcre2_finder_get_version_string ();

/*! \brief pcre2_finder object type */
struct pcre2_finder;

/*! \brief type of pointer to function for processing matches
 * \param  finder          pcre2_finder object
 * \param  data            match data (not NULL terminated)
 * \param  datalen         match data length
 * \param  callbackdata    custom data as passed to pcre2_finder_add_expr()
 * \param  matchid         match id as specified in pcre2_finder_add_expr()
 * \return zero to continue, or non-zero to abort further matching
 * \sa     pcre2_finder_add_expr()
 * \sa     pcre2_finder_open()
 * \sa     pcre2_finder_process()
 */
//typedef int (*pcre2_finder_match_fn)(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, struct pcre2_finder* finder);
typedef int (*pcre2_finder_match_fn)(struct pcre2_finder* finder, const char* data, size_t datalen, void* callbackdata, int matchid);

/*! \brief type of pointer to function for processing output (called for all non-matching data)
 * \param  callbackdata    custom data as passed to pcre2_finder_open()
 * \param  data            data to be processed
 * \param  datalen         length of data to be processed
 * \return (unused)
 * \sa     pcre2_finder_open()
 * \sa     pcre2_finder_process()
 * \sa     pcre2_finder_output_to_stream()
 */
typedef size_t (*pcre2_finder_output_fn) (void* callbackdata, const char* data, size_t datalen);

/*! \brief initialize pcre2_finder object
 * \return allocated pcre2_finder object (or NULL on error)
 * \sa     pcre2_finder_cleanup()
 */
DLL_EXPORT_PCRE2_FINDER struct pcre2_finder* pcre2_finder_initialize ();

/*! \brief clean up pcre2_finder object
 * \param  finder          pcre2_finder object
 * \sa     pcre2_finder_initialize()
 */
DLL_EXPORT_PCRE2_FINDER void pcre2_finder_cleanup (struct pcre2_finder* finder);

/*! \brief add search expression to pcre2_finder object
 * \param  finder          pcre2_finder object
 * \param  expr            matching expression
 * \param  flags           matching flags (PCRE2_*)
 * \param  matchfn         function to call for each match
 * \param  callbackdata    user data to pass to matchfn
 * \param  matchid         match id to pass to matchfn
 * \return zero on success
 * \sa     pcre2_finder_initialize()
 * \sa     pcre2_finder_process()
 */
DLL_EXPORT_PCRE2_FINDER int pcre2_finder_add_expr (struct pcre2_finder* finder, const char* expr, unsigned int flags, pcre2_finder_match_fn matchfn, void* callbackdata, int matchid);

/*! \brief function (of type pcre2_finder_output_fn) to write data to a FILE* stream
 * \param  callbackdata    output stream (of type FILE*)
 * \param  data            data to be written
 * \param  datalen         length of data to be written
 * \return number of bytes written
 * \sa     pcre2_finder_output_fn
 */
DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output_to_stream (void* callbackdata, const char* data, size_t datalen);

/*! \brief function (of type pcre2_finder_output_fn) to discard data
 * \param  callbackdata    not used
 * \param  data            data to be written
 * \param  datalen         length of data to be written
 * \return number of bytes discarded
 * \sa     pcre2_finder_output_fn
 */
DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output_to_null (void* callbackdata, const char* data, size_t datalen);

/*! \brief open data stream for searching
 * \param  finder          pcre2_finder object
 * \param  outputfn        function to call for processing output
 * \param  callbackdata    custom data to be passed to \p outputfn
 * \return zero on success
 * \sa     pcre2_finder_process()
 * \sa     pcre2_finder_close()
 * \sa     pcre2_finder_output_fn
 */
DLL_EXPORT_PCRE2_FINDER int pcre2_finder_open (struct pcre2_finder* finder, pcre2_finder_output_fn outputfn, void* callbackdata);

/*! \brief process chunk of data for searching
 * \param  finder          pcre2_finder object
 * \param  data            data to be processed
 * \param  datalen         length of data to be processed
 * \return zero or higher on success
 * \sa     pcre2_finder_open()
 * \sa     pcre2_finder_close()
 * \sa     pcre2_finder_add_expr()
 */
DLL_EXPORT_PCRE2_FINDER int pcre2_finder_process (struct pcre2_finder* finder, const char* data, size_t datalen);

/*! \brief close data stream
 * \param  finder          pcre2_finder object
 * \return zero on success
 * \sa     pcre2_finder_open()
 * \sa     pcre2_finder_process()
 */
DLL_EXPORT_PCRE2_FINDER int pcre2_finder_close (struct pcre2_finder* finder);

/*! \brief output chunk of data to output, to be used inside pcre2_finder_match_fn
 * \param  finder          pcre2_finder object
 * \param  data            data to be sent
 * \param  datalen         length of data to be sent
 * \return value returned by pcre2_finder_output_fn specified in pcre2_finder_open()
 * \sa     pcre2_finder_match_fn
 * \sa     pcre2_finder_open()
 * \sa     pcre2_finder_process()
 * \sa     pcre2_finder_flush()
 * \sa     pcre2_finder_output_fn
 */
DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output (struct pcre2_finder* finder, const char* data, size_t datalen);

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_PCRE2_FINDER_H
