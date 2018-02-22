//========================================================================
//
// xpdf.cc
//
// Copyright 1996-2105 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#include "gmem.h"
#include "Object.h"
#include "XpdfApp.h"

int main(int argc, char *argv[]) {
  int exitCode;

  {
    // this in inside a block so that the XpdfApp object gets freed
    XpdfApp app(argc, argv);
    if (app.getNumViewers() > 0) {
      exitCode = app.exec();
    } else {
      exitCode = 1;
    }
  }

  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}

#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE hIstance, HINSTANCE hPrevInstance,
		     LPSTR lpCmdLine, int nCmdShow) {
  wchar_t **args;
  int argc, i, n, ret;

  if (!(args = CommandLineToArgvW(GetCommandLineW(), &argc)) ||
      argc < 0) {
    return -1;
  }
  char **argv = (char  **)gmallocn(argc + 1, sizeof(char *));
  for (i = 0; i < argc; ++i) {
    n = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, NULL);
    argv[i] = (char *)gmalloc(n);
    WideCharToMultiByte(CP_ACP, 0, args[i], -1, argv[i], n, NULL, NULL);
  }
  argv[argc] = NULL;
  LocalFree(args);
  ret = main(argc, argv);
  for (i = 0; i < argc; ++i) {
    gfree(argv[i]);
  }
  gfree(argv);
  return ret;
}
#endif
