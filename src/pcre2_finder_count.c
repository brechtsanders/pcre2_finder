#include "pcre2_finder.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define READBUFFERSIZE 128

struct count_data_struct {
  size_t count;
  size_t* patterncounts;
};

static int when_found (struct pcre2_finder* finder, const char* data, size_t datalen, void* callbackdata, int matchid)
{
  struct count_data_struct* countdata = (struct count_data_struct*)callbackdata;
  countdata->count++;
  countdata->patterncounts[matchid]++;
  return 0;
}

void show_help()
{
  printf(
    "Usage:  pcre2_finder_count [[-?|-h] -c] [-i] [-f file] [-t text] [-p <pattern>] <pattern> ...\n" \
    "Parameters:\n" \
    "  -? | -h     \tshow help\n" \
    "  -c          \tcase sensitive matching for next pattern(s) (default)\n" \
    "  -i          \tcase insensitive matching for next pattern(s)\n" \
    "  -f file     \tinput file (default is to use standard input)\n" \
    "  -t text     \tuse text as search data (overrides -f)\n" \
    "  -p pattern  \tpattern to search for (can be used if pattern starts with \"-\")\n" \
    "  pattern     \tpattern to search for\n" \
    "Version: " PCRE2_FINDER_VERSION_STRING "\n" \
    "\n"
  );
}

int main (int argc, char** argv)
{
  struct pcre2_finder* finder;
  struct count_data_struct countdata;
  int flags = PCRE2_DFA_SHORTEST;
  const char* srcfile = NULL;
  const char* srctext = NULL;
  size_t* patterncounts = NULL;
  size_t patterns = 0;
  //initialize
  if ((patterncounts = (size_t*)malloc((argc - 1) * sizeof(size_t))) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 2;
  }
  countdata.count = 0;
  countdata.patterncounts = patterncounts;
  if ((finder = pcre2_finder_initialize()) == NULL) {
    fprintf(stderr, "Error in pcre2_finder_initialize()\n");
    return 3;
  }
  //process command line parameters
  {
    int i = 0;
    char* param;
    int paramerror = 0;
    while (!paramerror && ++i < argc) {
      if (argv[i][0] == '-') {
        param = NULL;
        switch (tolower(argv[i][1])) {
          case '?' :
          case 'h' :
            if (argv[i][2])
              paramerror++;
            else
              show_help();
            return 0;
          case 'c' :
            if (argv[i][2])
              paramerror++;
            else
              flags &= ~PCRE2_CASELESS;
            break;
          case 'i' :
            if (argv[i][2])
              paramerror++;
            else
              flags |= PCRE2_CASELESS;
            break;
          case 'f' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              srcfile = param;
            break;
          case 't' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              srctext = param;
            break;
          case 'p' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else {
              patterncounts[patterns] = 0;
              pcre2_finder_add_expr(finder, param, flags, when_found, &countdata, patterns++);
            }
            break;
          default :
            paramerror++;
            break;
        }
      } else {
        patterncounts[patterns] = 0;
        pcre2_finder_add_expr(finder, argv[i], flags, when_found, &countdata, patterns++);
      }
    }
    if (paramerror || argc <= 1) {
      if (paramerror)
        fprintf(stderr, "Invalid command line parameters\n");
      show_help();
      return 1;
    }
  }
  //prepare finder for searching
  if (pcre2_finder_open(finder, pcre2_finder_output_to_null, NULL) != 0) {
    fprintf(stderr, "Error in pcre2_finder_open()\n");
    pcre2_finder_cleanup(finder);
    return 4;
  }
  //process search data
  if (srctext) {
    //process supplied text
    if (pcre2_finder_process(finder, srctext, strlen(srctext)) < 0) {
      fprintf(stderr, "Error in pcre2_finder_process()\n");
    }
  } else {
    //process file (or standard input)
    FILE* src;
    char buf[READBUFFERSIZE];
    size_t buflen;
    if (!srcfile) {
      src = stdin;
    } else {
      if ((src = fopen(srcfile, "rb")) == NULL) {
        fprintf(stderr, "Error opening file: %s\n", srcfile);
        pcre2_finder_cleanup(finder);
        return 5;
      }
    }
    while ((buflen = fread(buf, 1, READBUFFERSIZE, src)) > 0) {
      if (pcre2_finder_process(finder, buf, buflen) < 0) {
        fprintf(stderr, "Error in pcre2_finder_process()\n");
      }
    }
    fclose(src);
  }
  pcre2_finder_close(finder);
  //show results
  printf("%lu matches found\n", (unsigned long)countdata.count);
  {
    size_t i;
    for (i = 0; i < patterns; i++)
      printf("pattern %lu found %lu times\n", (unsigned long)i + 1, (unsigned long)patterncounts[i]);
  }
  //clean up
  free(patterncounts);
  pcre2_finder_cleanup(finder);
  return 0;
}
