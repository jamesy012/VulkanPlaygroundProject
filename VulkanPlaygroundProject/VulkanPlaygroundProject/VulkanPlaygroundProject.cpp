#include "stdafx.h"

#include "Application.h"
#include "Browser.h"


int main() {
   LOG_ARGS("PROGRAM STARTUP!\n");
   int result = BrowserStart(nullptr, true);
   if (result == 0) {
      return 0;
   }
   Application app;
   app.Start();
   app.Run();
   app.Destroy();
}