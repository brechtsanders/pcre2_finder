#include "pcre2_finder.h"
#include <stdlib.h>
#include <string.h>

#define PCRE2_OPTIONS PCRE2_PARTIAL_HARD | PCRE2_DFA_SHORTEST
#define PCRE2_DFA_WORKSPACE_SIZE 128

DLL_EXPORT_PCRE2_FINDER void pcre2_finder_get_version (int* pmajor, int* pminor, int* pmicro)
{
  if (pmajor)
    *pmajor = PCRE2_FINDER_VERSION_MAJOR;
  if (pminor)
    *pminor = PCRE2_FINDER_VERSION_MINOR;
  if (pmicro)
    *pmicro = PCRE2_FINDER_VERSION_MICRO;
}

DLL_EXPORT_PCRE2_FINDER const char* pcre2_finder_get_version_string ()
{
  return PCRE2_FINDER_VERSION_STRING;
}

struct pcre2_finder {
  pcre2_finder_match_fn matchfn;
  void* matchcallbackdata;
  int matchid;
  pcre2_finder_output_fn outputfn;
  void* outputcallbackdata;
  char* partialmatch;
  size_t partialmatchlen;
  pcre2_code* re;
  pcre2_match_data* match_data;
  pcre2_match_context* match_context;
  int* dfaworkspace;
  size_t dfaworkspacesize;
  struct pcre2_finder* next;
  struct pcre2_finder* last;
};

DLL_EXPORT_PCRE2_FINDER struct pcre2_finder* pcre2_finder_initialize ()
{
  struct pcre2_finder* result;
  int* dfaworkspace;
  if ((dfaworkspace = (int*)malloc(PCRE2_DFA_WORKSPACE_SIZE * sizeof(int))) == NULL)
    return NULL;
  if ((result = (struct pcre2_finder*)malloc(sizeof(struct pcre2_finder))) != NULL) {
    result->matchfn = NULL;
    result->matchcallbackdata = NULL;
    result->matchid = 0;
    result->outputfn = NULL;
    result->outputcallbackdata = NULL;
    result->partialmatch = NULL;
    result->partialmatchlen = 0;
    result->re = NULL;
    result->match_data = NULL;
    result->match_context = NULL;
    result->dfaworkspace = dfaworkspace;
    result->dfaworkspacesize = PCRE2_DFA_WORKSPACE_SIZE;
    result->next = NULL;
    result->last = result;
  }
  return result;
}

DLL_EXPORT_PCRE2_FINDER void pcre2_finder_cleanup (struct pcre2_finder* finder)
{
  struct pcre2_finder* current;
  struct pcre2_finder* next;
  current = finder;
  while (current) {
    next = current->next;
    if (current->partialmatch)
      free(current->partialmatch);
    if (current->dfaworkspace)
      free(current->dfaworkspace);
    if (current->match_context)
      pcre2_match_context_free(current->match_context);
    if (current->match_data)
      pcre2_match_data_free(current->match_data);
    if (current->re)
      pcre2_code_free(current->re);
    free(current);
    current = next;
  }
}

DLL_EXPORT_PCRE2_FINDER int pcre2_finder_add_expr (struct pcre2_finder* finder, const char* expr, unsigned int flags, pcre2_finder_match_fn matchfn, void* callbackdata, int matchid)
{
  pcre2_code* re;
  int status;
  PCRE2_SIZE erroroffset;
  struct pcre2_finder* current = finder->last;
  //abort if expression is NULL or empty
  if (!expr || !*expr)
    return -1;
  //compile regular expression
  if ((re = pcre2_compile((PCRE2_UCHAR*)expr, PCRE2_ZERO_TERMINATED, flags /*PCRE2_EXTENDED | PCRE2_CASELESS | PCRE2_MULTILINE*/, &status, &erroroffset, NULL))  == NULL) {
/*
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(status, buffer, sizeof(buffer));
    fprintf(stderr, "PCRE2 compilation failed at offset %i: %s\n", (int)erroroffset, buffer);
*/
    return 1;
  }
  //add new instance if needed
  if (current && current->re) {
    current->next = pcre2_finder_initialize();
    current = current->next;
    finder->last = current;
  }
  //set data
  current->re = re;
  current->matchfn = matchfn;
  current->matchcallbackdata = callbackdata;
  current->matchid = matchid;
  //create match result data block
  //current->match_data = pcre2_match_data_create_from_pattern(current->re, NULL);
  current->match_data = pcre2_match_data_create(1, NULL);
  //create match context data block
  current->match_context = pcre2_match_context_create(NULL);
  return 0;
}

DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output_to_stream (void* callbackdata, const char* data, size_t datalen)
{
  return fwrite(data, 1, datalen, (FILE*)callbackdata);
}

DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output_to_null (void* callbackdata, const char* data, size_t datalen)
{
  return datalen;
}

DLL_EXPORT_PCRE2_FINDER int pcre2_finder_open (struct pcre2_finder* finder, pcre2_finder_output_fn outputfn, void* callbackdata)
{
  struct pcre2_finder* current = finder;
  //fail if no expressions are set
  if (finder->last == finder && !finder->re)
    return -1;
  //fail if no output function is set
  if (!outputfn)
    return -2;
  //loop through expressions
  while (current) {
    //set output function (daisy chain with next if not last in chain, otherwise set final output function)
    if (current->next) {
      current->outputfn = (pcre2_finder_output_fn)&pcre2_finder_process;
      current->outputcallbackdata = current->next;
    } else {
      current->outputfn = (pcre2_finder_output_fn)(outputfn ? outputfn : &pcre2_finder_output_to_stream);
      current->outputcallbackdata = callbackdata;
    }
    //continue with next expression
    current = current->next;
  }
  return 0;
}

char* partialmatch_append (struct pcre2_finder* finder, const char* s, size_t slen)
{
  if ((finder->partialmatch = (char*)realloc(finder->partialmatch, finder->partialmatchlen + slen)) == NULL) {
    finder->partialmatchlen = 0;
    return NULL;
  }
  memcpy(finder->partialmatch + finder->partialmatchlen, s, slen);
  finder->partialmatchlen += slen;
  return finder->partialmatch;
}

void partialmatch_clear (struct pcre2_finder* finder)
{
  if (finder->partialmatch)
    free(finder->partialmatch);
  finder->partialmatch = NULL;
  finder->partialmatchlen = 0;
}

DLL_EXPORT_PCRE2_FINDER int pcre2_finder_process (struct pcre2_finder* finder, const char* data, size_t datalen)
{
  int status;
  PCRE2_SIZE* ovector;
  PCRE2_SIZE start_offset = 0;
  //abort if no data was supplied
  if (datalen == 0)
    return 0;
  //continue search after previous partial match
  if (finder->partialmatch) {
    if ((status = pcre2_dfa_match(finder->re, (PCRE2_UCHAR*)data, datalen, start_offset, PCRE2_OPTIONS | PCRE2_DFA_RESTART, finder->match_data, finder->match_context, finder->dfaworkspace, finder->dfaworkspacesize)) >= 0) {
      //match found in combination with previous partial match
      ovector = pcre2_get_ovector_pointer(finder->match_data);
      partialmatch_append(finder, data + ovector[0], ovector[1] - ovector[0]);
      (*finder->matchfn)(finder, finder->partialmatch, finder->partialmatchlen, finder->matchcallbackdata, finder->matchid);
      partialmatch_clear(finder);
      start_offset = ovector[1];
    } else if (status == PCRE2_ERROR_PARTIAL) {
      //partial match continues
      ovector = pcre2_get_ovector_pointer(finder->match_data);
      partialmatch_append(finder, data + ovector[0], ovector[1] - ovector[0]);
      return 0;
    } else if (status == PCRE2_ERROR_NOMATCH) {
      //no match found in combination with previous partial match
      (*finder->outputfn)(finder->outputcallbackdata, finder->partialmatch, finder->partialmatchlen);
      partialmatch_clear(finder);
    } else {
      //abort on any other error
      return status;
    }
  }
  //search data
  while ((status = pcre2_dfa_match(finder->re, (PCRE2_UCHAR*)data, datalen, start_offset, PCRE2_OPTIONS, finder->match_data, finder->match_context, finder->dfaworkspace, finder->dfaworkspacesize)) >= 0) {
    //match found
    ovector = pcre2_get_ovector_pointer(finder->match_data);
    if (ovector[0] > start_offset)
      (*finder->outputfn)(finder->outputcallbackdata, data + start_offset, ovector[0] - start_offset);
    (*finder->matchfn)(finder, data + ovector[0], ovector[1] - ovector[0], finder->matchcallbackdata, finder->matchid);
    start_offset = ovector[1];
  }
  if (status == PCRE2_ERROR_PARTIAL) {
    //keep track of partial match
    ovector = pcre2_get_ovector_pointer(finder->match_data);
    if (ovector[0] > start_offset)
      (*finder->outputfn)(finder->outputcallbackdata, data + start_offset, ovector[0] - start_offset);
    partialmatch_append(finder, data + ovector[0], ovector[1] - ovector[0]);
  } else if (status == PCRE2_ERROR_NOMATCH) {
    //no match found
    if (datalen > start_offset)
      (*finder->outputfn)(finder->outputcallbackdata, data + start_offset, datalen - start_offset);
  } else {
    //abort on any other error
    return status;
  }
  return 0;
}

DLL_EXPORT_PCRE2_FINDER int pcre2_finder_close (struct pcre2_finder* finder)
{
  struct pcre2_finder* current = finder;
  while (current) {
    if (current->partialmatch) {
      (*current->outputfn)(current->outputcallbackdata, current->partialmatch, current->partialmatchlen);
      free(current->partialmatch);
      current->partialmatch = NULL;
      current->partialmatchlen = 0;
    }
    current = current->next;
  }
  return 0;
}

DLL_EXPORT_PCRE2_FINDER size_t pcre2_finder_output (struct pcre2_finder* finder, const char* data, size_t datalen)
{
  return (*finder->outputfn)(finder->outputcallbackdata, data, datalen);
}

