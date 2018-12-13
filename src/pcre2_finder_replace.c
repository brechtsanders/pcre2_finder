#include "pcre2_finder.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define READBUFFERSIZE 128

struct replace_data_struct {
  size_t count;
  size_t* patterncounts;
  const char** patternreplacements;
};

static int when_found (struct pcre2_finder* finder, const char* data, size_t datalen, void* callbackdata, int matchid)
{
  struct replace_data_struct* replacedata = (struct replace_data_struct*)callbackdata;
  replacedata->count++;
  replacedata->patterncounts[matchid]++;
  pcre2_finder_output(finder, replacedata->patternreplacements[matchid], strlen(replacedata->patternreplacements[matchid]));
  return 0;
}

static size_t flushsearchdata (void* callbackdata, const char* data, size_t datalen)
{
  if (datalen)
    fprintf((FILE*)callbackdata, "%.*s", (int)datalen, data);
  return 0;
}

void show_help()
{
  printf(
    "Usage:  pcre2_finder_replace [-?|-h] [-c] [-i] [-f file] [-t text] [-p <pattern> <replacement>] <pattern> <replacement> ...\n" \
    "Parameters:\n" \
    "  -? | -h     \tshow help\n" \
    "  -c          \tcase sensitive matching for next pattern(s) (default)\n" \
    "  -i          \tcase insensitive matching for next pattern(s)\n" \
    "  -f file     \tinput file (default is to use standard input)\n" \
    "  -o file     \toutput file (default is to use standard output)\n" \
    "  -v          \tprint number of replacements done\n" \
    "  -t text     \tuse text as search data (overrides -f)\n" \
    "  -p          \tnext 2 parameters are pattern and replacement (can be used if pattern or replacement starts with \"-\")\n" \
    "  pattern     \tpattern to search for\n" \
    "  replacement \treplacement to replace pattern with\n" \
    "Version: " PCRE2_FINDER_VERSION_STRING "\n" \
    "\n"
  );
}

int main (int argc, char** argv)
{
  struct pcre2_finder* finder;
  struct replace_data_struct replacedata;
  FILE* dst;
  int flags = PCRE2_DFA_SHORTEST;
  int verbose = 0;
  const char* srcfile = NULL;
  const char* dstfile = NULL;
  const char* srctext = NULL;
  size_t* patterncounts = NULL;
  const char** patternreplacements = NULL;
  size_t patterns = 0;
  //initialize
  if ((patterncounts = (size_t*)malloc((argc - 1) * sizeof(size_t))) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 2;
  }
  if ((patternreplacements = (const char**)malloc((argc - 1) * sizeof(char*))) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    return 2;
  }
  replacedata.count = 0;
  replacedata.patterncounts = patterncounts;
  replacedata.patternreplacements = patternreplacements;
  if ((finder = pcre2_finder_initialize()) == NULL) {
    fprintf(stderr, "Error in pcre2_finder_initialize()\n");
    return 2;
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
          case 'o' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              dstfile = param;
            break;
          case 'v' :
            if (argv[i][2])
              paramerror++;
            else
              verbose = 1;
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
            {
              char* param2 = NULL;
              if (argv[i][2])
                param = argv[i] + 2;
              else if (i + 2 < argc && argv[i + 1] && argv[i + 2]) {
                param = argv[++i];
                param2 = argv[++i];
              }
              if (!param || !param2)
                paramerror++;
              else {
                patterncounts[patterns] = 0;
                patternreplacements[patterns] = param2;
                pcre2_finder_add_expr(finder, param, flags, when_found, &replacedata, patterns++);
              }
              break;
            }
          default :
            paramerror++;
            break;
        }
      } else if (i + 1 < argc) {
        patterncounts[patterns] = 0;
        patternreplacements[patterns] = argv[i + 1];
        pcre2_finder_add_expr(finder, argv[i], flags, when_found, &replacedata, patterns++);
        i++;
      } else {
        paramerror++;
        break;
      }
    }
    if (paramerror || argc <= 1) {
      if (paramerror)
        fprintf(stderr, "Invalid command line parameters\n");
      show_help();
      return 1;
    }
  }
  //open output
  if (!dstfile)
    dst = stdout;
  else
    dst = fopen(dstfile, "wb");
  if (dst == NULL) {
    fprintf(stderr, "Error opening output file: %s\n", srcfile);
    pcre2_finder_cleanup(finder);
    return 3;
  }
  //prepare finder for searching
  if (pcre2_finder_open(finder, flushsearchdata, dst) != 0) {
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
  if (verbose) {
    size_t i;
    if (dst == stdout)
      printf("\n");
    printf("%lu matches replaced\n", (unsigned long)replacedata.count);
    for (i = 0; i < patterns; i++)
      printf("pattern %lu replaced %lu times\n", (unsigned long)i + 1, (unsigned long)patterncounts[i]);
  }
  //clean up
  free(patterncounts);
  free(patternreplacements);
  pcre2_finder_cleanup(finder);
  return 0;
}
