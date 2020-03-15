Global test resources:
- otb.h - replacement for standard otb.h, used to build otb_XXX.c modules
- esput.h - contains various necessary content to allow otb_XXX.c modules to build, and generic UT infrastructure

Per module test resources:
- test_XXX.c - contains tests for otb_XXX.c
- esput_XXX.c - contains resources required for testing otb_XXX.c (for example functions we need sample implementations of)
- esput_XXX.h - contains function headers, etc for esput_XXX.c stuff
- test_XXX entry in Makefile - builds bin/test_XXX app

To add tests for new module:
- Add section in otb.h (to include necessary otb_XXX.h files)
- Add test_XXX entry in Makefile 
- Write per module test resources

