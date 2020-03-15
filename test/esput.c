#include "otb.h"

bool esput_debug = FALSE;

int main(int argc, char *argv[])
{
  bool rc;
  esput_test *test;
  
  for (test = esput_tests; test->fn != NULL; test++)
  {
    LOG("%s: %s", test->name, test->descr);
    rc = test->fn(test->name);
    if (!rc)
    {
      LOG("%s: !!! FAILED", test->name);
      return(1);
    }
    else
    {
      LOG("%s: succeeded", test->name);
    }
  }

  LOG("Tests complete");

  return(0);
}